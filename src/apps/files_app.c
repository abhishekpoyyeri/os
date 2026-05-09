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

/* textedit_load_external is a new hook in textedit_app.c that receives a
   pre-loaded buffer. Declared here; defined in textedit_app.c. */
extern void textedit_load_external(const char* buf, int len);

#define FILES_READ_BUF 8192

static int selected_file = -1;
static uint8_t files_click_handled = 0;

/* Status message shown after open/delete */
static const char* status_msg  = 0;
static uint8_t     status_color = 4; /* white */

/* Cached file list from vfs_list */
static const char* cached_names[MYFS_MAX_FILES];
static uint32_t    cached_sizes[MYFS_MAX_FILES];
static uint8_t     cached_types[MYFS_MAX_FILES];
static int         file_count = 0;

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

/* Read named file into textedit */
static void do_open_file(void) {
    if (selected_file < 0 || selected_file >= file_count) {
        klog_warn("Files: Open clicked but no file selected");
        status_msg   = "No file selected";
        status_color = 35; /* orange */
        return;
    }

    const char* name = cached_names[selected_file];
    klog_info("Files: opening file");

    int fd = vfs_open(name);
    if (fd < 0) {
        klog_error("Files: vfs_open failed");
        status_msg   = "FILE NOT FOUND";
        status_color = 30;
        return;
    }

    uint32_t size = vfs_get_size(fd);
    klog_info_u32("Files: file size", size);
    if (size == 0) {
        vfs_close(fd);
        klog_warn("Files: file is empty");
        status_msg   = "File is empty";
        status_color = 24;
        textedit_load_external("", 0);
        return;
    }

    /* Use a static buffer to avoid stack overflow */
    static char read_buf[FILES_READ_BUF];
    if (size >= FILES_READ_BUF) size = FILES_READ_BUF - 1;

    int bytes = vfs_read(fd, (uint8_t*)read_buf, size);
    vfs_close(fd);

    if (bytes <= 0) {
        klog_error("Files: vfs_read returned 0 or error");
        status_msg   = "READ ERROR";
        status_color = 30;
        return;
    }
    read_buf[bytes] = '\0';
    klog_info_u32("Files: bytes read", (uint32_t)bytes);

    textedit_load_external(read_buf, bytes);
    status_msg   = "File opened in Notes";
    status_color = 8; /* green */
    klog_info("Files: file loaded into text editor");
}

void files_app_render(window_t* win) {
    int cx = win->x + 6;
    int cy = win->y + 20;

    refresh_files();

    /* Header */
    vga_fill_rounded_rect(cx, cy, win->w - 12, 14, 38);
    font_draw_string(cx + 6, cy + 4, "Files", 24);

    /* ---- Buttons ---- */
    int32_t mx = mouse_get_x();
    int32_t my = mouse_get_y();
    uint8_t mb = mouse_get_buttons();

    /* Open button */
    int open_bx = win->x + win->w - 74;
    int open_by = cy + 2;
    uint8_t open_hover = (mx >= open_bx && mx < open_bx + 30 &&
                          my >= open_by && my < open_by + 10);
    vga_fill_rounded_rect(open_bx, open_by, 30, 10,
                          open_hover ? 19 : (selected_file >= 0 ? 34 : 22));
    font_draw_string(open_bx + 4, open_by + 2, "Open", 4);

    if (open_hover && (mb & 1) && !files_click_handled) {
        files_click_handled = 1;
        klog_info("Files: Open button clicked");
        do_open_file();
    }

    /* Del button */
    int del_bx = win->x + win->w - 40;
    int del_by = cy + 2;
    uint8_t del_hover = (mx >= del_bx && mx < del_bx + 30 &&
                         my >= del_by && my < del_by + 10);
    vga_fill_rounded_rect(del_bx, del_by, 30, 10, del_hover ? 30 : 35);
    font_draw_string(del_bx + 4, del_by + 2, "Del", 4);

    if (del_hover && (mb & 1) && !files_click_handled) {
        files_click_handled = 1;
        if (selected_file >= 0 && selected_file < file_count) {
            klog_info("Files: Delete clicked");
            vfs_delete(cached_names[selected_file]);
            selected_file = -1;
            status_msg   = "File deleted";
            status_color = 35;
        }
    }

    if (!(mb & 1)) files_click_handled = 0;

    cy += 18;

    /* File list area */
    int list_h = win->h - 16 - 22 - 14; /* reserve bottom for status */
    vga_fill_rect(cx, cy, win->w - 12, list_h, 16);
    vga_draw_rect(cx, cy, win->w - 12, list_h, 40);

    if (file_count == 0) {
        font_draw_string(cx + 6, cy + 5, "No files found", 41);
    }

    for (int i = 0; i < file_count; i++) {
        int item_y = cy + 4 + i * 14;
        if (item_y + 14 > cy + list_h) break;

        uint8_t hover = (mx >= cx && mx < cx + win->w - 12 &&
                         my >= item_y && my < item_y + 14);

        if (selected_file == i)
            vga_fill_rect(cx + 2, item_y, win->w - 16, 12, 34);
        else if (hover)
            vga_fill_rect(cx + 2, item_y, win->w - 16, 12, 23);

        if (hover && (mb & 1) && !files_click_handled) {
            selected_file = i;
            files_click_handled = 1;
            klog_info_u32("Files: selected file index", (uint32_t)i);
            status_msg = 0; /* clear old status */
        }

        font_draw_string(cx + 6, item_y + 2,
                         cached_names[i],
                         (selected_file == i) ? 15 : 4);
    }

    /* Status bar */
    int sb_y = win->y + win->h - 12;
    if (status_msg) {
        font_draw_string(cx + 4, sb_y, status_msg, status_color);
    } else if (selected_file >= 0 && selected_file < file_count) {
        /* Show selected filename */
        font_draw_string(cx + 4, sb_y, cached_names[selected_file], 24);
    }
}
