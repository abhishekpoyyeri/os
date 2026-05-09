/* src/gui/desktop.c — Windows 11 Fluent Desktop Environment */
#include <stdint.h>
#include <stddef.h>

/* External VGA functions */
extern void vga_init(void);
extern void vga_putpixel(int x, int y, uint8_t color);
extern void vga_fill_rect(int x, int y, int w, int h, uint8_t color);
extern void vga_draw_rect(int x, int y, int w, int h, uint8_t color);
extern void vga_fill_rounded_rect(int x, int y, int w, int h, uint8_t color);
extern void vga_draw_hline(int x, int y, int len, uint8_t color);
extern void vga_draw_vline(int x, int y, int len, uint8_t color);
extern void vga_clear(uint8_t color);
extern void vga_swap(void);
extern int vga_get_width(void);
extern int vga_get_height(void);

/* External font functions */
extern void font_draw_char(int x, int y, char c, uint8_t color);
extern void font_draw_string(int x, int y, const char* str, uint8_t color);
extern void font_draw_string_shadow(int x, int y, const char* str, uint8_t fg, uint8_t shadow);
extern int font_string_width(const char* str);

/* External mouse functions */
extern int32_t mouse_get_x(void);
extern int32_t mouse_get_y(void);
extern uint8_t mouse_get_buttons(void);
extern void mouse_init(void);
extern void mouse_set_bounds(int32_t w, int32_t h);

/* External timer functions */
extern uint32_t timer_get_ticks(void);
extern void timer_init(uint32_t frequency);

/* External clock functions */
extern void rtc_get_time(uint8_t *second, uint8_t *minute, uint8_t *hour,
                         uint8_t *day, uint8_t *month, uint32_t *year);

/* External string/number functions */
extern size_t strlen(const char* str);
extern void itoa(int n, char s[]);

/* ===== Window System ===== */

#include "../apps/apps.h"

#define MAX_WINDOWS 16
#define TITLEBAR_HEIGHT 16
#define TASKBAR_HEIGHT  24
#define DESKTOP_ICON_COUNT 8

#define APP_CLOCK    0
#define APP_CALC     1
#define APP_ABOUT    2
#define APP_SYSMON   3
#define APP_TEXTEDIT 4
#define APP_TASKS    5
#define APP_HELP     6
#define APP_FILES    7
#define APP_SETTINGS 8

typedef struct {
    int x, y;
    char glyph;
    const char* label;
    uint8_t app_id;
    uint8_t color;
} desktop_icon_t;

typedef struct {
    window_t windows[MAX_WINDOWS];
    int window_count;
    int focused_window;
} WindowManager;

static WindowManager wm = { .window_count = 0, .focused_window = -1 };
#define windows wm.windows
#define window_count wm.window_count
#define focused_window wm.focused_window

#define MAX_EVENTS 128
typedef struct {
    uint8_t type;
    char key;
    int x, y;
    uint8_t buttons;
} gui_event_t;

static uint8_t gui_running = 0;

/* Click debounce: tracks last button state per frame */
static uint8_t calc_click_handled = 0;

/* Window dragging state */
static int dragging_window = -1;
static int drag_offset_x = 0;
static int drag_offset_y = 0;

static const desktop_icon_t desktop_icons[DESKTOP_ICON_COUNT] = {
    {14,  28, 'T', "Time",  APP_CLOCK,  34},
    {14,  68, 'N', "Notes", APP_TEXTEDIT,  35},
    {14, 108, 'M', "Stats", APP_SYSMON, 36},
    {76,  28, '+', "Calc",  APP_CALC,   37},
    {76,  68, '#', "Tasks", APP_TASKS,  11},
    {76, 108, '?', "Help",  APP_HELP,   30},
    {138, 28, 'F', "Files", APP_FILES,  42},
    {138, 68, 'S', "Setups",APP_SETTINGS, 43}
};

static uint8_t task_done[4] = {0, 0, 0, 0};
static uint8_t menu_open = 0;

/* ===== Calculator State ===== */

static int calc_num1 = 0;
static int calc_num2 = 0;
static int calc_result = 0;
static char calc_op = 0;
static uint8_t calc_state = 0; /* 0=num1, 1=entering num2, 2=result */
static char calc_display[16] = "0";

