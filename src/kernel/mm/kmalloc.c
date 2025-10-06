#include "kmalloc.h"
#include "pmm.h"
#include "../limine.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern void *memset(void *d, int c, size_t n);
extern void serial_write(const char *s);
extern volatile struct limine_hhdm_request hhdm_request;

// Simple bump allocator for now (can be improved to a real heap later)
#define HEAP_SIZE (16 * 1024 * 1024)  // 16 MiB heap

static uint8_t *heap_start = NULL;
static uint8_t *heap_current = NULL;
static uint8_t *heap_end = NULL;

void kmalloc_init(void) {
    serial_write("[KMALLOC] Initializing kernel heap...\n");

    // Allocate pages for heap
    size_t heap_pages = HEAP_SIZE / PAGE_SIZE;
    uint64_t phys_addr = pmm_alloc();
    
    if (phys_addr == 0) {
        serial_write("[KMALLOC] ERROR: Failed to allocate heap\n");
        return;
    }

    // For simplicity, allocate contiguous pages
    // In a real implementation, we'd use virtual memory to map non-contiguous pages
    uint64_t hhdm_offset = hhdm_request.response->offset;
    heap_start = (uint8_t *)(phys_addr + hhdm_offset);
    heap_current = heap_start;
    heap_end = heap_start + PAGE_SIZE;  // Start with one page

    // Allocate remaining pages
    for (size_t i = 1; i < heap_pages; i++) {
        uint64_t page = pmm_alloc();
        if (page == 0) break;
        heap_end += PAGE_SIZE;
    }

    // Clear the heap
    memset(heap_start, 0, heap_end - heap_start);

    serial_write("[KMALLOC] Heap initialized (simple bump allocator)\n");
}

void *kmalloc(size_t size) {
    if (heap_start == NULL) {
        return NULL;
    }

    // Align to 16 bytes
    size = (size + 15) & ~15;

    if (heap_current + size > heap_end) {
        return NULL;  // Out of heap memory
    }

    void *ptr = heap_current;
    heap_current += size;
    return ptr;
}

void kfree(void *ptr) {
    // Bump allocator doesn't support freeing individual blocks
    // In a real implementation, we'd use a proper heap allocator
    (void)ptr;
}
