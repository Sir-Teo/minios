/**
 * test_syscall.c - System call subsystem tests
 *
 * Tests for the system call infrastructure and implementations.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// External functions
extern void serial_write(const char *s);

// Test statistics
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Helper to print test results
static void print_test_result(const char *test_name, bool passed) {
    tests_run++;

    serial_write("[TEST] ");
    serial_write(test_name);
    serial_write(": ");

    if (passed) {
        serial_write("PASS\n");
        tests_passed++;
    } else {
        serial_write("FAIL\n");
        tests_failed++;
    }
}

// Helper for syscall invocations (until we have user mode)
// For now we'll test by directly calling the syscall dispatcher
extern int64_t syscall_dispatch(uint64_t syscall_num, uint64_t arg1, uint64_t arg2,
                                uint64_t arg3, uint64_t arg4, uint64_t arg5);

// System call numbers (from syscall.h)
#define SYS_READ    0
#define SYS_WRITE   1
#define SYS_OPEN    2
#define SYS_CLOSE   3
#define SYS_EXIT    4
#define SYS_FORK    5
#define SYS_EXEC    6
#define SYS_WAIT    7
#define SYS_MMAP    8
#define SYS_MUNMAP  9
#define SYS_YIELD   10
#define SYS_GETPID  11

// Test 1: Invalid syscall number
static void test_invalid_syscall(void) {
    int64_t result = syscall_dispatch(999, 0, 0, 0, 0, 0);
    print_test_result("Invalid syscall number", result == -1);
}

// Test 2: sys_write to stdout
static void test_write_stdout(void) {
    const char *msg = "[TEST] Hello from syscall!\n";
    int64_t result = syscall_dispatch(SYS_WRITE, 1, (uint64_t)msg, 27, 0, 0);

    // Should return number of bytes written
    print_test_result("sys_write to stdout", result == 27);
}

// Test 3: sys_write to stderr
static void test_write_stderr(void) {
    const char *msg = "[TEST] Error from syscall!\n";
    int64_t result = syscall_dispatch(SYS_WRITE, 2, (uint64_t)msg, 27, 0, 0);

    // Should return number of bytes written
    print_test_result("sys_write to stderr", result == 27);
}

// Test 4: sys_write to invalid fd
static void test_write_invalid_fd(void) {
    const char *msg = "test";
    int64_t result = syscall_dispatch(SYS_WRITE, 999, (uint64_t)msg, 4, 0, 0);

    // Should return error
    print_test_result("sys_write to invalid fd", result == -1);
}

// Test 5: sys_read (should return not implemented)
static void test_read_not_implemented(void) {
    char buf[16];
    int64_t result = syscall_dispatch(SYS_READ, 0, (uint64_t)buf, 16, 0, 0);

    print_test_result("sys_read not implemented", result == -1);
}

// Test 6: sys_open (should return not implemented)
static void test_open_not_implemented(void) {
    const char *path = "/test.txt";
    int64_t result = syscall_dispatch(SYS_OPEN, (uint64_t)path, 0, 0, 0, 0);

    print_test_result("sys_open not implemented", result == -1);
}

// Test 7: sys_close (should return not implemented)
static void test_close_not_implemented(void) {
    int64_t result = syscall_dispatch(SYS_CLOSE, 3, 0, 0, 0, 0);

    print_test_result("sys_close not implemented", result == -1);
}

// Test 8: sys_fork (should return not implemented)
static void test_fork_not_implemented(void) {
    int64_t result = syscall_dispatch(SYS_FORK, 0, 0, 0, 0, 0);

    print_test_result("sys_fork not implemented", result == -1);
}

// Test 9: sys_exec (should return not implemented)
static void test_exec_not_implemented(void) {
    const char *path = "/bin/test";
    int64_t result = syscall_dispatch(SYS_EXEC, (uint64_t)path, 0, 0, 0, 0);

    print_test_result("sys_exec not implemented", result == -1);
}

// Test 10: sys_wait (should return not implemented)
static void test_wait_not_implemented(void) {
    int64_t result = syscall_dispatch(SYS_WAIT, 1, 0, 0, 0, 0);

    print_test_result("sys_wait not implemented", result == -1);
}

// Test 11: sys_mmap (should return not implemented)
static void test_mmap_not_implemented(void) {
    int64_t result = syscall_dispatch(SYS_MMAP, 0, 4096, 0, 0, 0);

    print_test_result("sys_mmap not implemented", result == -1);
}

// Test 12: sys_munmap (should return not implemented)
static void test_munmap_not_implemented(void) {
    int64_t result = syscall_dispatch(SYS_MUNMAP, 0x10000, 4096, 0, 0, 0);

    print_test_result("sys_munmap not implemented", result == -1);
}

// Test 13: sys_yield
static void test_yield(void) {
    int64_t result = syscall_dispatch(SYS_YIELD, 0, 0, 0, 0, 0);

    print_test_result("sys_yield", result == 0);
}

// Test 14: sys_getpid
static void test_getpid(void) {
    int64_t pid = syscall_dispatch(SYS_GETPID, 0, 0, 0, 0, 0);

    // Should return a valid PID (or -1 if no current task)
    // In our case during init, there might not be a current task yet
    print_test_result("sys_getpid", pid >= -1);
}

// Test 15: Multiple syscalls in sequence
static void test_multiple_syscalls(void) {
    const char *msg1 = "Test 1\n";
    const char *msg2 = "Test 2\n";
    const char *msg3 = "Test 3\n";

    int64_t r1 = syscall_dispatch(SYS_WRITE, 1, (uint64_t)msg1, 7, 0, 0);
    int64_t r2 = syscall_dispatch(SYS_WRITE, 1, (uint64_t)msg2, 7, 0, 0);
    int64_t r3 = syscall_dispatch(SYS_WRITE, 1, (uint64_t)msg3, 7, 0, 0);

    print_test_result("Multiple syscalls in sequence",
                     r1 == 7 && r2 == 7 && r3 == 7);
}

// Main test runner
void run_syscall_tests(void) {
    serial_write("========================================\n");
    serial_write("     System Call Tests                 \n");
    serial_write("========================================\n");

    tests_run = 0;
    tests_passed = 0;
    tests_failed = 0;

    // Run all tests
    test_invalid_syscall();
    test_write_stdout();
    test_write_stderr();
    test_write_invalid_fd();
    test_read_not_implemented();
    test_open_not_implemented();
    test_close_not_implemented();
    test_fork_not_implemented();
    test_exec_not_implemented();
    test_wait_not_implemented();
    test_mmap_not_implemented();
    test_munmap_not_implemented();
    test_yield();
    test_getpid();
    test_multiple_syscalls();

    // Print summary
    serial_write("========================================\n");
    serial_write("[TEST] System call tests complete\n");
    serial_write("[TEST] Tests run: ");

    // Print numbers (simple conversion)
    char num_str[16];
    int idx = 0;

    // Tests run
    int temp = tests_run;
    if (temp == 0) {
        num_str[idx++] = '0';
    } else {
        char digits[16];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int j = digit_count - 1; j >= 0; j--) {
            num_str[idx++] = digits[j];
        }
    }
    num_str[idx] = '\0';
    serial_write(num_str);
    serial_write("\n");

    serial_write("[TEST] Tests passed: ");
    idx = 0;
    temp = tests_passed;
    if (temp == 0) {
        num_str[idx++] = '0';
    } else {
        char digits[16];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int j = digit_count - 1; j >= 0; j--) {
            num_str[idx++] = digits[j];
        }
    }
    num_str[idx] = '\0';
    serial_write(num_str);
    serial_write("\n");

    serial_write("[TEST] Tests failed: ");
    idx = 0;
    temp = tests_failed;
    if (temp == 0) {
        num_str[idx++] = '0';
    } else {
        char digits[16];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int j = digit_count - 1; j >= 0; j--) {
            num_str[idx++] = digits[j];
        }
    }
    num_str[idx] = '\0';
    serial_write(num_str);
    serial_write("\n");

    if (tests_failed == 0) {
        serial_write("[TEST] ✓ All system call tests PASSED!\n");
    } else {
        serial_write("[TEST] ✗ Some tests FAILED\n");
    }

    serial_write("========================================\n");
}