static void calc_update_display(void) {
    int val;
    if (calc_state == 2) val = calc_result;
    else if (calc_state == 1) val = calc_num2;
    else val = calc_num1;
    itoa(val, calc_display);
}

static void calc_press_digit(int digit) {
    if (calc_state == 0) {
        calc_num1 = calc_num1 * 10 + digit;
    } else if (calc_state == 1) {
        calc_num2 = calc_num2 * 10 + digit;
    } else {
        /* After result, start fresh */
        calc_num1 = digit;
        calc_num2 = 0;
        calc_op = 0;
        calc_state = 0;
    }
    calc_update_display();
}

static void calc_press_op(char op) {
    if (calc_state == 2) {
        calc_num1 = calc_result;
    }
    calc_op = op;
    calc_num2 = 0;
    calc_state = 1;
    calc_update_display();
}

static void calc_press_equals(void) {
    if (calc_state != 1 || calc_op == 0) return;
    if (calc_op == '+') calc_result = calc_num1 + calc_num2;
    else if (calc_op == '-') calc_result = calc_num1 - calc_num2;
    else if (calc_op == '*') calc_result = calc_num1 * calc_num2;
    else if (calc_op == '/' && calc_num2 != 0) calc_result = calc_num1 / calc_num2;
    else if (calc_op == '/' && calc_num2 == 0) { calc_result = 0; }
    calc_state = 2;
    calc_update_display();
}

static void calc_press_clear(void) {
    calc_num1 = 0; calc_num2 = 0; calc_result = 0;
    calc_op = 0; calc_state = 0;
    calc_display[0] = '0'; calc_display[1] = '\0';
}

static void append_text(char* out, int* idx, const char* text, int max) {
    for (int i = 0; text[i] && *idx < max - 1; i++) {
        out[(*idx)++] = text[i];
    }
    out[*idx] = '\0';
}

static void append_number(char* out, int* idx, int value, int max) {
    char buf[16];
    itoa(value, buf);
    append_text(out, idx, buf, max);
}

static void append_two_digit(char* out, int* idx, uint8_t value, int max) {
    if (value < 10 && *idx < max - 1) out[(*idx)++] = '0';
    append_number(out, idx, value, max);
}

static void make_time_string(char* out, uint8_t include_seconds) {
    uint8_t s, m, h, d, mo;
    uint32_t y;
    int idx = 0;
    rtc_get_time(&s, &m, &h, &d, &mo, &y);
    append_two_digit(out, &idx, h, 16);
    append_text(out, &idx, ":", 16);
    append_two_digit(out, &idx, m, 16);
    if (include_seconds) {
        append_text(out, &idx, ":", 16);
        append_two_digit(out, &idx, s, 16);
    }
    out[idx] = '\0';
}

static void make_date_string(char* out) {
    uint8_t s, m, h, d, mo;
    uint32_t y;
    int idx = 0;
    rtc_get_time(&s, &m, &h, &d, &mo, &y);
    append_two_digit(out, &idx, d, 16);
    append_text(out, &idx, "/", 16);
    append_two_digit(out, &idx, mo, 16);
    append_text(out, &idx, "/", 16);
    append_number(out, &idx, (int)y, 16);
    out[idx] = '\0';
}

static void draw_soft_panel(int x, int y, int w, int h, uint8_t color) {
    vga_fill_rect(x + 2, y + 2, w, h, 25);
    vga_fill_rounded_rect(x, y, w, h, color);
    vga_draw_hline(x + 3, y + 1, w - 6, 39);
    vga_draw_rect(x, y, w, h, 40);
}

static void draw_status_pill(int x, int y, int w, const char* text, uint8_t accent) {
    vga_fill_rounded_rect(x, y, w, 13, 38);
    vga_draw_vline(x + 4, y + 3, 7, accent);
    font_draw_string(x + 10, y + 3, text, 4);
}

static void draw_metric_bar(int x, int y, int w, int filled, uint8_t color) {
    if (filled < 0) filled = 0;
    if (filled > w) filled = w;
    vga_fill_rounded_rect(x, y, w, 6, 17);
    if (filled > 1) vga_fill_rounded_rect(x, y, filled, 6, color);
}

