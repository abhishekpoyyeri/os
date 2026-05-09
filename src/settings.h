/* src/settings.h */
#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

typedef struct {
    uint32_t version;
    uint8_t desktop_bg_color;
    uint8_t desktop_icon_color;
    uint8_t show_seconds;
} SystemSettings;

void settings_init(void);
void settings_load(void);
void settings_save(void);
SystemSettings* settings_get(void);

#endif
