/* src/apps/clock.c */
#include "apps.h"
#include "../drivers/rtc.h"
#include "../settings.h"
#include "../kernel/klog.h"
#include <stddef.h>

extern void vga_fill_rect(int x, int y, int w, int h, uint8_t color);
extern void vga_fill_rounded_rect(int x, int y, int w, int h, uint8_t color);
extern void vga_draw_hline(int x, int y, int len, uint8_t color);
extern void vga_draw_rect(int x, int y, int w, int h, uint8_t color);
extern void font_draw_string(int x, int y, const char* str, uint8_t color);
extern void font_draw_string_shadow(int x, int y, const char* str, uint8_t fg, uint8_t shadow);
extern void font_draw_char(int x, int y, char c, uint8_t color);
extern int font_string_width(const char* str);
extern void itoa(int n, char s[]);
extern size_t strlen(const char* str);
extern int32_t mouse_get_x(void);
extern int32_t mouse_get_y(void);
extern uint8_t mouse_get_buttons(void);

/* ---- helpers ---- */
static void append_text(char* out, int* idx, const char* text, int max) {
    for (int i = 0; text[i] && *idx < max - 1; i++)
        out[(*idx)++] = text[i];
    out[*idx] = '\0';
}

static void append_number(char* out, int* idx, int value, int max) {
    char buf[16]; itoa(value, buf);
    append_text(out, idx, buf, max);
}

static void append_two_digit(char* out, int* idx, uint8_t value, int max) {
    if (value < 10 && *idx < max - 1) out[(*idx)++] = '0';
    append_number(out, idx, value, max);
}

static void draw_soft_panel(int x, int y, int w, int h, uint8_t color) {
    vga_fill_rect(x + 2, y + 2, w, h, 25);
    vga_fill_rounded_rect(x, y, w, h, color);
    vga_draw_hline(x + 3, y + 1, w - 6, 39);
    vga_draw_rect(x, y, w, h, 40);
}

/* ---- edit state ---- */
static uint8_t editing   = 0;   /* 0 = view, 1 = edit */
/*
 * edit_buf layout: "HH:MM" (5 chars + NUL)
 * digit positions: 0,1,3,4 (index 2 is fixed ':')
 */
static char edit_buf[6]  = "00:00";
static int  edit_pos     = 0;   /* next digit to fill (0-3 logical) */
static uint8_t clock_click_handled = 0;

/* Convert logical digit index 0-3 to buf index */
static int digit_to_buf(int d) { return d < 2 ? d : d + 1; }

static void clock_enter_edit(void) {
    uint8_t s, m, h, d, mo; uint32_t y;
    rtc_get_time(&s, &m, &h, &d, &mo, &y);
    edit_buf[0] = (char)('0' + h / 10);
    edit_buf[1] = (char)('0' + h % 10);
    edit_buf[2] = ':';
    edit_buf[3] = (char)('0' + m / 10);
    edit_buf[4] = (char)('0' + m % 10);
    edit_buf[5] = '\0';
    edit_pos = 0;
    editing  = 1;
    klog_info("Clock: entered edit mode");
}

static void clock_apply_edit(void) {
    int hh = (edit_buf[0] - '0') * 10 + (edit_buf[1] - '0');
    int mm = (edit_buf[3] - '0') * 10 + (edit_buf[4] - '0');
    klog_info_u32("Clock apply hour", (uint32_t)hh);
    klog_info_u32("Clock apply min",  (uint32_t)mm);
    if (hh < 0 || hh > 23 || mm < 0 || mm > 59) {
        klog_warn("Clock: invalid time entered");
        editing = 0;
        return;
    }
    uint8_t s, m, h, d, mo; uint32_t y;
    rtc_get_time(&s, &m, &h, &d, &mo, &y);
    rtc_set_time((uint8_t)hh, (uint8_t)mm, d, mo, y);
    rtc_update_cache();
    klog_info("Clock: rtc_set_time + rtc_update_cache called");
    editing = 0;
}

