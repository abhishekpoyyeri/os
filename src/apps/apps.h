/* src/apps/apps.h */
#ifndef APPS_H
#define APPS_H

#include <stdint.h>

/* Forward declare window_t from desktop.c */
typedef struct {
    int x, y, w, h;
    const char* title;
    uint8_t is_open;
    uint8_t is_focused;
    uint8_t bg_color;
    uint8_t app_id;
    uint32_t flags;
} window_t;

void clock_app_render(window_t* win);
void files_app_render(window_t* win);
void textedit_app_render(window_t* win);

void textedit_app_key(char c);

#endif
