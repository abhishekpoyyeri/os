/* src/drivers/timer.c — Programmable Interval Timer (PIT) */
#include "../drivers/io.h"
#include <stdint.h>

static volatile uint32_t tick_count = 0;

void timer_handler(void) {
    tick_count++;
}

uint32_t timer_get_ticks(void) {
    return tick_count;
}

void timer_init(uint32_t frequency) {
    /* The PIT oscillates at 1193180 Hz */
    uint32_t divisor = 1193180 / frequency;

    /* Send the command byte: channel 0, lobyte/hibyte, rate generator */
    outb(0x43, 0x36);

    /* Send the frequency divisor */
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}