static const char* app_short_name(uint8_t app_id) {
    if (app_id == APP_CLOCK) return "Time";
    if (app_id == APP_CALC) return "Calc";
    if (app_id == APP_SYSMON) return "Stat";
    if (app_id == APP_TEXTEDIT) return "Note";
    if (app_id == APP_TASKS) return "Task";
    if (app_id == APP_HELP) return "Help";
    if (app_id == APP_FILES) return "File";
    if (app_id == APP_SETTINGS) return "Conf";
    return "Info";
}

/* ===== App Renderers ===== */


static void render_calc_app(window_t* win) {
    int cx = win->x + 6;
    int cy = win->y + TITLEBAR_HEIGHT + 4;

    /* Display area — dark rounded rectangle */
    vga_fill_rounded_rect(cx, cy, win->w - 12, 18, 16);
    /* Show operator indicator */
    if (calc_state == 1 && calc_op) {
        char op_str[2] = {calc_op, '\0'};
        font_draw_string(cx + 4, cy + 5, op_str, 24);
    }
    /* Right-align the number */
    int dw = font_string_width(calc_display);
    font_draw_string(cx + win->w - 16 - dw, cy + 5, calc_display, 4);
    cy += 22;

    /* Button grid: 4 cols x 4 rows */
    const char* labels[] = {
        "7","8","9","/",
        "4","5","6","*",
        "1","2","3","-",
        "C","0","=","+"
    };
    /* Color scheme per button type */
    /* digits=17, ops=18, equals=19, clear=30 */

    int btn_w = (win->w - 20) / 4;
    int btn_h = 14;
    int gap = 2;

    int32_t mx = mouse_get_x();
    int32_t my = mouse_get_y();
    uint8_t mb = mouse_get_buttons();

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int bx = cx + col * (btn_w + gap);
            int by = cy + row * (btn_h + gap);
            int bi = row * 4 + col;
            char lbl = labels[bi][0];

            /* Pick button color */
            uint8_t bg;
            if (lbl == '=') bg = 19;       /* Accent blue */
            else if (lbl == 'C') bg = 30;   /* Orange */
            else if (lbl == '+' || lbl == '-' || lbl == '*' || lbl == '/') bg = 18;
            else bg = 17;                    /* Number */

            /* Hover highlight */
            uint8_t is_hover = (mx >= bx && mx < bx + btn_w && my >= by && my < by + btn_h);
            if (is_hover) {
                if (lbl == '=') bg = 2;
                else bg = 23;
            }

            vga_fill_rounded_rect(bx, by, btn_w, btn_h, bg);
            /* Center label */
            font_draw_string(bx + btn_w/2 - 3, by + 3, labels[bi], 4);

            /* Handle click with debounce */
            if (is_hover && (mb & 1) && !calc_click_handled) {
                calc_click_handled = 1;
                if (lbl >= '0' && lbl <= '9') calc_press_digit(lbl - '0');
                else if (lbl == '+' || lbl == '-' || lbl == '*' || lbl == '/') calc_press_op(lbl);
                else if (lbl == '=') calc_press_equals();
                else if (lbl == 'C') calc_press_clear();
            }
        }
    }
}

static void render_about_app(window_t* win) {
    int cx = win->x + 8;
    int cy = win->y + TITLEBAR_HEIGHT + 8;

    draw_soft_panel(cx, cy, win->w - 16, 28, 16);
    font_draw_string_shadow(cx + 9, cy + 6, "MyOS v0.4", 4, 25);
    font_draw_string(cx + 9, cy + 17, "Aurora desktop", 24);
    cy += 38;
    font_draw_string(cx, cy, "32-bit x86 kernel", 24);
    cy += 11;
    font_draw_string(cx, cy, "Mode 13h graphics", 24);
    cy += 11;
    font_draw_string(cx, cy, "Mouse, windows, apps", 24);
    cy += 14;
    font_draw_string(cx, cy, "ESC returns to shell", 11);
}

/* ===== System Monitor App ===== */

extern uint32_t pmm_get_total_pages(void);
extern uint32_t pmm_get_free_pages(void);
extern uint32_t pmm_get_used_pages(void);

