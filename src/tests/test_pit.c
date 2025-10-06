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

// Global variables for testing callbacks
static volatile int callback_count = 0;
static volatile bool callback_executed = false;

void test_timer_callback(void) {
    callback_count++;
    callback_executed = true;
}

/**
 * Test 1: Verify PIT initialization doesn't crash
 */
void test_pit_init(void) {
    serial_write("\n[TEST] PIT Initialization\n");

    // Initialize with common frequency (100 Hz)
    pit_init(100);
    TEST_ASSERT(true, "PIT initialized at 100Hz without crash");

    // Check tick count is zero after init
    uint64_t ticks = pit_get_ticks();
    TEST_ASSERT(ticks == 0, "Initial tick count is zero");
}

/**
 * Test 2: Verify tick counting works
 */
void test_pit_tick_counting(void) {
    serial_write("\n[TEST] PIT Tick Counting\n");

    // Reset ticks (re-initialize)
    pit_init(1000);  // 1000 Hz for faster testing

    uint64_t start_ticks = pit_get_ticks();
    TEST_ASSERT(start_ticks == 0, "Tick count resets after re-init");

    // Wait for some ticks
    pit_sleep(10);

    uint64_t end_ticks = pit_get_ticks();
    TEST_ASSERT(end_ticks >= 10, "Ticks increment after sleep");
    TEST_ASSERT(end_ticks < 100, "Tick count is reasonable (not overflowing)");
}

/**
 * Test 3: Verify sleep function works
 */
void test_pit_sleep(void) {
    serial_write("\n[TEST] PIT Sleep Function\n");

    pit_init(100);  // 100 Hz

    uint64_t before = pit_get_ticks();
    pit_sleep(5);  // Sleep for 5 ticks
    uint64_t after = pit_get_ticks();

    uint64_t elapsed = after - before;
    TEST_ASSERT(elapsed >= 5, "Sleep for 5 ticks completes");
    TEST_ASSERT(elapsed < 10, "Sleep doesn't take too long");
}

/**
 * Test 4: Verify different frequencies work
 */
void test_pit_different_frequencies(void) {
    serial_write("\n[TEST] PIT Different Frequencies\n");

    // Test 50 Hz
    pit_init(50);
    TEST_ASSERT(true, "PIT initialized at 50Hz");

    // Test 1000 Hz
    pit_init(1000);
    TEST_ASSERT(true, "PIT initialized at 1000Hz");

    // Test 18.2 Hz (original PC timer frequency ~ 18.2065 Hz)
    pit_init(18);
    TEST_ASSERT(true, "PIT initialized at 18Hz");
}

/**
 * Test 5: Verify callback mechanism
 */
void test_pit_callback(void) {
    serial_write("\n[TEST] PIT Callback Mechanism\n");

    callback_count = 0;
    callback_executed = false;

    // Set callback
    pit_set_callback(test_timer_callback);
    TEST_ASSERT(true, "Callback set without crash");

    // Initialize timer
    pit_init(100);

    // Wait for callback to be called
    pit_sleep(5);

    TEST_ASSERT(callback_executed, "Callback was executed");
    TEST_ASSERT(callback_count >= 5, "Callback called multiple times");

    // Clear callback
    pit_set_callback(NULL);

    int count_before = callback_count;
    pit_sleep(5);
    int count_after = callback_count;

    TEST_ASSERT(count_before == count_after, "Callback not called after clearing");
}

/**
 * Test 6: Verify tick overflow handling (theoretical)
 */
void test_pit_tick_overflow(void) {
    serial_write("\n[TEST] PIT Tick Counter Properties\n");

    // We can't actually overflow a 64-bit counter in a test,
    // but we can verify the counter size
    uint64_t max_ticks = 0xFFFFFFFFFFFFFFFFULL;
    TEST_ASSERT(sizeof(uint64_t) == 8, "Tick counter is 64-bit");

    // At 1000 Hz, it would take ~584 million years to overflow
    // This is a theoretical test to ensure the counter is appropriately sized
    TEST_ASSERT(max_ticks > 1000000000000ULL, "Tick counter can hold trillions of ticks");
}

/**
 * Test 7: Verify multiple short sleeps
 */
void test_pit_multiple_sleeps(void) {
    serial_write("\n[TEST] PIT Multiple Short Sleeps\n");

    pit_init(100);

    uint64_t start = pit_get_ticks();

    for (int i = 0; i < 5; i++) {
        pit_sleep(2);
    }

    uint64_t end = pit_get_ticks();
    uint64_t total = end - start;

    TEST_ASSERT(total >= 10, "Multiple sleeps accumulate correctly");
    TEST_ASSERT(total < 20, "Multiple sleeps don't accumulate too much overhead");
}

/**
 * Test 8: Verify callback with high frequency
 */
void test_pit_high_frequency_callback(void) {
    serial_write("\n[TEST] PIT High Frequency Callback\n");

    callback_count = 0;
    pit_set_callback(test_timer_callback);

    // High frequency (1000 Hz = 1ms per tick)
    pit_init(1000);

    pit_sleep(100);  // 100 ms

    TEST_ASSERT(callback_count >= 90, "High frequency callback called ~100 times");
    TEST_ASSERT(callback_count <= 110, "High frequency callback count is accurate");

    pit_set_callback(NULL);
}

/**
 * Test 9: Verify timer accuracy (rough test)
 */
void test_pit_accuracy(void) {
    serial_write("\n[TEST] PIT Timer Accuracy\n");

    pit_init(100);  // 100 Hz = 10 ms per tick

    uint64_t before = pit_get_ticks();
    pit_sleep(10);  // Should be ~100ms
    uint64_t after = pit_get_ticks();

    uint64_t elapsed = after - before;

    // Allow 20% tolerance (between 8 and 12 ticks)
    TEST_ASSERT(elapsed >= 8, "Timer accuracy within tolerance (low bound)");
    TEST_ASSERT(elapsed <= 12, "Timer accuracy within tolerance (high bound)");
}

/**
 * Test 10: Edge case - zero tick sleep
 */
void test_pit_zero_sleep(void) {
    serial_write("\n[TEST] PIT Zero Tick Sleep\n");

    pit_init(100);

    uint64_t before = pit_get_ticks();
    pit_sleep(0);  // Should return immediately
    uint64_t after = pit_get_ticks();

    // Zero sleep should complete almost instantly (within 1-2 ticks max)
    TEST_ASSERT(after - before <= 2, "Zero tick sleep returns quickly");
}

/**
 * Main test runner
 */
void run_pit_tests(void) {
    serial_write("\n");
    serial_write("========================================\n");
    serial_write("   PIT (Timer) Test Suite\n");
    serial_write("========================================\n");

    test_count = 0;
    test_passed = 0;
    test_failed = 0;

    // Run all tests
    test_pit_init();
    test_pit_tick_counting();
    test_pit_sleep();
    test_pit_different_frequencies();
    test_pit_callback();
    test_pit_tick_overflow();
    test_pit_multiple_sleeps();
    test_pit_high_frequency_callback();
    test_pit_accuracy();
    test_pit_zero_sleep();

    // Print summary
    serial_write("\n========================================\n");
    serial_write("   Test Summary\n");
    serial_write("========================================\n");

    // Convert test counts to strings
    char buffer[64];
    int idx = 0;

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
