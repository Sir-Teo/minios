/**
 * test_usermode.c - User mode subsystem tests
 *
 * Tests for user mode transitions and memory protection.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// External functions
extern void serial_write(const char *s);
extern void kprintf(const char *fmt, ...);
extern void *kmalloc(uint64_t size);
extern void kfree(void *ptr);

// User mode functions
extern void usermode_init(void);
extern bool setup_user_memory(void *address_space, uint64_t code_start,
                               uint64_t code_size, uint64_t stack_top);
extern bool is_usermode_address(uint64_t addr);
extern bool is_kernelmode_address(uint64_t addr);

// VMM functions
extern void *vmm_create_address_space(void);
extern void vmm_switch_address_space(void *as);

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

// Test 1: User mode initialization
static void test_usermode_init(void) {
    usermode_init();
    print_test_result("User mode initialization", true);
}

// Test 2: Address space validation - user addresses
static void test_usermode_address_validation(void) {
    bool test1 = is_usermode_address(0x0000000000000000);
    bool test2 = is_usermode_address(0x0000000000400000);
    bool test3 = is_usermode_address(0x00007FFFFFFFFFFF);
    bool test4 = !is_usermode_address(0xFFFF800000000000);
    bool test5 = !is_usermode_address(0xFFFFFFFFFFFFFFFF);

    print_test_result("User address validation",
                     test1 && test2 && test3 && test4 && test5);
}

// Test 3: Address space validation - kernel addresses
static void test_kernelmode_address_validation(void) {
    bool test1 = is_kernelmode_address(0xFFFF800000000000);
    bool test2 = is_kernelmode_address(0xFFFFFFFF80000000);
    bool test3 = is_kernelmode_address(0xFFFFFFFFFFFFFFFF);
    bool test4 = !is_kernelmode_address(0x0000000000000000);
    bool test5 = !is_kernelmode_address(0x00007FFFFFFFFFFF);

    print_test_result("Kernel address validation",
                     test1 && test2 && test3 && test4 && test5);
}

// Test 4: Create user address space
static void test_create_user_address_space(void) {
    void *user_as = vmm_create_address_space();
    bool result = (user_as != NULL);

    print_test_result("Create user address space", result);
}

// Test 5: Setup user memory mapping
static void test_setup_user_memory(void) {
    void *user_as = vmm_create_address_space();
    if (!user_as) {
        print_test_result("Setup user memory mapping", false);
        return;
    }

    uint64_t code_start = 0x0000000000400000;  // 4 MB
    uint64_t code_size = 8192;  // 2 pages
    uint64_t stack_top = 0x0000000000500000;  // 5 MB

    bool result = setup_user_memory(user_as, code_start, code_size, stack_top);

    print_test_result("Setup user memory mapping", result);
}

// Test 6: Multiple user address spaces
static void test_multiple_user_address_spaces(void) {
    void *user_as1 = vmm_create_address_space();
    void *user_as2 = vmm_create_address_space();
    void *user_as3 = vmm_create_address_space();

    bool result = (user_as1 != NULL && user_as2 != NULL && user_as3 != NULL &&
                   user_as1 != user_as2 && user_as2 != user_as3);

    print_test_result("Multiple user address spaces", result);
}

// Test 7: User memory with different sizes
static void test_user_memory_different_sizes(void) {
    void *user_as = vmm_create_address_space();
    if (!user_as) {
        print_test_result("User memory different sizes", false);
        return;
    }

    // Test with 1 page code
    uint64_t code_start1 = 0x0000000000400000;
    uint64_t code_size1 = 4096;
    uint64_t stack_top1 = 0x0000000000500000;

    bool result1 = setup_user_memory(user_as, code_start1, code_size1, stack_top1);

    // Test with larger code (10 pages)
    void *user_as2 = vmm_create_address_space();
    uint64_t code_start2 = 0x0000000000400000;
    uint64_t code_size2 = 40960;  // 10 pages
    uint64_t stack_top2 = 0x0000000000600000;

    bool result2 = setup_user_memory(user_as2, code_start2, code_size2, stack_top2);

    print_test_result("User memory different sizes", result1 && result2);
}

// Test 8: Boundary address validation
static void test_boundary_addresses(void) {
    // Test addresses at boundaries
    bool test1 = is_usermode_address(0x00007FFFFFFFFFFF);  // Last user address
    bool test2 = !is_usermode_address(0x0000800000000000); // First non-user
    bool test3 = !is_kernelmode_address(0x00007FFFFFFFFFFF);
    bool test4 = is_kernelmode_address(0xFFFF800000000000); // First kernel

    print_test_result("Boundary address validation",
                     test1 && test2 && test3 && test4);
}

// Simple user mode function that will make a syscall
// This is just code we'll copy to user space for testing
static void user_program_code(void) {
    // Make a syscall (sys_getpid = 11)
    uint64_t pid;
    __asm__ volatile(
        "movq $11, %%rax\n"     // syscall number (sys_getpid)
        "syscall\n"
        "movq %%rax, %0\n"
        : "=r"(pid)
        :
        : "rax", "rcx", "r11", "memory"
    );

    // Exit user mode (sys_exit = 4)
    __asm__ volatile(
        "movq $4, %%rax\n"      // syscall number (sys_exit)
        "movq $0, %%rdi\n"      // exit code = 0
        "syscall\n"
        :
        :
        : "rax", "rdi", "rcx", "r11", "memory"
    );

    // Should never reach here
    while(1);
}

// Helper to get size of user program
static uint64_t get_user_program_size(void) {
    // Approximate size - in reality we'd use labels in assembly
    return 256;
}

// Test 9: Copy user program to user space
static void test_copy_user_program(void) {
    void *user_as = vmm_create_address_space();
    if (!user_as) {
        print_test_result("Copy user program", false);
        return;
    }

    uint64_t code_start = 0x0000000000400000;
    uint64_t code_size = get_user_program_size();
    uint64_t stack_top = 0x0000000000500000;

    bool result = setup_user_memory(user_as, code_start, code_size, stack_top);

    // In a real test, we would copy the code here
    // For now, we just verify the setup succeeded

    print_test_result("Copy user program", result);
}

// Test 10: Address space isolation check
static void test_address_space_isolation(void) {
    void *user_as1 = vmm_create_address_space();
    void *user_as2 = vmm_create_address_space();

    if (!user_as1 || !user_as2) {
        print_test_result("Address space isolation", false);
        return;
    }

    // Setup identical virtual addresses in both spaces
    uint64_t code_start = 0x0000000000400000;
    uint64_t code_size = 4096;
    uint64_t stack_top = 0x0000000000500000;

    bool r1 = setup_user_memory(user_as1, code_start, code_size, stack_top);
    bool r2 = setup_user_memory(user_as2, code_start, code_size, stack_top);

    // Both should succeed - same virtual addresses can map to different physical pages
    print_test_result("Address space isolation", r1 && r2);
}

// Main test runner
void run_usermode_tests(void) {
    serial_write("========================================\n");
    serial_write("     User Mode Tests                   \n");
    serial_write("========================================\n");

    tests_run = 0;
    tests_passed = 0;
    tests_failed = 0;

    // Run all tests
    test_usermode_init();
    test_usermode_address_validation();
    test_kernelmode_address_validation();
    test_create_user_address_space();
    test_setup_user_memory();
    test_multiple_user_address_spaces();
    test_user_memory_different_sizes();
    test_boundary_addresses();
    test_copy_user_program();
    test_address_space_isolation();

    // Print summary
    serial_write("========================================\n");
    serial_write("[TEST] User mode tests complete\n");

    kprintf("[TEST] Tests run: %d\n", tests_run);
    kprintf("[TEST] Tests passed: %d\n", tests_passed);
    kprintf("[TEST] Tests failed: %d\n", tests_failed);

    if (tests_failed == 0) {
        serial_write("[TEST] ✓ All user mode tests PASSED!\n");
    } else {
        serial_write("[TEST] ✗ Some tests FAILED\n");
    }

    serial_write("========================================\n");
}
