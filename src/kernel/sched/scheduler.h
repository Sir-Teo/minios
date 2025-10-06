#ifndef KERNEL_SCHED_SCHEDULER_H
#define KERNEL_SCHED_SCHEDULER_H

#include "task.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize the scheduler.
 * This sets up the ready queue and creates the idle task.
 */
void sched_init(void);

/**
 * Add a task to the ready queue.
 *
 * @param task Task to add to the ready queue
 */
void sched_add_task(task_t *task);

/**
 * Remove a task from the ready queue.
 *
 * @param task Task to remove
 */
void sched_remove_task(task_t *task);

/**
 * Perform a context switch to the next ready task.
 * This is called from the timer interrupt.
 */
void schedule(void);

/**
 * Yield the CPU voluntarily to another task.
 */
void sched_yield(void);

/**
 * Exit the current task.
 *
 * @param exit_code Exit code
 */
void task_exit(int exit_code);

/**
 * Get the number of tasks in the ready queue.
 *
 * @return Number of ready tasks
 */
uint64_t sched_get_task_count(void);

/**
 * Enable/disable the scheduler.
 *
 * @param enabled true to enable, false to disable
 */
void sched_set_enabled(bool enabled);

/**
 * Check if the scheduler is enabled.
 *
 * @return true if enabled, false otherwise
 */
bool sched_is_enabled(void);

/**
 * Get the currently running task.
 *
 * @return Pointer to current task, or NULL if none
 */
task_t *sched_get_current_task(void);

#endif
