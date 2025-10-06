#include "../kernel/fs/vfs.h"
#include "../kernel/kprintf.h"
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
 * Test 1: VFS initialization
 */
static void test_vfs_init(void) {
    TEST("VFS initialization");

    vfs_node_t *root = vfs_get_root();
    ASSERT(root != NULL, "VFS root should not be NULL");
    ASSERT(root->type == VFS_DIRECTORY, "Root should be a directory");
    ASSERT(strcmp(root->name, "/") == 0, "Root name should be '/'");

    PASS();
}

/**
 * Test 2: Path resolution
 */
static void test_path_resolution(void) {
    TEST("Path resolution");

    // Resolve root
    vfs_node_t *root = vfs_resolve_path("/");
    ASSERT(root != NULL, "Should resolve root path");
    ASSERT(strcmp(root->name, "/") == 0, "Root path should resolve to root");

    // Resolve test file created by tmpfs
    vfs_node_t *file = vfs_resolve_path("/hello.txt");
    ASSERT(file != NULL, "Should resolve /hello.txt");
    ASSERT(strcmp(file->name, "hello.txt") == 0, "File name should match");
    ASSERT(file->type == VFS_FILE, "Should be a file");

    // Resolve non-existent file
    vfs_node_t *nonexistent = vfs_resolve_path("/nonexistent.txt");
    ASSERT(nonexistent == NULL, "Non-existent file should not resolve");

    PASS();
}

/**
 * Test 3: File open and close
 */
static void test_file_open_close(void) {
    TEST("File open and close");

    // Open file for reading
    int fd = vfs_open("/hello.txt", VFS_FLAG_READ);
    ASSERT(fd >= 0, "Should successfully open file");

    // Close file
    int result = vfs_close(fd);
    ASSERT(result == VFS_ERR_SUCCESS, "Should successfully close file");

    // Try to close invalid fd
    result = vfs_close(999);
    ASSERT(result == VFS_ERR_BAD_FD, "Should fail to close invalid FD");

    PASS();
}

/**
 * Test 4: File read
 */
static void test_file_read(void) {
    TEST("File read");

    // Open file
    int fd = vfs_open("/hello.txt", VFS_FLAG_READ);
    ASSERT(fd >= 0, "Should open file for reading");

    // Read from file
    char buffer[128];
    int64_t bytes_read = vfs_read(fd, buffer, sizeof(buffer) - 1);
    ASSERT(bytes_read > 0, "Should read bytes from file");

    buffer[bytes_read] = '\0';
    kprintf("\n[TEST]   Read: \"%s\"", buffer);

    // Verify content
    ASSERT(strcmp(buffer, "Hello from miniOS VFS!") == 0, "Content should match");

    // Close file
    vfs_close(fd);

    PASS();
}

/**
 * Test 5: File write
 */
static void test_file_write(void) {
    TEST("File write");

    // Open file for write
    int fd = vfs_open("/hello.txt", VFS_FLAG_READ | VFS_FLAG_WRITE);
    ASSERT(fd >= 0, "Should open file for writing");

    // Write to file
    const char *new_content = "Modified content!";
    int64_t bytes_written = vfs_write(fd, new_content, 17);
    ASSERT(bytes_written == 17, "Should write 17 bytes");

    // Close and reopen for reading
    vfs_close(fd);
    fd = vfs_open("/hello.txt", VFS_FLAG_READ);
    ASSERT(fd >= 0, "Should reopen file");

    // Read back and verify
    char buffer[128];
    int64_t bytes_read = vfs_read(fd, buffer, sizeof(buffer) - 1);
    buffer[bytes_read] = '\0';

    ASSERT(strcmp(buffer, "Modified content!") == 0, "Content should be modified");

    vfs_close(fd);

    PASS();
}

/**
 * Test 6: File seek
 */
static void test_file_seek(void) {
    TEST("File seek");

    // Open file
    int fd = vfs_open("/hello.txt", VFS_FLAG_READ);
    ASSERT(fd >= 0, "Should open file");

    // Seek to position 5
    int64_t new_pos = vfs_seek(fd, 5, VFS_SEEK_SET);
    ASSERT(new_pos == 5, "Should seek to position 5");

    // Read from new position
    char buffer[16];
    int64_t bytes_read = vfs_read(fd, buffer, 5);
    buffer[bytes_read] = '\0';

    kprintf("\n[TEST]   Read after seek: \"%s\"", buffer);
    ASSERT(strcmp(buffer, "fied ") == 0, "Should read from seeked position");

    // Seek to end
    new_pos = vfs_seek(fd, 0, VFS_SEEK_END);
    ASSERT(new_pos == 17, "Should seek to end");

    // Try to read (should get 0 bytes - EOF)
    bytes_read = vfs_read(fd, buffer, 10);
    ASSERT(bytes_read == 0, "Should get EOF at end of file");

    vfs_close(fd);

    PASS();
}

/**
 * Test 7: Multiple file descriptors
 */
static void test_multiple_fds(void) {
    TEST("Multiple file descriptors");

    // Open same file twice
    int fd1 = vfs_open("/hello.txt", VFS_FLAG_READ);
    int fd2 = vfs_open("/hello.txt", VFS_FLAG_READ);

    ASSERT(fd1 >= 0 && fd2 >= 0, "Should open file twice");
    ASSERT(fd1 != fd2, "FDs should be different");

    // They should have independent offsets
    char buffer1[16], buffer2[16];

    vfs_read(fd1, buffer1, 5);
    vfs_seek(fd2, 9, VFS_SEEK_SET);
    vfs_read(fd2, buffer2, 6);

    buffer1[5] = '\0';
    buffer2[6] = '\0';

    ASSERT(strcmp(buffer1, "Modif") == 0, "FD1 should read from start");
    ASSERT(strcmp(buffer2, "conten") == 0, "FD2 should read from offset 9");

    vfs_close(fd1);
    vfs_close(fd2);

    PASS();
}

/**
 * Test 8: vfs_stat
 */
static void test_vfs_stat(void) {
    TEST("VFS stat");

    vfs_node_t node_info;
    int result = vfs_stat("/hello.txt", &node_info);

    ASSERT(result == VFS_ERR_SUCCESS, "Should stat file successfully");
    ASSERT(strcmp(node_info.name, "hello.txt") == 0, "Name should match");
    ASSERT(node_info.type == VFS_FILE, "Type should be FILE");
    ASSERT(node_info.size == 17, "Size should be 17 bytes");

    // Stat non-existent file
    result = vfs_stat("/nonexistent", &node_info);
    ASSERT(result == VFS_ERR_NOT_FOUND, "Should fail for non-existent file");

    PASS();
}

/**
 * Run all VFS tests
 */
void test_vfs_run_all(void) {
    kprintf("\n=== VFS Tests ===\n");

    tests_run = 0;
    tests_passed = 0;

    // Run all tests
    test_vfs_init();
    test_path_resolution();
    test_file_open_close();
    test_file_read();
    test_file_write();
    test_file_seek();
    test_multiple_fds();
    test_vfs_stat();

    // Print summary
    kprintf("\n=== VFS Test Summary ===\n");
    kprintf("Tests run: %d\n", tests_run);
    kprintf("Tests passed: %d\n", tests_passed);
    kprintf("Tests failed: %d\n", tests_run - tests_passed);

    if (tests_passed == tests_run) {
        kprintf("Result: ALL TESTS PASSED\n");
    } else {
        kprintf("Result: SOME TESTS FAILED\n");
    }
}
