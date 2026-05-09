/* src/apps/textedit_app.c */
#include "apps.h"
#include "../fs/vfs.h"
#include "../kernel/error.h"
#include "../kernel/klog.h"
#include <stdint.h>

#define TEXTEDITOR_MAX 8192
#define TEXTEDITOR_FILE "notes.txt"
static char editor_buffer[TEXTEDITOR_MAX];
static int editor_len = 0;
static uint8_t modified = 0;
static uint8_t loaded = 0;
static uint8_t save_click_handled = 0;

extern void vga_fill_rect(int x, int y, int w, int h, uint8_t color);
extern void vga_draw_rect(int x, int y, int w, int h, uint8_t color);
extern void vga_fill_rounded_rect(int x, int y, int w, int h, uint8_t color);
extern void font_draw_char(int x, int y, char c, uint8_t color);
extern void font_draw_string(int x, int y, const char* str, uint8_t color);
extern int font_string_width(const char* str);
extern int32_t mouse_get_x(void);
extern int32_t mouse_get_y(void);
extern uint8_t mouse_get_buttons(void);
extern void itoa(int n, char s[]);

static int local_memcmp(const void* a, const void* b, int len) {
    const uint8_t* pa = (const uint8_t*)a;
    const uint8_t* pb = (const uint8_t*)b;
    for (int i = 0; i < len; i++) {
        if (pa[i] != pb[i]) return (int)pa[i] - (int)pb[i];
    }
    return 0;
}

static void textedit_load_default(void) {
    loaded = 1;
    editor_len = 0;
    editor_buffer[0] = '\0';

    int fd = vfs_open(TEXTEDITOR_FILE);
    if (fd < 0) {
        klog_warn("TextEdit load: notes file not found");
        return;
    }

    uint32_t size = vfs_get_size(fd);
    if (size >= TEXTEDITOR_MAX) size = TEXTEDITOR_MAX - 1;
    int read_bytes = vfs_read(fd, (uint8_t*)editor_buffer, size);
    vfs_close(fd);

    if (read_bytes > 0) {
        editor_len = read_bytes;
        editor_buffer[editor_len] = '\0';
    }
    modified = 0;
    klog_info_u32("TextEdit loaded bytes", editor_len);
}

static void textedit_save_default(void) {
    klog_info("TextEdit save begin");
    if (!vfs_exists(TEXTEDITOR_FILE)) {
        int create_status = vfs_create(TEXTEDITOR_FILE, FILE_TYPE_TEXT);
        if (create_status != KERR_OK) {
            klog_error_u32("TextEdit create failed", (unsigned int)(-create_status));
            return;
        }
    }

    int fd = vfs_open(TEXTEDITOR_FILE);
    if (fd < 0) {
        klog_error("TextEdit save failed: open");
        return;
    }

    if (vfs_seek(fd, 0) != KERR_OK ||
        vfs_write(fd, (const uint8_t*)editor_buffer, (uint32_t)editor_len) != KERR_OK ||
        vfs_truncate(fd, (uint32_t)editor_len) != KERR_OK) {
        klog_error("TextEdit save failed: write/truncate");
        vfs_close(fd);
        return;
    }
    vfs_close(fd);

    char verify[TEXTEDITOR_MAX];
    fd = vfs_open(TEXTEDITOR_FILE);
    if (fd < 0) {
        klog_error("TextEdit verify failed: reopen");
        return;
    }
    int read_bytes = vfs_read(fd, (uint8_t*)verify, (uint32_t)editor_len);
    vfs_close(fd);

    if (read_bytes != editor_len || local_memcmp(editor_buffer, verify, editor_len) != 0) {
        klog_error("TextEdit save verification failed");
        return;
    }

    modified = 0;
    klog_info_u32("TextEdit save verified bytes", editor_len);
}

void textedit_load_external(const char* buf, int len) {
    if (!buf || len <= 0) {
        editor_len = 0;
        editor_buffer[0] = '\0';
    } else {
        if (len >= TEXTEDITOR_MAX) len = TEXTEDITOR_MAX - 1;
        /* copy manually — no memcpy in freestanding */
        for (int i = 0; i < len; i++) editor_buffer[i] = buf[i];
        editor_len = len;
        editor_buffer[editor_len] = '\0';
    }
    modified = 0;
    loaded   = 1;
    klog_info_u32("TextEdit external load bytes", (uint32_t)editor_len);
}


void textedit_app_key(char c) {
    if (!loaded) textedit_load_default();
    if (c == '\b') {
        if (editor_len > 0) {
            editor_len--;
            editor_buffer[editor_len] = '\0';
            modified = 1;
        }
    } else if (editor_len < TEXTEDITOR_MAX - 1) {
        editor_buffer[editor_len++] = c;
        editor_buffer[editor_len] = '\0';
        modified = 1;
    }
}

void textedit_app_render(window_t* win) {
    if (!loaded) textedit_load_default();

    int cx = win->x + 6;
    int cy = win->y + 20; // Titlebar height is 16 + 4
    int mx = mouse_get_x();
    int my = mouse_get_y();
    uint8_t mb = mouse_get_buttons();

    vga_fill_rounded_rect(cx, cy, win->w - 12, 14, 38);
    font_draw_string(cx + 6, cy + 4, "Text Editor", 24);
    
    // Save button
    vga_fill_rounded_rect(win->x + win->w - 42, cy + 2, 34, 10, modified ? 35 : 22);
    font_draw_string(win->x + win->w - 36, cy + 4, "Save", 4);
    if (mx >= win->x + win->w - 42 && mx < win->x + win->w - 8 &&
        my >= cy + 2 && my < cy + 12 && (mb & 1) && !save_click_handled) {
        save_click_handled = 1;
        textedit_save_default();
    }
    if (!(mb & 1)) save_click_handled = 0;
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
