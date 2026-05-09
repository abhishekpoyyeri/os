/* src/kernel/klog.c */
#include "klog.h"
#include "../drivers/io.h"
#include <stdint.h>

#define COM1_PORT 0x3F8

void klog_init(void) {
    outb(COM1_PORT + 1, 0x00);    /* Disable all interrupts */
    outb(COM1_PORT + 3, 0x80);    /* Enable DLAB (set baud rate divisor) */
    outb(COM1_PORT + 0, 0x03);    /* Set divisor to 3 (lo byte) 38400 baud */
    outb(COM1_PORT + 1, 0x00);    /*                  (hi byte) */
    outb(COM1_PORT + 3, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(COM1_PORT + 2, 0xC7);    /* Enable FIFO, clear them, with 14-byte threshold */
    outb(COM1_PORT + 4, 0x0B);    /* IRQs enabled, RTS/DSR set */
}

static void klog_write_char(char a) {
    /* Wait for transmit empty */
    while ((inb(COM1_PORT + 5) & 0x20) == 0);
    outb(COM1_PORT, (uint8_t)a);
}

static void klog_write_string(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        klog_write_char(str[i]);
    }
}

static void klog_write_u32(unsigned int value) {
    char buf[11];
    int i = 0;

    if (value == 0) {
        klog_write_char('0');
        return;
    }

    while (value > 0 && i < 10) {
        buf[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (i > 0) {
        klog_write_char(buf[--i]);
    }
}

static void klog_write_hex(unsigned int value) {
    static const char hex[] = "0123456789ABCDEF";
    klog_write_string("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        klog_write_char(hex[(value >> shift) & 0xF]);
    }
}

static void klog_write_labeled_value(const char* level, const char* label, unsigned int value, uint8_t hex) {
    klog_write_string(level);
    klog_write_string(label);
    klog_write_string("=");
    if (hex) {
        klog_write_hex(value);
    } else {
        klog_write_u32(value);
    }
    klog_write_string("\r\n");
}

void klog_info(const char* msg) {
    klog_write_string("[INFO] ");
    klog_write_string(msg);
    klog_write_string("\r\n");
}

void klog_warn(const char* msg) {
    klog_write_string("[WARN] ");
    klog_write_string(msg);
    klog_write_string("\r\n");
}

void klog_error(const char* msg) {
    klog_write_string("[ERROR] ");
    klog_write_string(msg);
    klog_write_string("\r\n");
}

void klog_info_u32(const char* label, unsigned int value) {
    klog_write_labeled_value("[INFO] ", label, value, 0);
}

void klog_info_hex(const char* label, unsigned int value) {
    klog_write_labeled_value("[INFO] ", label, value, 1);
}

void klog_warn_u32(const char* label, unsigned int value) {
    klog_write_labeled_value("[WARN] ", label, value, 0);
}

void klog_error_u32(const char* label, unsigned int value) {
    klog_write_labeled_value("[ERROR] ", label, value, 0);
}
