#include "scheduler.h"
#include "task.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// External functions
extern void serial_write(const char *s);
extern void context_switch(cpu_state_t *old_state, cpu_state_t *new_state);

// Scheduler state
static task_t *ready_queue_head = NULL;
static task_t *ready_queue_tail = NULL;
static uint64_t task_count = 0;
static bool scheduler_enabled = false;

// Idle task (runs when no other tasks are ready)
static task_t *idle_task = NULL;

/**
 * Idle task function - runs when no other tasks are ready.
 */
static void idle_task_func(void) {
    while (1) {
        // Halt until next interrupt
        __asm__ volatile("hlt");
    }
}

void sched_init(void) {
    serial_write("[SCHED] Initializing scheduler...\n");

    ready_queue_head = NULL;
    ready_queue_tail = NULL;
    task_count = 0;
    scheduler_enabled = false;

    // Create idle task (lowest priority)
    idle_task = task_create(idle_task_func, 999);
    if (!idle_task) {
        serial_write("[SCHED] ERROR: Failed to create idle task\n");
        return;
    }

    idle_task->state = TASK_READY;
    serial_write("[SCHED] Idle task created\n");

    // Add idle task to ready queue
    sched_add_task(idle_task);

    serial_write("[SCHED] Scheduler initialized\n");
}

void sched_add_task(task_t *task) {
    if (!task) {
        return;
    }

    task->state = TASK_READY;
    task->next = NULL;

    // Add to end of ready queue
    if (ready_queue_tail) {
        ready_queue_tail->next = task;
        ready_queue_tail = task;
    } else {
        ready_queue_head = task;
        ready_queue_tail = task;
    }

    task_count++;

    serial_write("[SCHED] Added task PID ");
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
    pid_str[idx++] = ' ';
    pid_str[idx++] = 't';
    pid_str[idx++] = 'o';
    pid_str[idx++] = ' ';
    pid_str[idx++] = 'q';
    pid_str[idx++] = 'u';
    pid_str[idx++] = 'e';
    pid_str[idx++] = 'u';
    pid_str[idx++] = 'e';
    pid_str[idx++] = '\n';
    pid_str[idx] = '\0';
    serial_write(pid_str);
}

void sched_remove_task(task_t *task) {
    if (!task || !ready_queue_head) {
        return;
    }

    // Special case: removing head
    if (ready_queue_head == task) {
        ready_queue_head = task->next;
        if (ready_queue_tail == task) {
            ready_queue_tail = NULL;
        }
        task_count--;
        return;
    }

    // Search for task in queue
    task_t *prev = ready_queue_head;
    task_t *curr = ready_queue_head->next;

    while (curr) {
        if (curr == task) {
            prev->next = curr->next;
            if (ready_queue_tail == curr) {
                ready_queue_tail = prev;
            }
            task_count--;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void schedule(void) {
    if (!scheduler_enabled || !ready_queue_head) {
        return;
    }

    task_t *current = task_get_current();
    task_t *next = ready_queue_head;

    // If there's only one task (the idle task), don't bother switching
    if (task_count == 1 && next == idle_task && current == idle_task) {
        return;
    }

    // Round-robin: move current task to end of queue if it's still ready
    if (current && current->state == TASK_RUNNING) {
        current->state = TASK_READY;

        // Remove from head
        ready_queue_head = current->next;
        if (ready_queue_head == NULL) {
            ready_queue_tail = NULL;
        }

        // Add to tail
        current->next = NULL;
        if (ready_queue_tail) {
            ready_queue_tail->next = current;
            ready_queue_tail = current;
        } else {
            ready_queue_head = current;
            ready_queue_tail = current;
        }
    }

    // Get next task from head of queue
    next = ready_queue_head;
    if (!next) {
        // This shouldn't happen (idle task should always be there)
        return;
    }

    // Mark next task as running
    next->state = TASK_RUNNING;
    task_set_current(next);

    // Perform context switch
    if (current && current != next) {
        context_switch(&current->cpu_state, &next->cpu_state);
    } else if (!current) {
        // First task switch - just load the new state
        context_switch(NULL, &next->cpu_state);
    }
}

void sched_yield(void) {
    schedule();
}

void task_exit(int exit_code) {
    task_t *current = task_get_current();
    if (!current) {
        return;
    }

    serial_write("[SCHED] Task ");
    char pid_str[16];
    int idx = 0;
    uint64_t temp = current->pid;
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
    pid_str[idx++] = ' ';
    pid_str[idx++] = 'e';
    pid_str[idx++] = 'x';
    pid_str[idx++] = 'i';
    pid_str[idx++] = 't';
    pid_str[idx++] = 'e';
    pid_str[idx++] = 'd';
    pid_str[idx++] = '\n';
    pid_str[idx] = '\0';
    serial_write(pid_str);

    current->state = TASK_TERMINATED;

    // Remove from ready queue
    sched_remove_task(current);

    // Switch to next task
    schedule();

    // Should never reach here
    while (1) {
        __asm__ volatile("hlt");
    }
}

uint64_t sched_get_task_count(void) {
    return task_count;
}

void sched_set_enabled(bool enabled) {
    scheduler_enabled = enabled;
    if (enabled) {
        serial_write("[SCHED] Scheduler enabled\n");
    } else {
        serial_write("[SCHED] Scheduler disabled\n");
    }
}

bool sched_is_enabled(void) {
    return scheduler_enabled;
}

task_t *sched_get_current_task(void) {
    return task_get_current();
}
