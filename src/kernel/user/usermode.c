#include "usermode.h"
#include "kprintf.h"
#include "sched/task.h"
#include <stddef.h>

// External functions
extern void *kmalloc(uint64_t size);
extern void kfree(void *ptr);
extern bool vmm_map_page(void *address_space, uint64_t virt, uint64_t phys, uint64_t flags);
extern uint64_t pmm_alloc(void);

bool is_usermode_address(uint64_t addr) {
    // User space: 0x0000000000000000 - 0x00007FFFFFFFFFFF
    return (addr < 0x0000800000000000);
}

bool is_kernelmode_address(uint64_t addr) {
    // Kernel space: 0xFFFF800000000000 - 0xFFFFFFFFFFFFFFFF
    return (addr >= 0xFFFF800000000000);
}

void usermode_init(void) {
    kprintf("[USER] Initializing user mode subsystem...\n");
    kprintf("[USER] User space: 0x0000000000000000 - 0x00007FFFFFFFFFFF\n");
    kprintf("[USER] Kernel space: 0xFFFF800000000000 - 0xFFFFFFFFFFFFFFFF\n");
    kprintf("[USER] User stack size: %d KB\n", USER_STACK_SIZE / 1024);
    kprintf("[USER] User code base: %p\n", (void *)USER_CODE_BASE);
    kprintf("[USER] User mode subsystem initialized\n");
}

bool setup_user_memory(void *address_space, uint64_t code_start,
                       uint64_t code_size, uint64_t stack_top) {
    if (!address_space) {
        kprintf("[USER] ERROR: Invalid address space\n");
        return false;
    }

    // Map user code pages (read-only for now)
    uint64_t num_code_pages = (code_size + 4095) / 4096;
    for (uint64_t i = 0; i < num_code_pages; i++) {
        uint64_t phys = pmm_alloc();
        if (phys == 0) {
            kprintf("[USER] ERROR: Failed to allocate code page %llu\n", i);
            return false;
        }

        uint64_t virt = code_start + (i * 4096);
        if (!vmm_map_page(address_space, virt, phys, PAGE_USER_RW)) {
            kprintf("[USER] ERROR: Failed to map code page at %p\n", (void *)virt);
            return false;
        }
    }

    // Map user stack pages (read-write)
    uint64_t num_stack_pages = USER_STACK_SIZE / 4096;
    uint64_t stack_base = stack_top - USER_STACK_SIZE;

    for (uint64_t i = 0; i < num_stack_pages; i++) {
        uint64_t phys = pmm_alloc();
        if (phys == 0) {
            kprintf("[USER] ERROR: Failed to allocate stack page %llu\n", i);
            return false;
        }

        uint64_t virt = stack_base + (i * 4096);
        if (!vmm_map_page(address_space, virt, phys, PAGE_USER_RW)) {
            kprintf("[USER] ERROR: Failed to map stack page at %p\n", (void *)virt);
            return false;
        }
    }

    kprintf("[USER] User memory setup complete:\n");
    kprintf("[USER]   Code: %p - %p (%llu pages)\n",
            (void *)code_start, (void *)(code_start + code_size), num_code_pages);
    kprintf("[USER]   Stack: %p - %p (%llu pages)\n",
            (void *)stack_base, (void *)stack_top, num_stack_pages);

    return true;
}

// This will be implemented in assembly
extern void usermode_entry(uint64_t entry, uint64_t user_stack, uint64_t user_cs, uint64_t user_ss);

void enter_usermode(uint64_t entry, uint64_t user_stack_top) {
    kprintf("[USER] Entering user mode at %p with stack %p\n",
            (void *)entry, (void *)user_stack_top);

    // Segment selectors
    // User code segment: 0x18 | 3 (RPL=3)
    // User data segment: 0x20 | 3 (RPL=3)
    uint64_t user_cs = 0x18 | 3;  // GDT entry 3, RPL 3
    uint64_t user_ss = 0x20 | 3;  // GDT entry 4, RPL 3

    // Call assembly routine to perform the transition
    usermode_entry(entry, user_stack_top, user_cs, user_ss);

    // Should never return
    kprintf("[USER] ERROR: Returned from user mode!\n");
}