/* ---- public key handler (called by desktop when Clock window focused) ---- */
void clock_app_key(char c) {
    if (!editing) {
        if (c == 'e' || c == 'E') { clock_enter_edit(); }
        return;
    }
    if (c >= '0' && c <= '9') {
        if (edit_pos < 4) {
            edit_buf[digit_to_buf(edit_pos)] = c;
            edit_pos++;
        }
    } else if (c == '\b') {
        if (edit_pos > 0) {
            edit_pos--;
            edit_buf[digit_to_buf(edit_pos)] = '0';
        }
    } else if (c == '\n') {
        clock_apply_edit();
    } else if (c == 27) {      /* ESC */
        editing = 0;
        klog_info("Clock: edit cancelled");
    }
}

/* ---- render ---- */
void clock_app_render(window_t* win) {
    uint8_t s, m, h, d, mo;
    uint32_t y;
    rtc_get_time(&s, &m, &h, &d, &mo, &y);

    char time_str[16];
    char date_str[16];
    int idx = 0;
    int cx  = win->x + 8;
    int cy  = win->y + 20 + 4;

    /* ---- header row ---- */
    font_draw_string(cx, cy, editing ? "Edit time" : "Live clock", 24);

    /* Edit / Apply button */
    int32_t mx = mouse_get_x();
    int32_t my = mouse_get_y();
    uint8_t mb = mouse_get_buttons();

    int btn_x = win->x + win->w - 48;
    int btn_y = cy - 2;
    uint8_t btn_hover = (mx >= btn_x && mx < btn_x + 40 &&
                         my >= btn_y && my < btn_y + 13);
    uint8_t btn_color = editing ? 35 : 34;
    if (btn_hover) btn_color = 19;
    vga_fill_rounded_rect(btn_x, btn_y, 40, 13, btn_color);
    font_draw_string(btn_x + 4, btn_y + 3, editing ? "Apply" : "Edit", 4);

    if (btn_hover && (mb & 1) && !clock_click_handled) {
        clock_click_handled = 1;
        if (editing) {
            klog_info("Clock: Apply button clicked");
            clock_apply_edit();
        } else {
            klog_info("Clock: Edit button clicked");
            clock_enter_edit();
        }
    }
    if (!(mb & 1)) clock_click_handled = 0;

    cy += 16;

    /* ---- time display / edit field ---- */
    if (editing) {
        /* Show editable buffer */
        draw_soft_panel(cx, cy, win->w - 16, 26, 17);
        /* Cursor highlight on current digit */
        if (edit_pos < 4) {
            int dbi = digit_to_buf(edit_pos);
            font_draw_char(cx + 18 + dbi * 7, cy + 9, edit_buf[dbi], 35);
            /* draw other chars normally */
            for (int i = 0; i < 5; i++) {
                if (i == dbi) continue;
                font_draw_char(cx + 18 + i * 7, cy + 9, edit_buf[i], 4);
            }
        } else {
            font_draw_string(cx + 18, cy + 9, edit_buf, 4);
        }
        font_draw_string(cx + 4, cy + 9, "HH:MM", 41);
    } else {
        /* Show live time */
        SystemSettings* settings = settings_get();
        idx = 0;
        if (h < 10) time_str[idx++] = '0';
        append_number(time_str, &idx, h, 16);
        time_str[idx++] = ':';
        if (m < 10) time_str[idx++] = '0';
        append_number(time_str, &idx, m, 16);
        if (settings->show_seconds) {
            time_str[idx++] = ':';
            if (s < 10) time_str[idx++] = '0';
            append_number(time_str, &idx, s, 16);
        }
        time_str[idx] = '\0';
        draw_soft_panel(cx, cy, win->w - 16, 26, 16);
        font_draw_string_shadow(cx + 18, cy + 9, time_str, 4, 25);
    }
    cy += 34;

    /* ---- date row ---- */
    idx = 0;
    append_two_digit(date_str, &idx, d,  16);
    append_text(date_str, &idx, "/", 16);
    append_two_digit(date_str, &idx, mo, 16);
    append_text(date_str, &idx, "/", 16);
    append_number(date_str, &idx, (int)y, 16);
    date_str[idx] = '\0';
    font_draw_string(cx + 2, cy, "Date", 41);
    font_draw_string(cx + 36, cy, date_str, 4);
}
