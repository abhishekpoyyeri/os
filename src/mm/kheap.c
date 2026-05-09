/* src/mm/kheap.c — Simple Kernel Heap (First-Fit Linked List) */
#include "kheap.h"
#include "pmm.h"

/* 
 * Simple heap built on top of PMM.
 * We pre-allocate a contiguous heap area and manage it with
 * a first-fit free list of blocks.
 */

/* Heap sits at a fixed virtual address range */
#define KHEAP_START  0x00400000   /* 4 MB */
#define KHEAP_INITIAL_SIZE (256 * 1024)  /* 256 KB initial heap */

typedef struct block_header {
    uint32_t size;       /* Size of the data area (not including header) */
    uint8_t  is_free;    /* 1 if free, 0 if allocated */
    struct block_header* next;
} block_header_t;

static block_header_t* heap_start = 0;
static uint32_t heap_end = 0;

/* memset implementation for the heap */
static void* memset_heap(void* ptr, int value, size_t num) {
    uint8_t* p = (uint8_t*)ptr;
    while (num--) {
        *p++ = (uint8_t)value;
    }
    return ptr;
}

void kheap_init(void) {
    /* For simplicity, we directly use the memory at KHEAP_START.
     * Since we're running without paging (identity mapped), we just
     * need to make sure this memory exists (which it does if we have >4MB RAM).
     * In a full OS, we'd map PMM pages to these virtual addresses. */
    
    heap_start = (block_header_t*)KHEAP_START;
    heap_end = KHEAP_START + KHEAP_INITIAL_SIZE;
    
    /* Create one big free block spanning the entire heap */
    heap_start->size = KHEAP_INITIAL_SIZE - sizeof(block_header_t);
    heap_start->is_free = 1;
    heap_start->next = 0;
}

void* kmalloc(size_t sz) {
    if (sz == 0) return 0;
    
    /* Align size to 4 bytes */
    sz = (sz + 3) & ~3;
    
    block_header_t* curr = heap_start;
    
    while (curr) {
        if (curr->is_free && curr->size >= sz) {
            /* Found a big enough free block */
            
            /* Split the block if there's enough room for another block */
            if (curr->size > sz + sizeof(block_header_t) + 8) {
                block_header_t* new_block = (block_header_t*)((uint8_t*)curr + sizeof(block_header_t) + sz);
                new_block->size = curr->size - sz - sizeof(block_header_t);
                new_block->is_free = 1;
                new_block->next = curr->next;
                
                curr->size = sz;
                curr->next = new_block;
            }
            
            curr->is_free = 0;
            void* ret = (void*)((uint8_t*)curr + sizeof(block_header_t));
            memset_heap(ret, 0xAA, sz);
            return ret;
        }
        curr = curr->next;
    }
    
    return 0;  /* Out of heap memory */
}

void* kmalloc_aligned(size_t sz) {
    /* Simple page-aligned allocation — waste some space */
    size_t total = sz + PAGE_SIZE;
    void* ptr = kmalloc(total);
    if (!ptr) return 0;
    
    /* Align up to next page boundary */
    uint32_t aligned = ((uint32_t)ptr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    return (void*)aligned;
}

void kfree(void* ptr) {
    if (!ptr) return;
    
    block_header_t* header = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    memset_heap(ptr, 0xDD, header->size);
    header->is_free = 1;
    
    /* Coalesce adjacent free blocks */
    block_header_t* curr = heap_start;
    while (curr) {
        if (curr->is_free && curr->next && curr->next->is_free) {
            curr->size += sizeof(block_header_t) + curr->next->size;
            curr->next = curr->next->next;
            continue;  /* Check again in case of triple merge */
        }
        curr = curr->next;
    }
}
