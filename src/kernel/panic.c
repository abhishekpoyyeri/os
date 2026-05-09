/* src/kernel/panic.c */
#include "panic.h"
#include <stdint.h>

extern void terminal_writestring(const char* data);
extern void terminal_setcolor(uint8_t color);

void panic(const char* message) {
    /* Disable interrupts */
    asm volatile("cli");
    
    /* Display red screen of death */
    terminal_setcolor(0x4F); /* White text on Red background */
    terminal_writestring("\n\n*** KERNEL PANIC ***\n");
    terminal_writestring(message);
    terminal_writestring("\n\nSystem Halted. Please restart.\n");
    
    /* Halt the CPU safely */
    while (1) {
        asm volatile("hlt");
    }
}
