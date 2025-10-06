#include "../kernel/sched/scheduler.h"
#include "../kernel/sched/task.h"
#include "../drivers/timer/pit.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// External functions
extern void serial_write(const char *s);

// Test framework macros
static int test_count = 0;
static int test_passed = 0;
static int test_failed = 0;

#define TEST_ASSERT(condition, message) do { \
    test_count++; \
    if (condition) { \
        serial_write("  [PASS] "); \
        serial_write(message); \
        serial_write("\n"); \
        test_passed++; \
    } else { \
        serial_write("  [FAIL] "); \
        serial_write(message); \
        serial_write("\n"); \
        test_failed++; \
    } \
} while(0)

// Test task counters
static volatile int task1_counter = 0;
static volatile int task2_counter = 0;
static volatile int task3_counter = 0;

// Test task functions
void test_task1_func(void) {
    for (int i = 0; i < 5; i++) {
        task1_counter++;
        sched_yield();  // Voluntarily yield
    }
    task_exit(0);
}

void test_task2_func(void) {
    for (int i = 0; i < 5; i++) {
        task2_counter++;
        sched_yield();
    }
    task_exit(0);
}

void test_task3_func(void) {
    for (int i = 0; i < 3; i++) {
        task3_counter++;
        sched_yield();
    }
    task_exit(0);
}

// Simple busy-wait test task
static volatile bool test_task_ran = false;

void simple_test_task(void) {
    test_task_ran = true;
    task_exit(0);
}

/**
 * Test 1: Initialize task subsystem
 */
void test_task_init(void) {
    serial_write("\n[TEST] Task Subsystem Initialization\n");

    task_init();
    TEST_ASSERT(true, "Task subsystem initialized");

    task_t *current = task_get_current();
    TEST_ASSERT(current == NULL, "No current task initially");
}

/**
 * Test 2: Create and destroy tasks
 */
void test_task_create_destroy(void) {
    serial_write("\n[TEST] Task Creation and Destruction\n");

    // Create a task
    task_t *task = task_create(simple_test_task, 1);
    TEST_ASSERT(task != NULL, "Task created successfully");

    if (task) {
        TEST_ASSERT(task->pid > 0, "Task has valid PID");
        TEST_ASSERT(task->state == TASK_READY, "Task state is READY");
        TEST_ASSERT(task->priority == 1, "Task priority is correct");
        TEST_ASSERT(task->kernel_stack != NULL, "Task has kernel stack");

        // Destroy the task
        task_destroy(task);
        TEST_ASSERT(true, "Task destroyed successfully");
    }
}

/**
 * Test 3: Initialize scheduler
 */
void test_sched_init(void) {
    serial_write("\n[TEST] Scheduler Initialization\n");

    sched_init();
    TEST_ASSERT(true, "Scheduler initialized");

    uint64_t task_count = sched_get_task_count();
    TEST_ASSERT(task_count == 1, "Idle task exists in ready queue");

    TEST_ASSERT(!sched_is_enabled(), "Scheduler initially disabled");
}

/**
 * Test 4: Add and remove tasks from scheduler
 */
void test_sched_add_remove(void) {
    serial_write("\n[TEST] Scheduler Add/Remove Tasks\n");

    task_t *task1 = task_create(simple_test_task, 1);
    task_t *task2 = task_create(simple_test_task, 2);

    TEST_ASSERT(task1 != NULL && task2 != NULL, "Tasks created");

    if (task1 && task2) {
        uint64_t count_before = sched_get_task_count();

        sched_add_task(task1);
        TEST_ASSERT(sched_get_task_count() == count_before + 1, "Task added to queue");

        sched_add_task(task2);
        TEST_ASSERT(sched_get_task_count() == count_before + 2, "Second task added");

        sched_remove_task(task1);
        TEST_ASSERT(sched_get_task_count() == count_before + 1, "Task removed from queue");

        sched_remove_task(task2);
        TEST_ASSERT(sched_get_task_count() == count_before, "Second task removed");

        task_destroy(task1);
        task_destroy(task2);
    }
}

/**
 * Test 5: Enable/disable scheduler
 */
void test_sched_enable_disable(void) {
    serial_write("\n[TEST] Scheduler Enable/Disable\n");

    sched_set_enabled(false);
    TEST_ASSERT(!sched_is_enabled(), "Scheduler disabled");

    sched_set_enabled(true);
    TEST_ASSERT(sched_is_enabled(), "Scheduler enabled");

    sched_set_enabled(false);
    TEST_ASSERT(!sched_is_enabled(), "Scheduler disabled again");
}

/**
 * Test 6: Task state transitions
 */
void test_task_states(void) {
    serial_write("\n[TEST] Task State Transitions\n");

    task_t *task = task_create(simple_test_task, 1);
    TEST_ASSERT(task != NULL, "Task created");

    if (task) {
        TEST_ASSERT(task->state == TASK_READY, "Initial state is READY");

        task->state = TASK_RUNNING;
        TEST_ASSERT(task->state == TASK_RUNNING, "State changed to RUNNING");

        task->state = TASK_BLOCKED;
        TEST_ASSERT(task->state == TASK_BLOCKED, "State changed to BLOCKED");

        task->state = TASK_TERMINATED;
        TEST_ASSERT(task->state == TASK_TERMINATED, "State changed to TERMINATED");

        task_destroy(task);
    }
}

/**
 * Test 7: Multiple task creation
 */
