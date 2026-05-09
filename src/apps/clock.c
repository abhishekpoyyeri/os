/* src/apps/clock.c */
#include "apps.h"
#include "../drivers/rtc.h"
#include "../settings.h"

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

static void draw_soft_panel(int x, int y, int w, int h, uint8_t color) {
    vga_fill_rect(x + 2, y + 2, w, h, 25);
    vga_fill_rounded_rect(x, y, w, h, color);
    vga_draw_hline(x + 3, y + 1, w - 6, 39);
    vga_draw_rect(x, y, w, h, 40);
}

static void draw_status_pill(int x, int y, int w, const char* text, uint8_t accent) {
    vga_fill_rounded_rect(x, y, w, 13, 38);
    vga_draw_rect(x + 4, y + 3, 7, 7, accent); /* Simplified for now */
    font_draw_string(x + 15, y + 3, text, 4);
}

void clock_app_render(window_t* win) {
    uint8_t s, m, h, d, mo;
    uint32_t y;
    rtc_get_time(&s, &m, &h, &d, &mo, &y);

    char time_str[16];
    char date_str[16];
    int idx = 0;
    int cx = win->x + 8;
    int cy = win->y + 20 + 7;

    font_draw_string(cx, cy, "Live clock", 24);
    draw_status_pill(win->x + win->w - 48, cy - 2, 40, "RTC", 34);
    cy += 14;

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
    cy += 34;

    idx = 0;
    append_two_digit(date_str, &idx, d, 16);
    append_text(date_str, &idx, "/", 16);
    append_two_digit(date_str, &idx, mo, 16);
    append_text(date_str, &idx, "/", 16);
    append_number(date_str, &idx, (int)y, 16);
    date_str[idx] = '\0';

    font_draw_string(cx + 2, cy, "Date", 41);
    font_draw_string(cx + 36, cy, date_str, 4);
}
