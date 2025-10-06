#include "task.h"
#include <stdint.h>
#include <stddef.h>

// External functions
extern void serial_write(const char *s);
extern void *kmalloc(uint64_t size);
extern void kfree(void *ptr);
extern void *memset(void *s, int c, uint64_t n);

// Kernel stack size (16 KB per task)
#define TASK_KERNEL_STACK_SIZE 16384

// PID counter
static uint64_t next_pid = 1;

// Current running task
static task_t *current_task = NULL;

void task_init(void) {
    serial_write("[TASK] Initializing task subsystem...\n");
    next_pid = 1;
    current_task = NULL;
    serial_write("[TASK] Task subsystem initialized\n");
}

task_t *task_create(void (*entry)(void), uint64_t priority) {
    // Allocate task structure
    task_t *task = (task_t *)kmalloc(sizeof(task_t));
    if (!task) {
        serial_write("[TASK] ERROR: Failed to allocate task structure\n");
        return NULL;
    }

    // Clear task structure
    memset(task, 0, sizeof(task_t));

    // Allocate kernel stack
    task->kernel_stack = (uint64_t *)kmalloc(TASK_KERNEL_STACK_SIZE);
    if (!task->kernel_stack) {
        serial_write("[TASK] ERROR: Failed to allocate kernel stack\n");
        kfree(task);
        return NULL;
    }

    // Clear kernel stack
    memset(task->kernel_stack, 0, TASK_KERNEL_STACK_SIZE);

    // Set task properties
    task->pid = next_pid++;
    task->state = TASK_READY;
    task->priority = priority;
    task->time_slice = 10;  // 10 ticks default time slice
    task->total_runtime = 0;
    task->next = NULL;
    task->address_space = NULL;  // For now, all tasks share kernel address space

    // Initialize CPU state
    // Stack grows downward, so start at the top
    uint64_t *stack_top = (uint64_t *)((uint64_t)task->kernel_stack + TASK_KERNEL_STACK_SIZE);

    // Set up initial stack frame for context switch
    // When we switch to this task, these values will be popped
    task->cpu_state.rip = (uint64_t)entry;           // Entry point
    task->cpu_state.rsp = (uint64_t)stack_top - 16;  // Stack pointer (16-byte aligned)
    task->cpu_state.rflags = 0x202;                  // Enable interrupts (IF flag)
    task->cpu_state.cs = 0x08;                       // Kernel code segment
    task->cpu_state.ss = 0x10;                       // Kernel data segment

    // Clear all general purpose registers
    task->cpu_state.rax = 0;
    task->cpu_state.rbx = 0;
    task->cpu_state.rcx = 0;
    task->cpu_state.rdx = 0;
    task->cpu_state.rsi = 0;
    task->cpu_state.rdi = 0;
    task->cpu_state.rbp = 0;
    task->cpu_state.r8 = 0;
    task->cpu_state.r9 = 0;
    task->cpu_state.r10 = 0;
    task->cpu_state.r11 = 0;
    task->cpu_state.r12 = 0;
    task->cpu_state.r13 = 0;
    task->cpu_state.r14 = 0;
    task->cpu_state.r15 = 0;

    serial_write("[TASK] Created task with PID ");
    // Simple PID to string conversion
    char pid_str[16];
    int idx = 0;
    uint64_t temp = task->pid;
    if (temp == 0) {
        pid_str[idx++] = '0';
    } else {
        char digits[16];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int j = digit_count - 1; j >= 0; j--) {
            pid_str[idx++] = digits[j];
        }
    }
    pid_str[idx] = '\0';
    serial_write(pid_str);
    serial_write("\n");

    return task;
}

void task_destroy(task_t *task) {
    if (!task) {
        return;
    }

    serial_write("[TASK] Destroying task PID ");
    char pid_str[16];
    int idx = 0;
    uint64_t temp = task->pid;
    if (temp == 0) {
        pid_str[idx++] = '0';
    } else {
        char digits[16];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int j = digit_count - 1; j >= 0; j--) {
            pid_str[idx++] = digits[j];
        }
    }
    pid_str[idx] = '\0';
    serial_write(pid_str);
    serial_write("\n");

    // Free kernel stack
    if (task->kernel_stack) {
        kfree(task->kernel_stack);
    }

    // Free task structure
    kfree(task);
}

