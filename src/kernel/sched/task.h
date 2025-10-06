#ifndef KERNEL_SCHED_TASK_H
#define KERNEL_SCHED_TASK_H

#include <stdint.h>
#include <stdbool.h>

// Forward declaration for address_space_t
struct address_space;

// Task states
typedef enum {
    TASK_READY,      // Ready to run
    TASK_RUNNING,    // Currently executing
    TASK_BLOCKED,    // Waiting for I/O or event
    TASK_TERMINATED  // Finished execution
} task_state_t;

// CPU register state (saved during context switch)
typedef struct {
    // General purpose registers
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;

    // Instruction pointer and flags
    uint64_t rip;
    uint64_t rflags;

    // Segment selectors
    uint64_t cs, ss;
} __attribute__((packed)) cpu_state_t;

// Task Control Block (TCB)
typedef struct task {
    uint64_t pid;                          // Process ID
    uint64_t *kernel_stack;                // Kernel stack pointer
    cpu_state_t cpu_state;                 // Saved CPU state
    struct address_space *address_space;   // Virtual memory address space
    task_state_t state;                    // Current task state
    uint64_t priority;                     // Task priority (0 = highest)
    uint64_t time_slice;                   // Remaining time slice (in ticks)
    uint64_t total_runtime;                // Total CPU time used
    struct task *next;                     // Next task in queue
} task_t;

/**
 * Initialize the task subsystem.
 */
void task_init(void);

/**
 * Create a new task with the specified entry point.
 *
 * @param entry Entry point function for the task
 * @param priority Task priority (0 = highest)
 * @return Pointer to the created task, or NULL on failure
 */
task_t *task_create(void (*entry)(void), uint64_t priority);

/**
 * Destroy a task and free its resources.
 *
 * @param task Task to destroy
 */
void task_destroy(task_t *task);

/**
 * Get the currently running task.
 *
 * @return Pointer to the current task
 */
task_t *task_get_current(void);

/**
 * Set the currently running task.
 *
 * @param task Task to set as current
 */
void task_set_current(task_t *task);

/**
 * Create a new user mode task with the specified entry point.
 *
 * @param entry Entry point address in user space
 * @param priority Task priority (0 = highest)
 * @return Pointer to the created task, or NULL on failure
 */
task_t *task_create_user(uint64_t entry, uint64_t priority);

#endif
