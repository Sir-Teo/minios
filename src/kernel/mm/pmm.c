#include "pmm.h"
#include "../limine.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern void serial_write(const char *s);
extern void serial_write_dec(uint64_t value);
extern void serial_write_hex(uint64_t value);

// Request memory map and HHDM from Limine
extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;

// Bitmap to track free pages
static uint64_t *bitmap = NULL;
static size_t bitmap_size = 0;
static uint64_t total_pages = 0;
static uint64_t used_pages = 0;
static uint64_t hhdm_offset = 0;

// Convert physical address to higher half virtual address
static inline void *phys_to_virt(uint64_t phys) {
    return (void *)(phys + hhdm_offset);
}

// Set a bit in the bitmap (mark as used)
static inline void bitmap_set(size_t bit) {
    bitmap[bit / 64] |= (1UL << (bit % 64));
}

// Clear a bit in the bitmap (mark as free)
static inline void bitmap_clear(size_t bit) {
    bitmap[bit / 64] &= ~(1UL << (bit % 64));
}

// Test a bit in the bitmap
static inline bool bitmap_test(size_t bit) {
    return (bitmap[bit / 64] & (1UL << (bit % 64))) != 0;
}

void pmm_init(void) {
    serial_write("[PMM] Initializing physical memory manager...\n");

    if (!memmap_request.response || !hhdm_request.response) {
        serial_write("[PMM] ERROR: Required Limine responses not available\n");
        return;
    }

    hhdm_offset = hhdm_request.response->offset;
    
    struct limine_memmap_response *memmap = memmap_request.response;

    // Find the highest usable memory address
    uint64_t highest_addr = 0;
    uint64_t total_usable_memory = 0;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t entry_end = entry->base + entry->length;
            if (entry_end > highest_addr) {
                highest_addr = entry_end;
            }
            total_usable_memory += entry->length;
        }
    }

    total_pages = highest_addr / PAGE_SIZE;
    bitmap_size = (total_pages + 63) / 64;  // Round up to nearest 64-bit word

    serial_write("[PMM] Total memory pages: ");
    serial_write_dec(total_pages);
    serial_write("\n[PMM] Bitmap size: ");
    serial_write_dec(bitmap_size * 8);
    serial_write(" bytes\n");

    // Find a suitable place for the bitmap in usable memory
    bool bitmap_allocated = false;
    for (uint64_t i = 0; i < memmap->entry_count && !bitmap_allocated; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size * 8) {
            bitmap = (uint64_t *)phys_to_virt(entry->base);
            bitmap_allocated = true;
            serial_write("[PMM] Bitmap allocated at physical: ");
            serial_write_hex(entry->base);
            serial_write("\n");
        }
    }

    if (!bitmap_allocated) {
        serial_write("[PMM] ERROR: Could not allocate bitmap\n");
        return;
    }

    // Initialize bitmap - mark all as used initially
    for (size_t i = 0; i < bitmap_size; i++) {
        bitmap[i] = 0xFFFFFFFFFFFFFFFFUL;
    }
    used_pages = total_pages;

    // Mark usable memory as free
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t base_page = entry->base / PAGE_SIZE;
            uint64_t page_count = entry->length / PAGE_SIZE;
            
            for (uint64_t page = 0; page < page_count; page++) {
                uint64_t page_num = base_page + page;
                if (page_num < total_pages) {
                    bitmap_clear(page_num);
                    used_pages--;
                }
            }
        }
    }

    // Reserve the bitmap itself
    uint64_t bitmap_pages = (bitmap_size * 8 + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t bitmap_base_page = ((uint64_t)bitmap - hhdm_offset) / PAGE_SIZE;
    for (uint64_t i = 0; i < bitmap_pages; i++) {
        if (bitmap_base_page + i < total_pages) {
            bitmap_set(bitmap_base_page + i);
            used_pages++;
        }
    }

    serial_write("[PMM] Initialization complete\n");
    serial_write("[PMM] Total memory: ");
    serial_write_dec(total_usable_memory / 1024 / 1024);
    serial_write(" MiB\n[PMM] Free pages: ");
    serial_write_dec(total_pages - used_pages);
    serial_write("\n[PMM] Used pages: ");
    serial_write_dec(used_pages);
    serial_write("\n");
}

uint64_t pmm_alloc(void) {
    for (size_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_pages++;
            return i * PAGE_SIZE;
        }
    }
    return 0;  // Out of memory
}

void pmm_free(uint64_t addr) {
    uint64_t page = addr / PAGE_SIZE;
    if (page < total_pages && bitmap_test(page)) {
        bitmap_clear(page);
        used_pages--;
    }
}

uint64_t pmm_get_total_memory(void) {
    return total_pages * PAGE_SIZE;
}

uint64_t pmm_get_free_memory(void) {
    return (total_pages - used_pages) * PAGE_SIZE;
}
