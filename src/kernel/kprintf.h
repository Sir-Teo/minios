#ifndef KPRINTF_H
#define KPRINTF_H

#include <stdint.h>
#include <stdarg.h>

// Kernel printf function
void kprintf(const char *fmt, ...);

// Helper functions for printing numbers
void serial_write_dec(uint64_t value);
void serial_write_hex(uint64_t value);

#endif // KPRINTF_H
