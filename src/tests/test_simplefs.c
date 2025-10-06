#include "../kernel/fs/simplefs.h"
#include "../kernel/kprintf.h"
#include "../drivers/disk/ata.h"
#include <stdint.h>

// Freestanding C library functions
extern int strcmp(const char *s1, const char *s2);
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
 * Test 1: SimpleFS initialization
 */
static void test_sfs_init(void) {
    TEST("SimpleFS initialization");

    // Filesystem should initialize successfully
    // (Already called by kernel)

    PASS();
}

/**
 * Test 2: Format filesystem
 */
static void test_sfs_format(void) {
    TEST("Format filesystem");

    // Check if drive 0 is present
    const ata_drive_t *drive = ata_get_drive_info(0);
    if (!drive || !drive->present) {
        FAIL("Drive 0 not present (skipping filesystem tests)");
        return;
    }

    // Format drive 0 with 64 MB filesystem
    int result = sfs_format(0, 16384);  // 16384 blocks * 4KB = 64 MB
    ASSERT(result == SFS_ERR_SUCCESS, "Format should succeed");

    PASS();
}

/**
 * Test 3: Mount filesystem
 */
static void test_sfs_mount(void) {
    TEST("Mount filesystem");

    // Check if drive 0 is present
    const ata_drive_t *drive = ata_get_drive_info(0);
    if (!drive || !drive->present) {
        FAIL("Drive 0 not present");
        return;
    }

    // Mount the filesystem
    int result = sfs_mount(0, "/disk");
    ASSERT(result == SFS_ERR_SUCCESS, "Mount should succeed");

    // Verify filesystem is mounted
    const sfs_state_t *state = sfs_get_state();
    ASSERT(state != NULL, "State should not be NULL");
    ASSERT(state->mounted == true, "Filesystem should be mounted");
    ASSERT(state->sb.magic == 0x53465330, "Magic number should match");

    PASS();
}

/**
 * Test 4: Create file
 */
static void test_sfs_create(void) {
    TEST("Create file");

    // Create a test file
    int result = sfs_create_file("/test.txt", SFS_TYPE_FILE);
    ASSERT(result == SFS_ERR_SUCCESS, "File creation should succeed");

    // Try to create the same file again (should fail)
    result = sfs_create_file("/test.txt", SFS_TYPE_FILE);
    ASSERT(result == SFS_ERR_EXISTS, "Duplicate file should fail");

    // Create another file
    result = sfs_create_file("/data.bin", SFS_TYPE_FILE);
    ASSERT(result == SFS_ERR_SUCCESS, "Second file creation should succeed");

    PASS();
}

/**
 * Test 5: Write to file
 */
static void test_sfs_write(void) {
    TEST("Write to file");

    const char *data = "Hello from SimpleFS!";
    int result = sfs_write_file("/test.txt", 0, 20, data);
    ASSERT(result == 20, "Write should return 20 bytes written");

    // Write to second file
    const char *data2 = "This is binary data: \x01\x02\x03\x04\x05";
    result = sfs_write_file("/data.bin", 0, 26, data2);
    ASSERT(result == 26, "Write should return 26 bytes written");

    // Write at offset
    const char *append = " More data!";
    result = sfs_write_file("/test.txt", 20, 11, append);
    ASSERT(result == 11, "Append write should succeed");

    PASS();
}

/**
 * Test 6: Read from file
 */
static void test_sfs_read(void) {
    TEST("Read from file");

    char buffer[128];

    // Read entire file
    int result = sfs_read_file("/test.txt", 0, sizeof(buffer), buffer);
    ASSERT(result == 31, "Should read 31 bytes");

    buffer[31] = '\0';
    ASSERT(strcmp(buffer, "Hello from SimpleFS! More data!") == 0, "Content should match");

    // Read partial file
    result = sfs_read_file("/test.txt", 6, 4, buffer);
    ASSERT(result == 4, "Should read 4 bytes");
    buffer[4] = '\0';
    ASSERT(strcmp(buffer, "from") == 0, "Partial read should match");

    // Read from second file
    result = sfs_read_file("/data.bin", 20, 5, buffer);
    ASSERT(result == 5, "Should read 5 bytes from binary file");
    ASSERT(buffer[0] == '\x01' && buffer[4] == '\x05', "Binary data should match");

    PASS();
}

/**
 * Test 7: Read past EOF
 */
static void test_sfs_read_eof(void) {
    TEST("Read past EOF");

    char buffer[128];

    // Read past end of file
    int result = sfs_read_file("/test.txt", 25, 100, buffer);
    ASSERT(result == 6, "Should read only remaining 6 bytes");

    // Read at EOF
    result = sfs_read_file("/test.txt", 31, 10, buffer);
    ASSERT(result == 0, "Should return 0 at EOF");

    // Read beyond EOF
    result = sfs_read_file("/test.txt", 100, 10, buffer);
    ASSERT(result == 0, "Should return 0 beyond EOF");

    PASS();
}

