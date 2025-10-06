#ifndef KERNEL_MM_PMM_H
#define KERNEL_MM_PMM_H

#include <stdint.h>
#include <stddef.h>

// Page size (4 KiB)
#define PAGE_SIZE 4096

// Initialize physical memory manager
void pmm_init(void);

// Allocate a physical page frame
uint64_t pmm_alloc(void);

// Free a physical page frame
void pmm_free(uint64_t addr);

// Get total memory size
uint64_t pmm_get_total_memory(void);

// Get free memory size
uint64_t pmm_get_free_memory(void);

#endif