static void render_sysmon_app(window_t* win) {
    int cx = win->x + 8;
    int cy = win->y + TITLEBAR_HEIGHT + 6;
    char buf[16];

    font_draw_string(cx, cy, "System health", 4);
    cy += 14;

    uint32_t total = pmm_get_total_pages();
    uint32_t used = pmm_get_used_pages();
    uint32_t free_p = pmm_get_free_pages();
    int bar_w = win->w - 20;
    int fill_w = (total > 0) ? (int)((uint32_t)bar_w * used / total) : 0;
    int free_w = (total > 0) ? (int)((uint32_t)bar_w * free_p / total) : 0;

    font_draw_string(cx, cy, "Memory:", 24);
    cy += 10;
    draw_metric_bar(cx, cy, bar_w, fill_w, 19);
    cy += 12;

    itoa(used * 4, buf);
    font_draw_string(cx, cy, "Used", 41);
    font_draw_string(cx + 42, cy, buf, 4);
    font_draw_string(cx + 42 + font_string_width(buf), cy, " KB", 24);
    cy += 10;

    itoa(free_p * 4, buf);
    font_draw_string(cx, cy, "Free", 41);
    font_draw_string(cx + 42, cy, buf, 8);
    font_draw_string(cx + 42 + font_string_width(buf), cy, " KB", 24);
    cy += 12;

    font_draw_string(cx, cy, "Free pages", 41);
    draw_metric_bar(cx + 70, cy + 1, bar_w - 70, free_w, 35);
    cy += 12;

    uint32_t ticks = timer_get_ticks();
    uint32_t secs = ticks / 100;
    uint32_t mins = secs / 60;
    font_draw_string(cx, cy, "Uptime", 41);
    itoa(mins, buf);
    font_draw_string(cx + 56, cy, buf, 4);
    font_draw_string(cx + 56 + font_string_width(buf), cy, "m ", 24);
    itoa(secs % 60, buf);
    font_draw_string(cx + 76, cy, buf, 4);
    font_draw_string(cx + 76 + font_string_width(buf), cy, "s", 24);
    cy += 12;

    font_draw_string(cx, cy, "Open apps", 41);
    int open_count = 0;
    for (int i = 0; i < window_count; i++) {
        if (windows[i].is_open) open_count++;
    }
    itoa(open_count, buf);
    font_draw_string(cx + 70, cy, buf, 11);
}

/* ===== Notepad App ===== */


/* ===== Tasks App ===== */

static void render_tasks_app(window_t* win) {
    int cx = win->x + 8;
    int cy = win->y + TITLEBAR_HEIGHT + 7;
    const char* tasks[] = {
        "Boot cleanly",
        "Open desktop",
        "Check memory",
        "Write a note"
    };

    font_draw_string(cx, cy, "Today", 4);
    draw_status_pill(win->x + win->w - 54, cy - 2, 46, "Local", 35);
    cy += 16;

    for (int i = 0; i < 4; i++) {
        int row_y = cy + i * 18;
        vga_fill_rounded_rect(cx, row_y, win->w - 16, 15, 38);
        vga_draw_rect(cx + 3, row_y + 3, 9, 9, task_done[i] ? 35 : 40);
        if (task_done[i]) {
            font_draw_char(cx + 5, row_y + 3, 'x', 35);
        }
        font_draw_string(cx + 18, row_y + 4, tasks[i], task_done[i] ? 8 : 4);
    }
}

/* ===== Help App ===== */

static void render_help_app(window_t* win) {
    int cx = win->x + 8;
    int cy = win->y + TITLEBAR_HEIGHT + 8;

    font_draw_string(cx, cy, "Shortcuts", 4);
    cy += 14;
    font_draw_string(cx, cy, "c  calculator", 24);
    cy += 10;
    font_draw_string(cx, cy, "t  clock", 24);
    cy += 10;
    font_draw_string(cx, cy, "n  notes", 24);
    cy += 10;
    font_draw_string(cx, cy, "m  system monitor", 24);
    cy += 10;
    font_draw_string(cx, cy, "h  this help", 24);
    cy += 14;
    font_draw_string(cx, cy, "Click icons or Menu", 11);
    cy += 10;
    font_draw_string(cx, cy, "ESC exits desktop", 30);
}

/* ===== Window Management ===== */

static void focus_window(int index) {
    if (index < 0 || index >= window_count) return;
    for (int j = 0; j < window_count; j++) windows[j].is_focused = 0;
    windows[index].is_focused = 1;
    focused_window = index;
}