/**
 * Test 8: Large file write/read
 */
static void test_sfs_large_file(void) {
    TEST("Large file write/read");

    // Create a file
    int result = sfs_create_file("/large.dat", SFS_TYPE_FILE);
    ASSERT(result == SFS_ERR_SUCCESS, "Large file creation should succeed");

    // Write 8KB of data (2 blocks)
    char *data = (char *)0x100000;  // Use some known memory location
    for (int i = 0; i < 8192; i++) {
        data[i] = (char)(i & 0xFF);
    }

    result = sfs_write_file("/large.dat", 0, 8192, data);
    ASSERT(result == 8192, "Should write 8192 bytes");

    // Read it back
    char *read_buffer = (char *)0x200000;
    result = sfs_read_file("/large.dat", 0, 8192, read_buffer);
    ASSERT(result == 8192, "Should read 8192 bytes");

    // Verify data
    bool match = true;
    for (int i = 0; i < 8192; i++) {
        if (read_buffer[i] != (char)(i & 0xFF)) {
            match = false;
            break;
        }
    }
    ASSERT(match, "Large file data should match");

    PASS();
}

/**
 * Test 9: List files
 */
static void test_sfs_list(void) {
    TEST("List files");

    kprintf("\n");
    sfs_list_files();

    // Just verify it doesn't crash
    PASS();
}

/**
 * Test 10: File not found
 */
static void test_sfs_not_found(void) {
    TEST("File not found");

    char buffer[64];
    int result = sfs_read_file("/nonexistent.txt", 0, 64, buffer);
    ASSERT(result == SFS_ERR_NOT_FOUND, "Should return not found error");

    result = sfs_write_file("/nonexistent.txt", 0, 10, "data");
    ASSERT(result == SFS_ERR_NOT_FOUND, "Write to non-existent should fail");

    PASS();
}

/**
 * Test 11: Unmount
 */
static void test_sfs_unmount(void) {
    TEST("Unmount filesystem");

    sfs_unmount();

    const sfs_state_t *state = sfs_get_state();
    ASSERT(state->mounted == false, "Filesystem should be unmounted");

    // Operations should fail when unmounted
    int result = sfs_create_file("/fail.txt", SFS_TYPE_FILE);
    ASSERT(result == SFS_ERR_NOT_MOUNTED, "Should fail when unmounted");

    PASS();
}

/**
 * Test 12: Remount and verify persistence
 */
static void test_sfs_remount(void) {
    TEST("Remount and verify persistence");

    // Check if drive 0 is present
    const ata_drive_t *drive = ata_get_drive_info(0);
    if (!drive || !drive->present) {
        FAIL("Drive 0 not present");
        return;
    }

    // Remount
    int result = sfs_mount(0, "/disk");
    ASSERT(result == SFS_ERR_SUCCESS, "Remount should succeed");

    // Verify files still exist
    char buffer[64];
    result = sfs_read_file("/test.txt", 0, 31, buffer);
    ASSERT(result == 31, "Should read file after remount");

    buffer[31] = '\0';
    ASSERT(strcmp(buffer, "Hello from SimpleFS! More data!") == 0,
           "File content should persist");

    PASS();
}

/**
 * Run all SimpleFS tests
 */
void test_simplefs_run_all(void) {
    kprintf("\n=== SimpleFS Tests ===\n");

    tests_run = 0;
    tests_passed = 0;

    // Check if we have a disk to test with
    const ata_drive_t *drive = ata_get_drive_info(0);
    if (!drive || !drive->present) {
        kprintf("[TEST] WARNING: No disk drive available, skipping filesystem tests\n");
        kprintf("[TEST] (Filesystem tests require a disk image in QEMU)\n");
        return;
    }

    // Run all tests in order
    test_sfs_init();
    test_sfs_format();
    test_sfs_mount();
    test_sfs_create();
    test_sfs_write();
    test_sfs_read();
    test_sfs_read_eof();
    test_sfs_large_file();
    test_sfs_list();
    test_sfs_not_found();
    test_sfs_unmount();
    test_sfs_remount();

    // Final unmount
    sfs_unmount();

    // Print summary
    kprintf("\n=== SimpleFS Test Summary ===\n");
    kprintf("Tests run: %d\n", tests_run);
    kprintf("Tests passed: %d\n", tests_passed);
    kprintf("Tests failed: %d\n", tests_run - tests_passed);

    if (tests_passed == tests_run) {
        kprintf("Result: ALL TESTS PASSED\n");
    } else {
        kprintf("Result: SOME TESTS FAILED\n");
    }
}