void test_multiple_tasks(void) {
    serial_write("\n[TEST] Multiple Task Creation\n");

    task_t *tasks[5];
    int created_count = 0;

    for (int i = 0; i < 5; i++) {
        tasks[i] = task_create(simple_test_task, i);
        if (tasks[i]) {
            created_count++;
        }
    }

    TEST_ASSERT(created_count == 5, "All 5 tasks created");

    // Verify PIDs are unique
    bool pids_unique = true;
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            if (tasks[i]->pid == tasks[j]->pid) {
                pids_unique = false;
                break;
            }
        }
    }
    TEST_ASSERT(pids_unique, "All task PIDs are unique");

    // Clean up
    for (int i = 0; i < 5; i++) {
        if (tasks[i]) {
            task_destroy(tasks[i]);
        }
    }
}

/**
 * Test 8: Task priority handling
 */
void test_task_priority(void) {
    serial_write("\n[TEST] Task Priority\n");

    task_t *high_prio = task_create(simple_test_task, 0);
    task_t *low_prio = task_create(simple_test_task, 10);

    TEST_ASSERT(high_prio != NULL && low_prio != NULL, "Tasks with different priorities created");

    if (high_prio && low_prio) {
        TEST_ASSERT(high_prio->priority == 0, "High priority task has priority 0");
        TEST_ASSERT(low_prio->priority == 10, "Low priority task has priority 10");
        TEST_ASSERT(high_prio->priority < low_prio->priority, "Priority ordering correct");

        task_destroy(high_prio);
        task_destroy(low_prio);
    }
}

/**
 * Test 9: CPU state initialization
 */
void test_cpu_state_init(void) {
    serial_write("\n[TEST] CPU State Initialization\n");

    task_t *task = task_create(simple_test_task, 1);
    TEST_ASSERT(task != NULL, "Task created");

    if (task) {
        TEST_ASSERT(task->cpu_state.rip != 0, "RIP initialized to entry point");
        TEST_ASSERT(task->cpu_state.rsp != 0, "RSP initialized to stack");
        TEST_ASSERT(task->cpu_state.rflags == 0x202, "RFLAGS has IF flag set");
        TEST_ASSERT(task->cpu_state.cs == 0x08, "CS set to kernel code segment");
        TEST_ASSERT(task->cpu_state.ss == 0x10, "SS set to kernel data segment");

        // Verify stack is 16-byte aligned
        TEST_ASSERT((task->cpu_state.rsp & 0xF) == 0, "Stack is 16-byte aligned");

        task_destroy(task);
    }
}

/**
 * Test 10: Scheduler task count
 */
void test_sched_task_count(void) {
    serial_write("\n[TEST] Scheduler Task Count\n");

    // Reinitialize scheduler for clean state
    sched_init();

    uint64_t initial_count = sched_get_task_count();
    TEST_ASSERT(initial_count == 1, "Initial count is 1 (idle task)");

    task_t *task1 = task_create(simple_test_task, 1);
    task_t *task2 = task_create(simple_test_task, 2);

    if (task1 && task2) {
        sched_add_task(task1);
        TEST_ASSERT(sched_get_task_count() == initial_count + 1, "Count increased by 1");

        sched_add_task(task2);
        TEST_ASSERT(sched_get_task_count() == initial_count + 2, "Count increased by 2");

        sched_remove_task(task1);
        sched_remove_task(task2);
        TEST_ASSERT(sched_get_task_count() == initial_count, "Count back to initial");

        task_destroy(task1);
        task_destroy(task2);
    }
}

/**
 * Main test runner
 */
void run_sched_tests(void) {
    serial_write("\n");
    serial_write("========================================\n");
    serial_write("   Scheduler Test Suite\n");
    serial_write("========================================\n");

    test_count = 0;
    test_passed = 0;
    test_failed = 0;

    // Run all tests
    test_task_init();
    test_task_create_destroy();
    test_sched_init();
    test_sched_add_remove();
    test_sched_enable_disable();
    test_task_states();
    test_multiple_tasks();
    test_task_priority();
    test_cpu_state_init();
    test_sched_task_count();

    // Print summary
    serial_write("\n========================================\n");
    serial_write("   Test Summary\n");
    serial_write("========================================\n");

    char buffer[64];
    int idx;

    // Total tests
    serial_write("Total tests:  ");
    idx = 0;
    int temp = test_count;
    if (temp == 0) {
        buffer[idx++] = '0';
    } else {
        char digits[16];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int j = digit_count - 1; j >= 0; j--) {
            buffer[idx++] = digits[j];
        }
    }
    buffer[idx] = '\0';
    serial_write(buffer);
    serial_write("\n");

    // Passed tests
    serial_write("Passed:       ");
    idx = 0;
    temp = test_passed;
    if (temp == 0) {
        buffer[idx++] = '0';
    } else {
        char digits[16];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int j = digit_count - 1; j >= 0; j--) {
            buffer[idx++] = digits[j];
        }
    }
    buffer[idx] = '\0';
    serial_write(buffer);
    serial_write("\n");

    // Failed tests
    serial_write("Failed:       ");
    idx = 0;
    temp = test_failed;
    if (temp == 0) {
        buffer[idx++] = '0';
    } else {
        char digits[16];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int j = digit_count - 1; j >= 0; j--) {
            buffer[idx++] = digits[j];
        }
    }
    buffer[idx] = '\0';
    serial_write(buffer);
    serial_write("\n");

    serial_write("========================================\n");

    if (test_failed == 0) {
        serial_write("   ✓ ALL TESTS PASSED!\n");
    } else {
        serial_write("   ✗ SOME TESTS FAILED\n");
    }
    serial_write("========================================\n\n");
}
