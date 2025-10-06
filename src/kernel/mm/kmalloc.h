#ifndef KERNEL_MM_KMALLOC_H
#define KERNEL_MM_KMALLOC_H

#include <stddef.h>

// Initialize kernel heap
void kmalloc_init(void);

// Allocate memory from kernel heap
void *kmalloc(size_t size);

// Free memory from kernel heap
void kfree(void *ptr);

#endif
