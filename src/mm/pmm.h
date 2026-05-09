/* src/mm/pmm.h — Physical Memory Manager */
#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

/* Initialize the PMM with available memory */
void pmm_init(uint32_t mem_size_kb);

/* Allocate a single 4KB physical page, returns physical address */
uint32_t pmm_alloc_page(void);

/* Free a single 4KB physical page */
void pmm_free_page(uint32_t phys_addr);

/* Get total/used/free page counts */
uint32_t pmm_get_total_pages(void);
uint32_t pmm_get_used_pages(void);
uint32_t pmm_get_free_pages(void);

#endif