static int raise_window(int index) {
    if (index < 0 || index >= window_count) return -1;
    if (index == window_count - 1) {
        focus_window(index);
        return index;
    }

    window_t temp = windows[index];
    for (int i = index; i < window_count - 1; i++) {
        windows[i] = windows[i + 1];
    }
    windows[window_count - 1] = temp;
    focus_window(window_count - 1);
    return window_count - 1;
}

static int create_window(int x, int y, int w, int h, const char* title, uint8_t app_id) {
    int slot = -1;
    for (int i = 0; i < window_count; i++) {
        if (!windows[i].is_open) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        if (window_count >= MAX_WINDOWS) return -1;
        slot = window_count++;
    }

    window_t* win = &windows[slot];
    win->x = x; win->y = y; win->w = w; win->h = h;
    win->title = title;
    win->is_open = 1; win->is_focused = 0;
    win->bg_color = 38;
    win->app_id = app_id;
    return raise_window(slot);
}

static int find_open_app(uint8_t app_id) {
    for (int i = 0; i < window_count; i++) {
        if (windows[i].is_open && windows[i].app_id == app_id) return i;
    }
    return -1;
}

static int open_app(uint8_t app_id) {
    int existing = find_open_app(app_id);
    if (existing >= 0) return raise_window(existing);

    if (app_id == APP_CLOCK) return create_window(178, 18, 126, 92, "Clock", APP_CLOCK);
    if (app_id == APP_CALC) return create_window(182, 36, 122, 122, "Calc", APP_CALC);
    if (app_id == APP_SYSMON) return create_window(138, 20, 164, 126, "System", APP_SYSMON);
    if (app_id == APP_TEXTEDIT) return create_window(132, 26, 170, 132, "Notes", APP_TEXTEDIT);
    if (app_id == APP_TASKS) return create_window(148, 22, 154, 112, "Tasks", APP_TASKS);
    if (app_id == APP_HELP) return create_window(138, 22, 164, 120, "Help", APP_HELP);
    if (app_id == APP_FILES) return create_window(120, 30, 180, 140, "Files", APP_FILES);
    return create_window(164, 28, 140, 104, "About", APP_ABOUT);
}

static void draw_window(window_t* win) {
    if (!win->is_open) return;

    vga_fill_rect(win->x + 3, win->y + 3, win->w, win->h, 25);

    vga_fill_rounded_rect(win->x, win->y, win->w, win->h, win->bg_color);

    uint8_t tb_color = win->is_focused ? 34 : 29;
    vga_fill_rounded_rect(win->x, win->y, win->w, TITLEBAR_HEIGHT + 2, tb_color);
    vga_fill_rect(win->x, win->y + TITLEBAR_HEIGHT - 2, win->w, 4, tb_color);

    if (win->is_focused) {
        vga_draw_hline(win->x + 1, win->y + TITLEBAR_HEIGHT, win->w - 2, 35);
    }

    font_draw_char(win->x + 7, win->y + 4, app_short_name(win->app_id)[0], 4);
    font_draw_string(win->x + 18, win->y + 4, win->title, 4);

    int cbx = win->x + win->w - 14;
    int cby = win->y + 3;
    vga_fill_rounded_rect(cbx, cby, 11, 10, win->is_focused ? 37 : 9);
    font_draw_char(cbx + 2, cby + 1, 'x', 4);

    vga_draw_rect(win->x, win->y, win->w, win->h, win->is_focused ? 40 : 6);

    switch (win->app_id) {
        case APP_CLOCK:  clock_app_render(win); break;
        case APP_CALC:   render_calc_app(win); break;
        case APP_ABOUT:  render_about_app(win); break;
        case APP_SYSMON: render_sysmon_app(win); break;
        case APP_TEXTEDIT: textedit_app_render(win); break;
        case APP_TASKS:  render_tasks_app(win); break;
        case APP_HELP:   render_help_app(win); break;
        case APP_FILES:  files_app_render(win); break;
    }
}

/* ===== Taskbar (centered, Windows 11 style) ===== */

