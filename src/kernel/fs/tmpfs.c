#include "vfs.h"
#include "../kprintf.h"
#include "../mm/kmalloc.h"
#include <stddef.h>

// Freestanding C library functions
extern void *memcpy(void *restrict dest, const void *restrict src, size_t n);
extern void *memset(void *dest, int c, size_t n);
extern size_t strlen(const char *s);

// tmpfs file data structure
typedef struct tmpfs_file {
    uint8_t *data;          // File contents
    uint64_t capacity;      // Allocated capacity
} tmpfs_file_t;

/**
 * tmpfs read operation
 */
static int64_t tmpfs_read(vfs_node_t *node, uint64_t offset, uint64_t size, void *buffer) {
    if (!node || !node->fs_data || !buffer) {
        return VFS_ERR_INVALID;
    }

    tmpfs_file_t *file = (tmpfs_file_t *)node->fs_data;

    // Check bounds
    if (offset >= node->size) {
        return 0;  // EOF
    }

    // Adjust size if reading past end
    if (offset + size > node->size) {
        size = node->size - offset;
    }

    // Copy data
    memcpy(buffer, file->data + offset, size);

    return size;
}

/**
 * tmpfs write operation
 */
static int64_t tmpfs_write(vfs_node_t *node, uint64_t offset, uint64_t size, const void *buffer) {
    if (!node || !node->fs_data || !buffer) {
        return VFS_ERR_INVALID;
    }

    tmpfs_file_t *file = (tmpfs_file_t *)node->fs_data;

    // Check if we need to expand the file
    uint64_t needed_size = offset + size;
    if (needed_size > file->capacity) {
        // Expand capacity (double it or use needed size, whichever is larger)
        uint64_t new_capacity = file->capacity * 2;
        if (new_capacity < needed_size) {
            new_capacity = needed_size;
        }

        uint8_t *new_data = (uint8_t *)kmalloc(new_capacity);
        if (!new_data) {
            return VFS_ERR_NO_MEM;
        }

        // Copy old data
        if (file->data) {
            memcpy(new_data, file->data, file->capacity);
            kfree(file->data);
        }

        file->data = new_data;
        file->capacity = new_capacity;
    }

    // Write data
    memcpy(file->data + offset, buffer, size);

    // Update size if we wrote past the end
    if (offset + size > node->size) {
        node->size = offset + size;
    }

    return size;
}

/**
 * tmpfs open operation
 */
static int tmpfs_open(vfs_node_t *node, uint32_t flags) {
    (void)flags;

    if (!node->fs_data) {
        // Create file data if it doesn't exist
        tmpfs_file_t *file = (tmpfs_file_t *)kmalloc(sizeof(tmpfs_file_t));
        if (!file) {
            return VFS_ERR_NO_MEM;
        }

        file->data = NULL;
        file->capacity = 0;

        node->fs_data = file;
    }

    return VFS_ERR_SUCCESS;
}

/**
 * tmpfs close operation
 */
static void tmpfs_close(vfs_node_t *node) {
    // Nothing special to do for tmpfs
    (void)node;
}

// tmpfs operations table
static vfs_operations_t tmpfs_ops = {
    .open = tmpfs_open,
    .close = tmpfs_close,
    .read = tmpfs_read,
    .write = tmpfs_write,
    .finddir = NULL,
    .readdir = NULL,
    .create = NULL,
    .unlink = NULL,
    .mkdir = NULL,
    .rmdir = NULL
};

/**
 * Create a tmpfs file node
 */
vfs_node_t *tmpfs_create_file(const char *name) {
    vfs_node_t *node = vfs_create_node(name, VFS_FILE);
    if (!node) {
        return NULL;
    }

    node->ops = &tmpfs_ops;
    node->permissions = 0644;

    return node;
}

/**
 * Initialize tmpfs and create some test files
 */
void tmpfs_init(void) {
    kprintf("[TMPFS] Initializing temporary filesystem\n");

    // Get root
    vfs_node_t *root = vfs_get_root();
    if (!root) {
        kprintf("[TMPFS] ERROR: VFS root not available\n");
        return;
    }

    // Create a test file
    vfs_node_t *test_file = tmpfs_create_file("hello.txt");
    if (test_file) {
        // Add to root
        vfs_add_child(root, test_file);

        // Write some initial data
        const char *content = "Hello from miniOS VFS!";
        tmpfs_file_t *file_data = (tmpfs_file_t *)kmalloc(sizeof(tmpfs_file_t));
        if (file_data) {
            file_data->capacity = 256;
            file_data->data = (uint8_t *)kmalloc(file_data->capacity);
            if (file_data->data) {
                size_t len = strlen(content);
                memcpy(file_data->data, content, len);
                test_file->size = len;
                test_file->fs_data = file_data;
            }
        }

        kprintf("[TMPFS] Created test file: /hello.txt\n");
    }

    kprintf("[TMPFS] tmpfs initialized\n");
}
