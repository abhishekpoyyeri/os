/* src/settings.c */
#include "settings.h"
#include "fs/vfs.h"
#include "kernel/klog.h"
#include <stddef.h>

static SystemSettings current_settings;

static void* local_memset(void* ptr, int value, size_t num) {
    uint8_t* p = (uint8_t*)ptr;
    while (num--) *p++ = (uint8_t)value;
    return ptr;
}

void settings_init(void) {
    local_memset(&current_settings, 0, sizeof(SystemSettings));
    current_settings.version = 1;
    current_settings.desktop_bg_color = 33;
    current_settings.desktop_icon_color = 42;
    current_settings.show_seconds = 1;
}

void settings_load(void) {
    if (!vfs_exists("settings.cfg")) {
        klog_info("Settings file not found, creating defaults.");
        settings_save();
        return;
    }
    
    int fd = vfs_open("settings.cfg");
    if (fd >= 0) {
        SystemSettings temp;
        int read_bytes = vfs_read(fd, (uint8_t*)&temp, sizeof(SystemSettings));
        vfs_close(fd);
        if (read_bytes == sizeof(SystemSettings) && temp.version == 1) {
            current_settings = temp;
            klog_info("Settings loaded successfully.");
        } else {
            klog_warn("Settings file invalid or old version, using defaults.");
        }
    }
}

void settings_save(void) {
    if (!vfs_exists("settings.cfg")) {
        vfs_create("settings.cfg", FILE_TYPE_CONFIG);
    }
    
    int fd = vfs_open("settings.cfg");
    if (fd >= 0) {
        vfs_write(fd, (const uint8_t*)&current_settings, sizeof(SystemSettings));
        vfs_truncate(fd, sizeof(SystemSettings));
        vfs_close(fd);
        klog_info("Settings saved.");
    } else {
        klog_error("Failed to open settings.cfg for saving.");
    }
}

SystemSettings* settings_get(void) {
    return &current_settings;
}
