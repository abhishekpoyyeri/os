/* src/drivers/mouse.c — PS/2 Mouse Driver */
#include "io.h"
#include <stdint.h>

/* Mouse state */
static int32_t mouse_x = 160;  /* Start at center (320/2) */
static int32_t mouse_y = 100;  /* Start at center (200/2) */
static uint8_t mouse_buttons = 0;
static uint8_t mouse_cycle = 0;
static int8_t  mouse_bytes[3];

/* Screen bounds (will be set by VGA driver) */
static int32_t screen_width = 320;
static int32_t screen_height = 200;

/* Wait for the PS/2 controller to be ready */
static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        /* Wait for output buffer (data available to read) */
        while (timeout--) {
            if (inb(0x64) & 1) return;
        }
    } else {
        /* Wait for input buffer (ready to accept command) */
        while (timeout--) {
            if (!(inb(0x64) & 2)) return;
        }
    }
}

static void mouse_write(uint8_t data) {
    mouse_wait(1);
    outb(0x64, 0xD4);  /* Tell controller we're sending to mouse */
    mouse_wait(1);
    outb(0x60, data);
}

static uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_handler(void) {
    uint8_t status = inb(0x64);
    if (!(status & 0x20)) return;  /* Not a mouse packet */
    
    int8_t data = (int8_t)inb(0x60);
    
    switch (mouse_cycle) {
    case 0:
        mouse_bytes[0] = data;
        if (data & 0x08) {  /* Bit 3 must always be set in byte 0 */
            mouse_cycle = 1;
        }
        break;
    case 1:
        mouse_bytes[1] = data;
        mouse_cycle = 2;
        break;
    case 2:
        mouse_bytes[2] = data;
        mouse_cycle = 0;
        
        /* Process complete packet */
        mouse_buttons = mouse_bytes[0] & 0x07;
        
        /* Apply movement deltas */
        mouse_x += mouse_bytes[1];
        mouse_y -= mouse_bytes[2];  /* Y is inverted in PS/2 */
        
        /* Clamp to screen bounds */
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_x >= screen_width) mouse_x = screen_width - 1;
        if (mouse_y >= screen_height) mouse_y = screen_height - 1;
        break;
    }
}

void mouse_init(void) {
    /* Enable the auxiliary (mouse) device */
    mouse_wait(1);
    outb(0x64, 0xA8);
    
    /* Enable interrupts for the mouse */
    mouse_wait(1);
    outb(0x64, 0x20);  /* Get Compaq status byte */
    mouse_wait(0);
    uint8_t status = inb(0x60);
    status |= 2;       /* Enable IRQ12 */
    status &= ~0x20;   /* Enable mouse clock */
    mouse_wait(1);
    outb(0x64, 0x60);  /* Set Compaq status byte */
    mouse_wait(1);
    outb(0x60, status);
    
    /* Tell mouse to use default settings */
    mouse_write(0xF6);
    mouse_read();  /* Acknowledge */
    
    /* Enable the mouse (start sending packets) */
    mouse_write(0xF4);
    mouse_read();  /* Acknowledge */
}

int32_t mouse_get_x(void) { return mouse_x; }
int32_t mouse_get_y(void) { return mouse_y; }
uint8_t mouse_get_buttons(void) { return mouse_buttons; }

void mouse_set_bounds(int32_t w, int32_t h) {
    screen_width = w;
    screen_height = h;
    if (mouse_x >= w) mouse_x = w - 1;
    if (mouse_y >= h) mouse_y = h - 1;
}
