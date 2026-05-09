/* src/drivers/io.h */
#ifndef IO_H
#define IO_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}
static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline void outsw(uint16_t port, const void *buffer, uint32_t word_count) {
    asm volatile("rep outsw" : "+S"(buffer), "+c"(word_count) : "d"(port));
}

static inline void insw(uint16_t port, void *buffer, uint32_t word_count) {
    asm volatile("rep insw" : "+D"(buffer), "+c"(word_count) : "d"(port) : "memory");
}

static inline void outsl(uint16_t port, const uint32_t *buffer, uint32_t quads) {
    asm volatile("rep outsl" : "+S"(buffer), "+c"(quads) : "d"(port));
}

static inline void insl(uint16_t port, uint32_t *buffer, uint32_t quads) {
    asm volatile("rep insl" : "+D"(buffer), "+c"(quads) : "d"(port) : "memory");
}

#endif
