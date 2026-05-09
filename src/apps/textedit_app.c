/* src/apps/textedit_app.c */
#include "apps.h"
#include "../fs/vfs.h"
#include <stdint.h>

#define TEXTEDITOR_MAX 8192
static char editor_buffer[TEXTEDITOR_MAX];
static int editor_len = 0;
static uint8_t modified = 0;

extern void vga_fill_rect(int x, int y, int w, int h, uint8_t color);
extern void vga_draw_rect(int x, int y, int w, int h, uint8_t color);
extern void vga_fill_rounded_rect(int x, int y, int w, int h, uint8_t color);
extern void font_draw_char(int x, int y, char c, uint8_t color);
extern void font_draw_string(int x, int y, const char* str, uint8_t color);
extern int font_string_width(const char* str);
extern void itoa(int n, char s[]);

void textedit_app_key(char c) {
    if (c == '\b') {
        if (editor_len > 0) {
            editor_len--;
            modified = 1;
        }
    } else if (editor_len < TEXTEDITOR_MAX - 1) {
        editor_buffer[editor_len++] = c;
        modified = 1;
    }
}

void textedit_app_render(window_t* win) {
    int cx = win->x + 6;
    int cy = win->y + 20; // Titlebar height is 16 + 4

    vga_fill_rounded_rect(cx, cy, win->w - 12, 14, 38);
    font_draw_string(cx + 6, cy + 4, "Text Editor", 24);
    
    // Save button
    vga_fill_rounded_rect(win->x + win->w - 42, cy + 2, 34, 10, modified ? 35 : 22);
    font_draw_string(win->x + win->w - 36, cy + 4, "Save", 4);
    cy += 18;

    vga_fill_rect(cx, cy, win->w - 12, win->h - 16 - 22, 16);
    vga_draw_rect(cx, cy, win->w - 12, win->h - 16 - 22, 40);

    if (editor_len == 0) {
        font_draw_string(cx + 5, cy + 5, "Type here...", 41);
    } else {
        int tx = cx + 4;
        int ty = cy + 4;
        int max_x = cx + win->w - 16;
        for (int i = 0; i < editor_len; i++) {
            if (editor_buffer[i] == '\n' || tx + 7 > max_x) {
                tx = cx + 4;
                ty += 10;
                if (editor_buffer[i] == '\n') continue;
            }
            if (ty + 10 > win->y + win->h - 8) break;
            font_draw_char(tx, ty, editor_buffer[i], 4);
            tx += 7;
        }
    }

    int sy = win->y + win->h - 10;
    char lbuf[8];
    itoa(editor_len, lbuf);
    font_draw_string(cx + 4, sy, lbuf, 24);
    font_draw_string(cx + 4 + font_string_width(lbuf), sy, " chars", 24);
    if (modified) {
        font_draw_string(cx + win->w - 60, sy, "*unsaved", 35);
    }
}