static void draw_taskbar(void) {
    int sw = vga_get_width();
    int sh = vga_get_height();
    int ty = sh - TASKBAR_HEIGHT;

    vga_fill_rect(0, ty, sw, TASKBAR_HEIGHT, 12);
    vga_draw_hline(0, ty, sw, 40);

    vga_fill_rounded_rect(8, ty + 4, 38, 16, menu_open ? 34 : 22);
    font_draw_string(18, ty + 8, "OS", 4);

    int bx = 56;
    for (int i = 0; i < window_count; i++) {
        if (!windows[i].is_open) continue;
        uint8_t bg = (i == focused_window) ? 28 : 22;
        vga_fill_rounded_rect(bx, ty + 4, 24, 16, bg);
        if (i == focused_window) {
            vga_fill_rect(bx + 7, ty + 20, 10, 2, 35);
        }
        font_draw_char(bx + 9, ty + 8, app_short_name(windows[i].app_id)[0], 4);
        bx += 28;
    }

    char tb[16];
    make_time_string(tb, 0);
    draw_status_pill(sw - 54, ty + 5, 46, tb, 36);
}

/* ===== Mouse Cursor (Win11 arrow) ===== */

static void draw_cursor(int x, int y) {
    static const uint8_t cursor[] = {
        0x80, 0xC0, 0xE0, 0xF0,
        0xF8, 0xFC, 0xE0, 0x90
    };
    /* Shadow */
    for (int row = 0; row < 8; row++)
        for (int col = 0; col < 8; col++)
            if (cursor[row] & (0x80 >> col))
                vga_putpixel(x + col + 1, y + row + 1, 25);
    /* White body */
    for (int row = 0; row < 8; row++)
        for (int col = 0; col < 8; col++)
            if (cursor[row] & (0x80 >> col))
                vga_putpixel(x + col, y + row, 15);
}

/* ===== Start Menu (Win11 centered flyout) ===== */

static void draw_menu(void) {
    if (!menu_open) return;
    int sh = vga_get_height();

    int mw = 188;
    int mh = 118;
    int mx = 8;
    int my2 = sh - TASKBAR_HEIGHT - mh - 4;

    draw_soft_panel(mx, my2, mw, mh, 22);
    font_draw_string(mx + 10, my2 + 8, "Launcher", 4);
    font_draw_string(mx + 88, my2 + 8, "MyOS", 41);

    const char* items[] = {"Clock", "Calculator", "System", "Notes", "Tasks", "Help", "About"};
    uint8_t app_ids[] = {APP_CLOCK, APP_CALC, APP_SYSMON, APP_TEXTEDIT, APP_TASKS, APP_HELP, APP_ABOUT};
    int32_t mmx = mouse_get_x(), mmy = mouse_get_y();

    for (int i = 0; i < 7; i++) {
        int col = i % 2;
        int row = i / 2;
        int ix = mx + 10 + col * 88;
        int iy = my2 + 25 + row * 20;
        uint8_t bg = 38;
        if (mmx >= ix && mmx < ix + 78 && mmy >= iy && mmy < iy + 16) {
            bg = 43;
        }
        vga_fill_rounded_rect(ix, iy, 78, 16, bg);
        vga_fill_rounded_rect(ix + 4, iy + 3, 10, 10, 34 + (app_ids[i] % 4));
        font_draw_char(ix + 6, iy + 4, app_short_name(app_ids[i])[0], 4);
        font_draw_string(ix + 20, iy + 4, items[i], 4);
    }

    font_draw_string(mx + 10, my2 + mh - 13, "Tip: desktop icons open apps", 41);
}

/* ===== Desktop Background ===== */

static void draw_desktop_icons(void) {
    int32_t mx = mouse_get_x();
    int32_t my = mouse_get_y();

    for (int i = 0; i < DESKTOP_ICON_COUNT; i++) {
        const desktop_icon_t* icon = &desktop_icons[i];
        uint8_t hover = (mx >= icon->x && mx < icon->x + 48 &&
                         my >= icon->y && my < icon->y + 32);
        vga_fill_rounded_rect(icon->x, icon->y, 48, 32, hover ? 43 : 42);
        vga_fill_rounded_rect(icon->x + 6, icon->y + 5, 16, 16, icon->color);
        font_draw_char(icon->x + 11, icon->y + 9, icon->glyph, 4);
        font_draw_string(icon->x + 6, icon->y + 23, icon->label, 4);
    }
}

