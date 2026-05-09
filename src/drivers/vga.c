/* src/drivers/vga.c — VGA Mode 13h Graphics Driver (320x200, 256 colors) */
#include "io.h"
#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH  320
#define VGA_HEIGHT 200
#define VGA_FB     ((uint8_t*)0xA0000)

/* Double buffer to avoid flicker */
static uint8_t backbuffer[VGA_WIDTH * VGA_HEIGHT];

/* ===== VGA Register Programming ===== */

static void vga_write_regs(uint8_t *regs);

/* Mode 13h register dump (320x200x256) */
static uint8_t mode_13h[] = {
    /* MISC */
    0x63,
    /* SEQ */
    0x03, 0x01, 0x0F, 0x00, 0x0E,
    /* CRTC */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
    0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
    0xFF,
    /* GC */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
    0xFF,
    /* AC */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x41, 0x00, 0x0F, 0x00, 0x00
};

static void vga_write_regs(uint8_t *regs) {
    /* Write MISC register */
    outb(0x3C2, *regs);
    regs++;

    /* Write SEQ registers */
    for (uint8_t i = 0; i < 5; i++) {
        outb(0x3C4, i);
        outb(0x3C5, *regs);
        regs++;
    }

    /* Unlock CRTC registers */
    outb(0x3D4, 0x03);
    outb(0x3D5, inb(0x3D5) | 0x80);
    outb(0x3D4, 0x11);
    outb(0x3D5, inb(0x3D5) & ~0x80);

    /* Write CRTC registers */
    for (uint8_t i = 0; i < 25; i++) {
        outb(0x3D4, i);
        outb(0x3D5, *regs);
        regs++;
    }

    /* Write GC registers */
    for (uint8_t i = 0; i < 9; i++) {
        outb(0x3CE, i);
        outb(0x3CF, *regs);
        regs++;
    }

    /* Write AC registers */
    for (uint8_t i = 0; i < 21; i++) {
        inb(0x3DA);  /* Reset flip-flop */
        outb(0x3C0, i);
        outb(0x3C0, *regs);
        regs++;
    }

    /* Lock palette and unblank display */
    inb(0x3DA);
    outb(0x3C0, 0x20);
}

/* ===== Color Palette ===== */

/* Set a single palette entry (index 0-255, r/g/b are 0-63) */
void vga_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(0x3C8, index);
    outb(0x3C9, r);
    outb(0x3C9, g);
    outb(0x3C9, b);
}

/*
 * Windows 11 Fluent Design inspired palette.
 * Mica-style soft backgrounds, clean whites, WinUI accent blue.
 */
static void vga_init_palette(void) {
    /* Index  0: Desktop wallpaper base — dark navy mica */
    vga_set_palette(0,  6,  8, 18);
    /* Index  1: Title bar (focused) — Windows blue accent */
    vga_set_palette(1, 12, 24, 48);
    /* Index  2: Button hover / highlight — lighter accent */
    vga_set_palette(2, 18, 32, 52);
    /* Index  3: Light accent / desktop text */
    vga_set_palette(3, 28, 40, 56);
    /* Index  4: White — primary text */
    vga_set_palette(4, 63, 63, 63);
    /* Index  5: Window chrome — light surface (Mica light) */
    vga_set_palette(5, 56, 57, 58);
    /* Index  6: Border / separator — subtle gray */
    vga_set_palette(6, 42, 42, 44);
    /* Index  7: Window background — slightly off-white surface */
    vga_set_palette(7, 50, 51, 52);
    /* Index  8: Green — success indicator */
    vga_set_palette(8, 14, 52, 24);
    /* Index  9: Red — close button */
    vga_set_palette(9, 54, 10, 10);
    /* Index 10: Yellow — warning */
    vga_set_palette(10, 60, 52, 10);
    /* Index 11: Cyan / teal — accent */
    vga_set_palette(11, 10, 44, 52);
    /* Index 12: Taskbar background — frosted dark */
    vga_set_palette(12, 10, 11, 16);
    /* Index 13: Taskbar darker — bottom edge */
    vga_set_palette(13,  7,  8, 12);
    /* Index 14: Bright white (same as 4 for compat) */
    vga_set_palette(14, 63, 63, 63);
    /* Index 15: Cursor — white */
    vga_set_palette(15, 63, 63, 63);

    /* === Extended palette for modern UI === */
    /* Index 16: Calc display bg — dark charcoal */
    vga_set_palette(16, 12, 13, 16);
    /* Index 17: Calc number btn — neutral dark surface */
    vga_set_palette(17, 22, 23, 26);
    /* Index 18: Calc operator btn — muted blue */
    vga_set_palette(18, 16, 28, 44);
    /* Index 19: Calc equals btn — Windows accent blue */
    vga_set_palette(19, 12, 30, 56);
    /* Index 20: Close btn hover — bright red */
    vga_set_palette(20, 60, 14, 14);
    /* Index 21: Button press effect — darkened */
    vga_set_palette(21, 8, 16, 36);
    /* Index 22: Menu bg — dark frosted glass */
    vga_set_palette(22, 14, 15, 20);
    /* Index 23: Menu hover — subtle highlight */
    vga_set_palette(23, 20, 22, 30);
    /* Index 24: Text secondary — medium gray */
    vga_set_palette(24, 36, 36, 40);
    /* Index 25: Window shadow color */
    vga_set_palette(25,  3,  3,  6);
    /* Index 26: Desktop wallpaper gradient mid */
    vga_set_palette(26,  8, 12, 24);
    /* Index 27: Desktop wallpaper gradient top */
    vga_set_palette(27, 10, 16, 30);
    /* Index 28: Taskbar app active — accent glow */
    vga_set_palette(28, 14, 28, 48);
    /* Index 29: Title bar unfocused — muted */
    vga_set_palette(29, 18, 19, 24);
    /* Index 30: Calc clear btn — orange accent */
    vga_set_palette(30, 50, 28, 10);
    /* Index 31: Rounded corner anti-alias — blend dark */
    vga_set_palette(31,  4,  5, 10);

    /* === Aurora desktop accents === */
    /* Index 32: Deep ink background */
    vga_set_palette(32,  5,  7, 14);
    /* Index 33: Violet shadow band */
    vga_set_palette(33, 13,  9, 24);
    /* Index 34: Teal glass band */
    vga_set_palette(34,  7, 31, 35);
    /* Index 35: Emerald accent */
    vga_set_palette(35, 10, 48, 31);
    /* Index 36: Warm amber accent */
    vga_set_palette(36, 55, 36, 11);
    /* Index 37: Soft rose accent */
    vga_set_palette(37, 49, 15, 27);
    /* Index 38: Elevated panel */
    vga_set_palette(38, 16, 18, 25);
    /* Index 39: Panel top highlight */
    vga_set_palette(39, 24, 27, 37);
    /* Index 40: Fine divider */
    vga_set_palette(40, 29, 32, 42);
    /* Index 41: Dim text */
    vga_set_palette(41, 31, 34, 43);
    /* Index 42: Icon tile */
    vga_set_palette(42, 20, 22, 30);
    /* Index 43: Icon tile hover */
    vga_set_palette(43, 28, 31, 41);
}

