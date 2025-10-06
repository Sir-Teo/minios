#ifndef USERMODE_H
#define USERMODE_H

#include <stdint.h>
#include <stdbool.h>

// User mode memory layout
#define USER_STACK_SIZE     (16 * 1024)     // 16 KB user stack
#define USER_STACK_TOP      0x00007FFFFFFFFFFF  // Top of user space
#define USER_CODE_BASE      0x0000000000400000  // 4 MB (standard ELF base)

// Page flags for user mode
#define PAGE_USER_RW        0x07  // Present, R/W, User
#define PAGE_USER_RO        0x05  // Present, R/O, User

// Check if address is in user space
bool is_usermode_address(uint64_t addr);

// Check if address is in kernel space
bool is_kernelmode_address(uint64_t addr);

// Initialize user mode subsystem
void usermode_init(void);

// Enter user mode at given entry point with user stack
void enter_usermode(uint64_t entry, uint64_t user_stack_top);

// Setup user mode memory mapping for a task
bool setup_user_memory(void *address_space, uint64_t code_start,
                       uint64_t code_size, uint64_t stack_top);

#endif // USERMODE_H