static void draw_desktop_bg(void) {
    int sw = vga_get_width();
    int sh = vga_get_height();

    for (int y = 0; y < sh - TASKBAR_HEIGHT; y++) {
        uint8_t color;
        if (y < sh / 4) color = 33;
        else if (y < sh / 2) color = 34;
        else if (y < 3 * sh / 4) color = 26;
        else color = 32;
        for (int x = 0; x < sw; x++) {
            vga_putpixel(x, y, color);
        }
    }

    for (int x = 0; x < sw; x += 2) {
        int y1 = 20 + x / 7;
        int y2 = 96 - x / 10;
        if (y1 < sh - TASKBAR_HEIGHT) vga_putpixel(x, y1, 35);
        if (y1 + 1 < sh - TASKBAR_HEIGHT) vga_putpixel(x, y1 + 1, 34);
        if (y2 > 0 && y2 < sh - TASKBAR_HEIGHT) vga_putpixel(x, y2, 37);
    }

    font_draw_string_shadow(154, 10, "MyOS", 4, 25);
    font_draw_string(154, 22, "aurora workspace", 24);

    char time_buf[16];
    char date_buf[16];
    make_time_string(time_buf, 0);
    make_date_string(date_buf);
    draw_soft_panel(sw - 102, 12, 88, 45, 38);
    font_draw_string(sw - 92, 22, time_buf, 4);
    font_draw_string(sw - 92, 34, date_buf, 24);

    uint32_t total = pmm_get_total_pages();
    uint32_t used = pmm_get_used_pages();
    int fill = (total > 0) ? (int)(44 * used / total) : 0;
    font_draw_string(sw - 92, 46, "RAM", 41);
    draw_metric_bar(sw - 64, 48, 44, fill, 35);

    draw_desktop_icons();
}

/* ===== Input Handling ===== */

static uint8_t prev_buttons = 0;

/* Keyboard input for notepad — called from keyboard_handler via shell */
void desktop_key_input(char c) {
    /* ESC exits GUI mode */
    if (c == 27) {
        extern volatile uint8_t gui_mode;
        gui_mode = 0;
        /* Restore text mode */
        extern void terminal_initialize(void);
        terminal_initialize();
        extern void terminal_writestring(const char* data);
        terminal_writestring("Returned to shell.\n> ");
        return;
    }

    /* Forward keys to text editor if it's focused */
    if (focused_window >= 0 && windows[focused_window].app_id == APP_TEXTEDIT && windows[focused_window].is_open) {
        textedit_app_key(c);
        return;
    }

    /* Forward keys to clock app if it's focused */
    if (focused_window >= 0 && windows[focused_window].app_id == APP_CLOCK && windows[focused_window].is_open) {
        clock_app_key(c);
        return;
    }

    if (c == 'c') open_app(APP_CALC);
    else if (c == 't') open_app(APP_CLOCK);
    else if (c == 'n') open_app(APP_TEXTEDIT);
    else if (c == 'm') open_app(APP_SYSMON);
    else if (c == 'h') open_app(APP_HELP);
    else if (c == 'f') open_app(APP_FILES);
}