/* ===== Drawing Primitives ===== */

void vga_init(void) {
    vga_write_regs(mode_13h);
    vga_init_palette();
    
    /* Clear backbuffer */
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        backbuffer[i] = 0;
    }
}

void vga_putpixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        backbuffer[y * VGA_WIDTH + x] = color;
    }
}

uint8_t vga_getpixel(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        return backbuffer[y * VGA_WIDTH + x];
    }
    return 0;
}

void vga_fill_rect(int x, int y, int w, int h, uint8_t color) {
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            vga_putpixel(i, j, color);
        }
    }
}

void vga_draw_rect(int x, int y, int w, int h, uint8_t color) {
    /* Top and bottom edges */
    for (int i = x; i < x + w; i++) {
        vga_putpixel(i, y, color);
        vga_putpixel(i, y + h - 1, color);
    }
    /* Left and right edges */
    for (int j = y; j < y + h; j++) {
        vga_putpixel(x, j, color);
        vga_putpixel(x + w - 1, j, color);
    }
}

/* Draw a rectangle with simulated rounded corners (2px radius) */
void vga_fill_rounded_rect(int x, int y, int w, int h, uint8_t color) {
    /* Fill center area */
    vga_fill_rect(x + 2, y, w - 4, h, color);
    /* Fill left/right strips minus corners */
    vga_fill_rect(x, y + 2, 2, h - 4, color);
    vga_fill_rect(x + w - 2, y + 2, 2, h - 4, color);
    /* Corner pixels for rounded effect */
    vga_putpixel(x + 1, y + 1, color);
    vga_putpixel(x + w - 2, y + 1, color);
    vga_putpixel(x + 1, y + h - 2, color);
    vga_putpixel(x + w - 2, y + h - 2, color);
    /* Fill remaining corner fill */
    vga_putpixel(x + 1, y, color);
    vga_putpixel(x + w - 2, y, color);
    vga_putpixel(x + 1, y + h - 1, color);
    vga_putpixel(x + w - 2, y + h - 1, color);
    vga_putpixel(x, y + 1, color);
    vga_putpixel(x + w - 1, y + 1, color);
    vga_putpixel(x, y + h - 2, color);
    vga_putpixel(x + w - 1, y + h - 2, color);
}

void vga_draw_hline(int x, int y, int len, uint8_t color) {
    for (int i = x; i < x + len; i++) {
        vga_putpixel(i, y, color);
    }
}

void vga_draw_vline(int x, int y, int len, uint8_t color) {
    for (int j = y; j < y + len; j++) {
        vga_putpixel(x, j, color);
    }
}

void vga_clear(uint8_t color) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        backbuffer[i] = color;
    }
}

void vga_swap(void) {
    /* Copy backbuffer to VGA framebuffer */
    uint8_t* fb = VGA_FB;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        fb[i] = backbuffer[i];
    }
}

int vga_get_width(void)  { return VGA_WIDTH; }
int vga_get_height(void) { return VGA_HEIGHT; }
