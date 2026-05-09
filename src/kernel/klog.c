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
