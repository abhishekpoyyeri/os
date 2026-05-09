/* src/drivers/keyboard.c */
#include "io.h"
#include "../kernel/klog.h"
#include <stdint.h>

extern void terminal_putchar(char c);

static const char keyboard_map[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',
  '9', '0', '-', '=', '\b',
  '\t',
  'q', 'w', 'e', 'r',
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
 '\'', '`',   0,
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',
  'm', ',', '.', '/',   0,
  '*',
    0,
  ' ',
    0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,
    0,
    0,
    0,
    0,
    0,
  '-',
    0,
    0,
    0,
  '+',
    0,
    0,
    0,
    0,
    0,
    0, 0, 0,
    0,
    0,
    0,
};

static const char keyboard_shift_map[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*',
  '(', ')', '_', '+', '\b',
  '\t',
  'Q', 'W', 'E', 'R',
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
 '"', '~',   0,
 '|', 'Z', 'X', 'C', 'V', 'B', 'N',
  'M', '<', '>', '?',   0,
  '*',
    0,
  ' ',
    0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,
    0,
    0,
    0,
    0,
    0,
  '-',
    0,
    0,
    0,
  '+',
    0,
    0,
    0,
    0,
    0,
    0, 0, 0,
    0,
    0,
    0,
};

static uint8_t lshift = 0, rshift = 0;
static uint8_t lctrl = 0, rctrl = 0;
static uint8_t lalt = 0, ralt = 0;

static uint8_t shift_pressed = 0;
static uint8_t ctrl_pressed = 0;
static uint8_t alt_pressed = 0;
static uint8_t capslock_enabled = 0; // Caps Lock toggle state
static uint8_t extended_scancode = 0;

extern void shell_input(char c);
extern volatile uint8_t gui_mode;
extern void desktop_key_input(char c);

uint8_t keyboard_shift_pressed(void) { return shift_pressed; }
uint8_t keyboard_ctrl_pressed(void) { return ctrl_pressed; }
uint8_t keyboard_alt_pressed(void) { return alt_pressed; }

static void set_modifier_state(uint8_t scancode, uint8_t pressed, uint8_t extended) {
    if (scancode == 0x2A) {
        lshift = pressed;
        klog_info_u32(pressed ? "keyboard lshift down scancode" : "keyboard lshift up scancode", scancode);
    } else if (scancode == 0x36) {
        rshift = pressed;
        klog_info_u32(pressed ? "keyboard rshift down scancode" : "keyboard rshift up scancode", scancode);
    } else if (scancode == 0x1D) {
        if (extended) rctrl = pressed;
        else lctrl = pressed;
        klog_info_u32(pressed ? "keyboard ctrl down scancode" : "keyboard ctrl up scancode", extended ? 0xE01D : 0x1D);
    } else if (scancode == 0x38) {
        if (extended) ralt = pressed;
        else lalt = pressed;
        klog_info_u32(pressed ? "keyboard alt down scancode" : "keyboard alt up scancode", extended ? 0xE038 : 0x38);
    }
    
    shift_pressed = lshift | rshift;
    ctrl_pressed = lctrl | rctrl;
    alt_pressed = lalt | ralt;
}

static char translate_scancode(uint8_t scancode) {
    if (scancode >= sizeof(keyboard_map)) return 0;
    char c;
    if (shift_pressed && scancode < sizeof(keyboard_shift_map)) {
        c = keyboard_shift_map[scancode];
    } else {
        c = keyboard_map[scancode];
    }
    // Apply Caps Lock XOR Shift for alphabetic characters
    if (capslock_enabled && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))){
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        } else {
            c = (char)(c - 'A' + 'a');
        }
    }
    return c;
}

void keyboard_handler() {
    uint8_t scancode = inb(0x60);

    if (scancode == 0xE0) {
        extended_scancode = 1;
        return;
    }

    uint8_t released = (scancode & 0x80) != 0;
    uint8_t base_scancode = scancode & 0x7F;
    uint8_t was_extended = extended_scancode;
    extended_scancode = 0;

    klog_info_u32(released ? "keyboard break scancode" : "keyboard make scancode",
                  was_extended ? (0xE000 | base_scancode) : base_scancode);

    if (base_scancode == 0x3A && !released) {
        capslock_enabled = !capslock_enabled;
        klog_info(capslock_enabled ? "CAPSLOCK ON" : "CAPSLOCK OFF");
        return;
    }
    if (base_scancode == 0x2A || base_scancode == 0x36 ||
        base_scancode == 0x1D || base_scancode == 0x38) {
        set_modifier_state(base_scancode, released ? 0 : 1, was_extended);
        return;
    }

    if (!released) {
        char c = translate_scancode(base_scancode);
        if (c == 0) return;

        if (gui_mode) {
            desktop_key_input(c);
        } else {
            terminal_putchar(c);
            shell_input(c);
        }
    }
}
