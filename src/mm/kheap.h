/* src/mm/kheap.h — Kernel Heap Allocator */
#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>

/* Initialize the kernel heap */
void kheap_init(void);

/* Allocate sz bytes from the kernel heap */
void* kmalloc(size_t sz);

/* Allocate sz bytes, page-aligned */
void* kmalloc_aligned(size_t sz);

/* Free a previously allocated block */
void kfree(void* ptr);

#endif
