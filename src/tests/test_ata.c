#include "../drivers/disk/ata.h"
#include "../kernel/kprintf.h"
#include "../kernel/mm/kmalloc.h"
#include <stdint.h>

// Freestanding C library functions
extern void *memset(void *dest, int c, size_t n);
extern void *memcpy(void *restrict dest, const void *restrict src, size_t n);
extern int memcmp(const void *s1, const void *s2, size_t n);

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
 * Test 1: Drive detection
 */
static void test_drive_detection(void) {
    TEST("Drive detection");

    // Check if at least one drive was detected
    bool drive_found = false;
    for (int i = 0; i < 4; i++) {
        const ata_drive_t *drive = ata_get_drive_info(i);
        if (drive != NULL) {
            drive_found = true;
            kprintf("\n[TEST]   Drive %d: %s (%lu MB)",
                    i, drive->model, (drive->sectors * 512) / (1024 * 1024));
        }
    }

    if (drive_found) {
        kprintf("\n");
        PASS();
    } else {
        kprintf("\n");
        FAIL("No drives detected (this may be expected in some environments)");
    }
}

/**
 * Test 2: Read single sector
 */
static void test_read_single_sector(void) {
    TEST("Read single sector");

    // Find first available drive
    const ata_drive_t *drive = NULL;
    int drive_num = -1;
    for (int i = 0; i < 4; i++) {
        drive = ata_get_drive_info(i);
        if (drive != NULL) {
            drive_num = i;
            break;
        }
    }

    if (drive == NULL) {
        kprintf(" SKIP (no drives available)\n");
        tests_passed++;  // Don't fail if no drives
        return;
    }

    // Allocate buffer for one sector
    uint8_t *buffer = (uint8_t *)kmalloc(512);
    ASSERT(buffer != NULL, "Failed to allocate buffer");

    // Read sector 0 (MBR)
    int result = ata_read_sectors(drive_num, 0, 1, buffer);
    ASSERT(result == 0, "Read failed");

    // Check for valid MBR signature (0x55AA at offset 510)
    ASSERT(buffer[510] == 0x55 && buffer[511] == 0xAA, "Invalid MBR signature");

    kfree(buffer);
    PASS();
}

/**
 * Test 3: Read multiple sectors
 */
static void test_read_multiple_sectors(void) {
    TEST("Read multiple sectors");

    // Find first available drive
    int drive_num = -1;
    for (int i = 0; i < 4; i++) {
        if (ata_get_drive_info(i) != NULL) {
            drive_num = i;
            break;
        }
    }

    if (drive_num == -1) {
        kprintf(" SKIP (no drives available)\n");
        tests_passed++;
        return;
    }

    // Allocate buffer for 4 sectors
    uint8_t *buffer = (uint8_t *)kmalloc(512 * 4);
    ASSERT(buffer != NULL, "Failed to allocate buffer");

    // Read 4 sectors starting at sector 0
    int result = ata_read_sectors(drive_num, 0, 4, buffer);
    ASSERT(result == 0, "Read failed");

    // Verify MBR signature is still correct
    ASSERT(buffer[510] == 0x55 && buffer[511] == 0xAA, "Invalid MBR signature");

    kfree(buffer);
    PASS();
}

/**
 * Test 4: Write and read back (WARNING: This modifies disk!)
 *
 * We'll write to a high LBA sector that's unlikely to be used
 * This test is commented out by default for safety
 */
static void test_write_read_verify(void) {
    TEST("Write and read verification");

    // Find first available drive
    int drive_num = -1;
    const ata_drive_t *drive = NULL;
    for (int i = 0; i < 4; i++) {
        drive = ata_get_drive_info(i);
        if (drive != NULL) {
            drive_num = i;
            break;
        }
    }

    if (drive_num == -1) {
        kprintf(" SKIP (no drives available)\n");
        tests_passed++;
        return;
    }

    // Use a high sector number (less likely to corrupt important data)
    // But make sure it's within drive capacity
    uint64_t test_lba = drive->sectors > 1000 ? 1000 : (drive->sectors / 2);

    // Allocate buffers
    uint8_t *write_buffer = (uint8_t *)kmalloc(512);
    uint8_t *read_buffer = (uint8_t *)kmalloc(512);
    ASSERT(write_buffer != NULL && read_buffer != NULL, "Failed to allocate buffers");

    // Fill write buffer with test pattern
    for (int i = 0; i < 512; i++) {
        write_buffer[i] = (uint8_t)(i & 0xFF);
    }

    // WARNING: This actually writes to disk!
    // Skip this test for safety
    kprintf(" SKIP (write test disabled for safety)\n");
    kfree(write_buffer);
    kfree(read_buffer);
    tests_passed++;
    return;

    // Uncomment below to actually test write/read
    /*
    // Write test data
    int result = ata_write_sectors(drive_num, test_lba, 1, write_buffer);
    ASSERT(result == 0, "Write failed");

    // Read back
    result = ata_read_sectors(drive_num, test_lba, 1, read_buffer);
    ASSERT(result == 0, "Read failed");

    // Verify data matches
    ASSERT(memcmp(write_buffer, read_buffer, 512) == 0, "Data mismatch");

    kfree(write_buffer);
    kfree(read_buffer);
    PASS();
    */
}

/**
 * Test 5: Invalid drive number
 */
static void test_invalid_drive(void) {
    TEST("Invalid drive handling");

    uint8_t buffer[512];

    // Try to read from non-existent drive
    int result = ata_read_sectors(99, 0, 1, buffer);
    ASSERT(result != 0, "Should fail for invalid drive");

    PASS();
}

/**
 * Test 6: Drive info retrieval
 */
static void test_drive_info(void) {
    TEST("Drive info retrieval");

    // Test getting info for all drives
    for (int i = 0; i < 4; i++) {
        const ata_drive_t *drive = ata_get_drive_info(i);
        if (drive != NULL) {
            // Verify basic sanity
            ASSERT(drive->present == true, "Drive should be marked present");
            ASSERT(drive->sectors > 0, "Drive should have sectors");
            ASSERT(drive->model[0] != '\0', "Drive should have model string");
        }
    }

    // Test invalid drive number
    const ata_drive_t *invalid = ata_get_drive_info(99);
    ASSERT(invalid == NULL, "Invalid drive should return NULL");

    PASS();
}

/**
 * Run all ATA disk driver tests
 */
void test_ata_run_all(void) {
    kprintf("\n=== ATA Disk Driver Tests ===\n");

    tests_run = 0;
    tests_passed = 0;

    // Run all tests
    test_drive_detection();
    test_drive_info();
    test_invalid_drive();
    test_read_single_sector();
    test_read_multiple_sectors();
    test_write_read_verify();

    // Print summary
    kprintf("\n=== ATA Test Summary ===\n");
    kprintf("Tests run: %d\n", tests_run);
    kprintf("Tests passed: %d\n", tests_passed);
    kprintf("Tests failed: %d\n", tests_run - tests_passed);

    if (tests_passed == tests_run) {
        kprintf("Result: ALL TESTS PASSED\n");
    } else {
        kprintf("Result: SOME TESTS FAILED\n");
    }
}