static void handle_input(void) {
    int32_t mx = mouse_get_x();
    int32_t my = mouse_get_y();
    uint8_t mb = mouse_get_buttons();

    uint8_t clicked = (mb & 1) && !(prev_buttons & 1);
    prev_buttons = mb;

    /* Reset calc debounce when mouse released */
    if (!(mb & 1)) calc_click_handled = 0;

    /* Handle window dragging */
    if (dragging_window >= 0) {
        if (mb & 1) {
            /* Still holding — move the window */
            window_t* win = &windows[dragging_window];
            win->x = mx - drag_offset_x;
            win->y = my - drag_offset_y;
            /* Clamp to screen */
            if (win->x < 0) win->x = 0;
            if (win->y < 0) win->y = 0;
            int sw2 = vga_get_width();
            int sh2 = vga_get_height() - TASKBAR_HEIGHT;
            if (win->x + win->w > sw2) win->x = sw2 - win->w;
            if (win->y + win->h > sh2) win->y = sh2 - win->h;
            return;
        } else {
            /* Released — stop dragging */
            dragging_window = -1;
        }
    }

    if (!clicked) return;

    int sh = vga_get_height();

    if (mx >= 8 && mx < 46 &&
        my >= sh - TASKBAR_HEIGHT + 4 && my < sh - TASKBAR_HEIGHT + 20) {
        menu_open = !menu_open;
        return;
    }

    if (menu_open) {
        int mw2 = 188;
        int mmx2 = 8;
        int mh2 = 118;
        int mmy2 = sh - TASKBAR_HEIGHT - mh2 - 4;
        uint8_t app_ids[] = {APP_CLOCK, APP_CALC, APP_SYSMON, APP_TEXTEDIT, APP_TASKS, APP_HELP, APP_ABOUT};
        if (mx >= mmx2 && mx < mmx2 + mw2 && my >= mmy2 && my < mmy2 + mh2) {
            for (int i = 0; i < 7; i++) {
                int col = i % 2;
                int row = i / 2;
                int ix = mmx2 + 10 + col * 88;
                int iy = mmy2 + 25 + row * 20;
                if (mx >= ix && mx < ix + 78 && my >= iy && my < iy + 16) {
                    open_app(app_ids[i]);
                    menu_open = 0;
                    return;
                }
            }
            return;
        }
        menu_open = 0;
    }

    {
        int bx = 56;
        for (int i = 0; i < window_count; i++) {
            if (!windows[i].is_open) continue;
            if (mx >= bx && mx < bx + 24 &&
                my >= sh - TASKBAR_HEIGHT + 4 && my < sh - TASKBAR_HEIGHT + 20) {
                raise_window(i);
                return;
            }
            bx += 28;
        }
    }

    /* Window interactions (reverse for z-order) */
    for (int i = window_count - 1; i >= 0; i--) {
        window_t* win = &windows[i];
        if (!win->is_open) continue;
        if (mx >= win->x && mx < win->x + win->w &&
            my >= win->y && my < win->y + win->h) {
            int cbx = win->x + win->w - 14;
            int cby = win->y + 3;
            if (mx >= cbx && mx < cbx + 11 && my >= cby && my < cby + 10) {
                win->is_open = 0;
                if (focused_window == i) {
                    focused_window = -1;
                    for (int j = window_count - 1; j >= 0; j--) {
                        if (windows[j].is_open) {
                            focus_window(j);
                            break;
                        }
                    }
                }
                return;
            }

            int active = raise_window(i);
            win = &windows[active];

            if (my >= win->y && my < win->y + TITLEBAR_HEIGHT) {
                dragging_window = active;
                drag_offset_x = mx - win->x;
                drag_offset_y = my - win->y;
                return;
            }


            if (win->app_id == APP_TASKS) {
                int tx = win->x + 8;
                int ty = win->y + TITLEBAR_HEIGHT + 23;
                for (int t = 0; t < 4; t++) {
                    int row_y = ty + t * 18;
                    if (mx >= tx && mx < tx + win->w - 16 &&
                        my >= row_y && my < row_y + 15) {
                        task_done[t] = !task_done[t];
                        return;
                    }
                }
            }
            return;
        }
    }

    for (int i = 0; i < DESKTOP_ICON_COUNT; i++) {
        const desktop_icon_t* icon = &desktop_icons[i];
        if (mx >= icon->x && mx < icon->x + 48 &&
            my >= icon->y && my < icon->y + 32) {
            open_app(icon->app_id);
            return;
        }
    }
}

/* ===== Public API ===== */

void desktop_init(void) {
    vga_init();
    mouse_init();
    mouse_set_bounds(vga_get_width(), vga_get_height());
    timer_init(100);
    gui_running = 1;
    menu_open = 0;
    dragging_window = -1;
    window_count = 0;
    focused_window = -1;

    calc_press_clear();
    create_window(164, 28, 140, 104, "Welcome", APP_ABOUT);
}

void desktop_render_frame(void) {
    draw_desktop_bg();
    for (int i = 0; i < window_count; i++) draw_window(&windows[i]);
    draw_taskbar();
    draw_menu();
    draw_cursor(mouse_get_x(), mouse_get_y());
    vga_swap();
}

void desktop_update(void) {
    handle_input();
    desktop_render_frame();
}

uint8_t desktop_is_running(void) {
    return gui_running;
}
