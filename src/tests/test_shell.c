#include "../kernel/shell/shell.h"
#include "../kernel/kprintf.h"
#include <stdint.h>

// Test counters
static int tests_run = 0;
static int tests_passed = 0;

// Helper macros
#define TEST(name) do { \
    kprintf("[TEST] %s...", name); \
    tests_run++; \
} while(0)

#define PASS() do { \
    kprintf(" PASS\n"); \
    tests_passed++; \
} while(0)

#define FAIL(msg) do { \
    kprintf(" FAIL: %s\n", msg); \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        FAIL(msg); \
        return; \
    } \
} while(0)

/**
 * Test 1: Shell initialization
 */
static void test_shell_init(void) {
    TEST("Shell initialization");

    // Shell should already be initialized by kernel
    // Just verify it doesn't crash

    PASS();
}

/**
 * Test 2: Execute echo command
 */
static void test_shell_echo(void) {
    TEST("Execute echo command");

    int result = shell_execute("echo Hello World");
    ASSERT(result == 0, "echo should succeed");

    PASS();
}

/**
 * Test 3: Execute help command
 */
static void test_shell_help(void) {
    TEST("Execute help command");

    int result = shell_execute("help");
    ASSERT(result == 0, "help should succeed");

    PASS();
}

/**
 * Test 4: Execute uname command
 */
static void test_shell_uname(void) {
    TEST("Execute uname command");

    int result = shell_execute("uname");
    ASSERT(result == 0, "uname should succeed");

    PASS();
}

/**
 * Test 5: Execute uptime command
 */
static void test_shell_uptime(void) {
    TEST("Execute uptime command");

    int result = shell_execute("uptime");
    ASSERT(result == 0, "uptime should succeed");

    PASS();
}

/**
 * Test 6: Execute free command
 */
static void test_shell_free(void) {
    TEST("Execute free command");

    int result = shell_execute("free");
    ASSERT(result == 0, "free should succeed");

    PASS();
}

/**
 * Test 7: Unknown command
 */
static void test_shell_unknown(void) {
    TEST("Unknown command handling");

    int result = shell_execute("invalidcommand");
    ASSERT(result != 0, "Unknown command should fail");

    PASS();
}

/**
 * Test 8: Empty command
 */
static void test_shell_empty(void) {
    TEST("Empty command handling");

    int result = shell_execute("");
    ASSERT(result == 0, "Empty command should succeed silently");

    PASS();
}

/**
 * Test 9: Command with multiple arguments
 */
static void test_shell_multiarg(void) {
    TEST("Command with multiple arguments");

    int result = shell_execute("echo one two three");
    ASSERT(result == 0, "Multi-arg echo should succeed");

    PASS();
}

/**
 * Test 10: Whitespace handling
 */
static void test_shell_whitespace(void) {
    TEST("Whitespace handling");

    int result = shell_execute("  echo   test  ");
    ASSERT(result == 0, "Command with extra whitespace should succeed");

    PASS();
}

/**
 * Run all shell tests
 */
void test_shell_run_all(void) {
    kprintf("\n=== Shell Tests ===\n");

    tests_run = 0;
    tests_passed = 0;

    // Run all tests
    test_shell_init();
    test_shell_echo();
    test_shell_help();
    test_shell_uname();
    test_shell_uptime();
    test_shell_free();
    test_shell_unknown();
    test_shell_empty();
    test_shell_multiarg();
    test_shell_whitespace();

    // Print summary
    kprintf("\n=== Shell Test Summary ===\n");
    kprintf("Tests run: %d\n", tests_run);
    kprintf("Tests passed: %d\n", tests_passed);
    kprintf("Tests failed: %d\n", tests_run - tests_passed);

    if (tests_passed == tests_run) {
        kprintf("Result: ALL TESTS PASSED\n");
    } else {
        kprintf("Result: SOME TESTS FAILED\n");
    }
}
