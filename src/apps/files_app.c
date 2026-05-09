/* src/apps/files_app.c */
#include "apps.h"
#include "../fs/vfs.h"
#include "../kernel/klog.h"
#include <stdint.h>

extern void vga_fill_rect(int x, int y, int w, int h, uint8_t color);
extern void vga_fill_rounded_rect(int x, int y, int w, int h, uint8_t color);
extern void vga_draw_rect(int x, int y, int w, int h, uint8_t color);
extern void font_draw_char(int x, int y, char c, uint8_t color);
extern void font_draw_string(int x, int y, const char* str, uint8_t color);
extern int32_t mouse_get_x(void);
extern int32_t mouse_get_y(void);
extern uint8_t mouse_get_buttons(void);
extern void itoa(int n, char s[]);

static int selected_file = -1;
static uint8_t files_click_handled = 0;

static const char* cached_names[MYFS_MAX_FILES];
static uint32_t cached_sizes[MYFS_MAX_FILES];
static uint8_t cached_types[MYFS_MAX_FILES];
static int file_count = 0;

static void list_cb(const char* name, uint32_t size, uint8_t type) {
    if (file_count < MYFS_MAX_FILES) {
        cached_names[file_count] = name;
        cached_sizes[file_count] = size;
        cached_types[file_count] = type;
        file_count++;
    }
}

static void refresh_files(void) {
    file_count = 0;
    vfs_list(list_cb);
}

void files_app_render(window_t* win) {
    int cx = win->x + 6;
    int cy = win->y + 20;
    
    // Periodically refresh, or rely on events (for now, refresh every frame is okay for this simple implementation)
    refresh_files();

    vga_fill_rounded_rect(cx, cy, win->w - 12, 14, 38);
    font_draw_string(cx + 6, cy + 4, "Files", 24);
    
    // Buttons
    vga_fill_rounded_rect(win->x + win->w - 74, cy + 2, 30, 10, 22);
    font_draw_string(win->x + win->w - 70, cy + 4, "Open", 4);

    vga_fill_rounded_rect(win->x + win->w - 40, cy + 2, 30, 10, 35);
    font_draw_string(win->x + win->w - 36, cy + 4, "Del", 4);

    cy += 18;
    vga_fill_rect(cx, cy, win->w - 12, win->h - 16 - 22, 16);
    vga_draw_rect(cx, cy, win->w - 12, win->h - 16 - 22, 40);

    int mx = mouse_get_x();
    int my = mouse_get_y();
    uint8_t mb = mouse_get_buttons();

    for (int i = 0; i < file_count; i++) {
        int item_y = cy + 4 + i * 14;
        if (item_y + 14 > win->y + win->h) break;

        uint8_t hover = (mx >= cx && mx < cx + win->w - 12 && my >= item_y && my < item_y + 14);
        
        if (selected_file == i) {
            vga_fill_rect(cx + 2, item_y, win->w - 16, 12, 34);
        } else if (hover) {
            vga_fill_rect(cx + 2, item_y, win->w - 16, 12, 23);
        }

        if (hover && (mb & 1) && !files_click_handled) {
            selected_file = i;
            files_click_handled = 1;
        }

        font_draw_string(cx + 6, item_y + 2, cached_names[i], (selected_file == i) ? 15 : 4);
    }

    if (!(mb & 1)) {
        files_click_handled = 0;
    }
}