task_t *task_get_current(void) {
    return current_task;
}

void task_set_current(task_t *task) {
    current_task = task;
}

task_t *task_create_user(uint64_t entry, uint64_t priority) {
    // Import user mode functions
    extern void *vmm_create_address_space(void);
    extern bool setup_user_memory(void *address_space, uint64_t code_start,
                                   uint64_t code_size, uint64_t stack_top);

    // Allocate task structure
    task_t *task = (task_t *)kmalloc(sizeof(task_t));
    if (!task) {
        serial_write("[TASK] ERROR: Failed to allocate user task structure\n");
        return NULL;
    }

    // Clear task structure
    memset(task, 0, sizeof(task_t));

    // Allocate kernel stack (used when task enters kernel via syscall/interrupt)
    task->kernel_stack = (uint64_t *)kmalloc(TASK_KERNEL_STACK_SIZE);
    if (!task->kernel_stack) {
        serial_write("[TASK] ERROR: Failed to allocate kernel stack for user task\n");
        kfree(task);
        return NULL;
    }

    // Clear kernel stack
    memset(task->kernel_stack, 0, TASK_KERNEL_STACK_SIZE);

    // Create user mode address space
    task->address_space = vmm_create_address_space();
    if (!task->address_space) {
        serial_write("[TASK] ERROR: Failed to create address space for user task\n");
        kfree(task->kernel_stack);
        kfree(task);
        return NULL;
    }

    // Setup user memory (code and stack)
    uint64_t code_start = 0x0000000000400000;  // Standard user code base (4 MB)
    uint64_t code_size = 4096;                  // 1 page for code
    uint64_t user_stack_top = 0x0000000000500000;  // User stack at 5 MB

    if (!setup_user_memory(task->address_space, code_start, code_size, user_stack_top)) {
        serial_write("[TASK] ERROR: Failed to setup user memory\n");
        // TODO: Free address space
        kfree(task->kernel_stack);
        kfree(task);
        return NULL;
    }

    // Set task properties
    task->pid = next_pid++;
    task->state = TASK_READY;
    task->priority = priority;
    task->time_slice = 10;  // 10 ticks default time slice
    task->total_runtime = 0;
    task->next = NULL;

    // Initialize CPU state for user mode
    uint64_t *kernel_stack_top = (uint64_t *)((uint64_t)task->kernel_stack + TASK_KERNEL_STACK_SIZE);

    task->cpu_state.rip = entry;                        // User entry point
    task->cpu_state.rsp = user_stack_top;              // User stack pointer
    task->cpu_state.rflags = 0x202;                    // Enable interrupts (IF flag)
    task->cpu_state.cs = 0x18 | 3;                     // User code segment (RPL=3)
    task->cpu_state.ss = 0x20 | 3;                     // User data segment (RPL=3)

    // Clear all general purpose registers
    task->cpu_state.rax = 0;
    task->cpu_state.rbx = 0;
    task->cpu_state.rcx = 0;
    task->cpu_state.rdx = 0;
    task->cpu_state.rsi = 0;
    task->cpu_state.rdi = 0;
    task->cpu_state.rbp = 0;
    task->cpu_state.r8 = 0;
    task->cpu_state.r9 = 0;
    task->cpu_state.r10 = 0;
    task->cpu_state.r11 = 0;
    task->cpu_state.r12 = 0;
    task->cpu_state.r13 = 0;
    task->cpu_state.r14 = 0;
    task->cpu_state.r15 = 0;

    serial_write("[TASK] Created user mode task with PID ");
    // Simple PID to string conversion
    char pid_str[16];
    int idx = 0;
    uint64_t temp = task->pid;
    if (temp == 0) {
        pid_str[idx++] = '0';
    } else {
        char digits[16];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int j = digit_count - 1; j >= 0; j--) {
            pid_str[idx++] = digits[j];
        }
    }
    pid_str[idx] = '\0';
    serial_write(pid_str);
    serial_write("\n");

    return task;
}
