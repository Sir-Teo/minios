#include "gdt.h"
#include <stdint.h>
#include <stddef.h>

// GDT entries (null, kernel code, kernel data, user code, user data, TSS x2)
// TSS takes 2 entries in x86_64
#define GDT_ENTRIES 7
static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gdt_pointer;
static struct tss_entry tss;

// External assembly function to load the GDT
extern void gdt_flush(uint64_t gdt_ptr);

// External assembly function to load the TSS
extern void tss_flush(uint16_t tss_selector);

// Set a GDT entry
static void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

// Set a TSS entry in the GDT (TSS is 16 bytes in x86_64, split across 2 GDT entries)
static void gdt_set_tss(int num, uint64_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    // First entry (lower 8 bytes)
    gdt[num].limit_low = limit & 0xFFFF;
    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].access = access;
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].base_high = (base >> 24) & 0xFF;

    // Second entry (upper 8 bytes) - x86_64 TSS is 16 bytes
    gdt[num + 1].limit_low = (base >> 32) & 0xFFFF;
    gdt[num + 1].base_low = (base >> 48) & 0xFFFF;
    gdt[num + 1].base_middle = 0;
    gdt[num + 1].access = 0;
    gdt[num + 1].granularity = 0;
    gdt[num + 1].base_high = 0;
}

void gdt_init(void) {
    // Zero out the TSS
    for (size_t i = 0; i < sizeof(struct tss_entry); i++) {
        ((uint8_t*)&tss)[i] = 0;
    }

    gdt_pointer.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdt_pointer.base = (uint64_t)&gdt;

    // Null descriptor (entry 0)
    gdt_set_entry(0, 0, 0, 0, 0);

    // Kernel code segment (entry 1)
    // Access: 0x9A = 10011010b = Present, Ring 0, Code, Executable, Readable
    // Granularity: 0xA0 = 10100000b = 64-bit, page granularity
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xA0);

    // Kernel data segment (entry 2)
    // Access: 0x92 = 10010010b = Present, Ring 0, Data, Writable
    // Granularity: 0xC0 = 11000000b = 32-bit, page granularity
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xC0);

    // User code segment (entry 3)
    // Access: 0xFA = 11111010b = Present, Ring 3, Code, Executable, Readable
    // Granularity: 0xA0 = 10100000b = 64-bit, page granularity
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0xA0);

    // User data segment (entry 4)
    // Access: 0xF2 = 11110010b = Present, Ring 3, Data, Writable
    // Granularity: 0xC0 = 11000000b = 32-bit, page granularity
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0xC0);

    // TSS (entries 5 and 6 - TSS is 16 bytes in x86_64)
    // Access: 0x89 = 10001001b = Present, Ring 0, TSS Available
    uint64_t tss_base = (uint64_t)&tss;
    uint32_t tss_limit = sizeof(struct tss_entry) - 1;
    gdt_set_tss(5, tss_base, tss_limit, 0x89, 0x00);

    // Load the new GDT
    gdt_flush((uint64_t)&gdt_pointer);

    // Load the TSS (selector = 5 * 8 = 0x28, with RPL=0)
    tss_flush(0x28);
}

void tss_set_stack(uint64_t stack) {
    tss.rsp0 = stack;
}
