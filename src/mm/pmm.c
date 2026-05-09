/* src/mm/pmm.c — Physical Memory Manager (Bitmap Allocator) */
#include "pmm.h"

/* We support up to 128 MB of RAM = 32768 pages.
 * Each bit in the bitmap represents one 4KB page.
 * 32768 bits = 4096 bytes = 1024 uint32_t entries. */
#define MAX_PAGES  32768
#define BITMAP_SIZE (MAX_PAGES / 32)

static uint32_t bitmap[BITMAP_SIZE];
static uint32_t total_pages = 0;
static uint32_t used_pages = 0;

/* Mark a page as used */
static void pmm_set(uint32_t page) {
    bitmap[page / 32] |= (1 << (page % 32));
}

/* Mark a page as free */
static void pmm_clear(uint32_t page) {
    bitmap[page / 32] &= ~(1 << (page % 32));
}

/* Test if a page is used */
static int pmm_test(uint32_t page) {
    return bitmap[page / 32] & (1 << (page % 32));
}

void pmm_init(uint32_t mem_size_kb) {
    total_pages = mem_size_kb / 4;  /* Each page is 4KB */
    if (total_pages > MAX_PAGES)
        total_pages = MAX_PAGES;

    /* Mark ALL pages as used initially */
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        bitmap[i] = 0xFFFFFFFF;
    }
    used_pages = total_pages;

    /* Free usable pages after the reserved low memory and kernel area.
     * Assume the kernel ends at 2MB for safety. */
    uint32_t kernel_end_page = 0x200000 / PAGE_SIZE;  /* Page 512 = 2MB */

    for (uint32_t i = kernel_end_page; i < total_pages; i++) {
        pmm_clear(i);
        used_pages--;
    }
}

uint32_t pmm_alloc_page(void) {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        if (bitmap[i] != 0xFFFFFFFF) {
            /* There's a free bit in this entry */
            for (int bit = 0; bit < 32; bit++) {
                if (!(bitmap[i] & (1 << bit))) {
                    uint32_t page = i * 32 + bit;
                    if (page >= total_pages) return 0;  /* Out of memory */
                    pmm_set(page);
                    used_pages++;
                    return page * PAGE_SIZE;
                }
            }
        }
    }
    return 0;  /* Out of memory */
}

void pmm_free_page(uint32_t phys_addr) {
    uint32_t page = phys_addr / PAGE_SIZE;
    if (page < total_pages && pmm_test(page)) {
        pmm_clear(page);
        used_pages--;
    }
}

uint32_t pmm_get_total_pages(void) { return total_pages; }
uint32_t pmm_get_used_pages(void)  { return used_pages; }
uint32_t pmm_get_free_pages(void)  { return total_pages - used_pages; }
