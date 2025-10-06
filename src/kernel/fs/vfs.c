#include "vfs.h"
#include "../kprintf.h"
#include "../mm/kmalloc.h"
#include <stddef.h>

// Freestanding C library functions
extern void *memset(void *dest, int c, size_t n);
extern void *memcpy(void *restrict dest, const void *restrict src, size_t n);
extern size_t strlen(const char *s);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern char *strcpy(char *restrict dest, const char *restrict src);

// VFS state
static vfs_node_t *vfs_root = NULL;
static vfs_fd_t vfs_fd_table[VFS_MAX_OPEN_FILES];

/**
 * Initialize the VFS subsystem
 */
void vfs_init(void) {
    kprintf("[VFS] Initializing Virtual File System\n");

    // Clear file descriptor table
    memset(vfs_fd_table, 0, sizeof(vfs_fd_table));

    // Create root directory
    vfs_root = vfs_create_node("/", VFS_DIRECTORY);
    if (!vfs_root) {
        kprintf("[VFS] ERROR: Failed to create root directory\n");
        return;
    }

    vfs_root->permissions = 0755;
    vfs_root->parent = vfs_root;  // Root is its own parent

    kprintf("[VFS] VFS initialized successfully\n");
}

/**
 * Create a new VFS node
 */
vfs_node_t *vfs_create_node(const char *name, uint32_t type) {
    vfs_node_t *node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    if (!node) {
        return NULL;
    }

    memset(node, 0, sizeof(vfs_node_t));

    // Copy name
    size_t name_len = strlen(name);
    if (name_len >= VFS_MAX_NAME) {
        name_len = VFS_MAX_NAME - 1;
    }
    memcpy(node->name, name, name_len);
    node->name[name_len] = '\0';

    node->type = type;
    node->permissions = 0644;  // Default permissions
    node->refcount = 1;

    return node;
}

/**
 * Destroy a VFS node
 */
void vfs_destroy_node(vfs_node_t *node) {
    if (!node) {
        return;
    }

    node->refcount--;
    if (node->refcount == 0) {
        kfree(node);
    }
}

/**
 * Add a child to a directory node
 */
int vfs_add_child(vfs_node_t *parent, vfs_node_t *child) {
    if (!parent || !child) {
        return VFS_ERR_INVALID;
    }

    if (parent->type != VFS_DIRECTORY) {
        return VFS_ERR_NOT_DIR;
    }

    // Set parent
    child->parent = parent;

    // Add to children list
    child->next = parent->children;
    parent->children = child;

    return VFS_ERR_SUCCESS;
}

/**
 * Remove a child from a directory node
 */
int vfs_remove_child(vfs_node_t *parent, vfs_node_t *child) {
    if (!parent || !child) {
        return VFS_ERR_INVALID;
    }

    if (parent->type != VFS_DIRECTORY) {
        return VFS_ERR_NOT_DIR;
    }

    // Find and remove from children list
    vfs_node_t **current = &parent->children;
    while (*current) {
        if (*current == child) {
            *current = child->next;
            child->parent = NULL;
            child->next = NULL;
            return VFS_ERR_SUCCESS;
        }
        current = &(*current)->next;
    }

    return VFS_ERR_NOT_FOUND;
}

/**
 * Find a child by name in a directory
 */
static vfs_node_t *vfs_find_child(vfs_node_t *parent, const char *name) {
    if (!parent || parent->type != VFS_DIRECTORY) {
        return NULL;
    }

    vfs_node_t *child = parent->children;
    while (child) {
        if (strcmp(child->name, name) == 0) {
            return child;
        }
        child = child->next;
    }

    return NULL;
}

/**
 * Resolve a path to a VFS node
 */
vfs_node_t *vfs_resolve_path(const char *path) {
    if (!path || !vfs_root) {
        return NULL;
    }

    // Handle root
    if (strcmp(path, "/") == 0) {
        return vfs_root;
    }

    // Start from root
    vfs_node_t *current = vfs_root;

    // Skip leading slash
    if (path[0] == '/') {
        path++;
    }

    // Parse path components
    char component[VFS_MAX_NAME];
    while (*path) {
        // Extract next component
        size_t i = 0;
        while (*path && *path != '/' && i < VFS_MAX_NAME - 1) {
            component[i++] = *path++;
        }
        component[i] = '\0';

        // Skip trailing slash
        if (*path == '/') {
            path++;
        }

        // Empty component (double slash or trailing slash)
        if (i == 0) {
            continue;
        }

        // Find child
        current = vfs_find_child(current, component);
        if (!current) {
            return NULL;  // Path component not found
        }
    }

    return current;
}

/**
 * Get the root VFS node
 */
vfs_node_t *vfs_get_root(void) {
    return vfs_root;
}

/**
 * Allocate a file descriptor
 */
static int vfs_alloc_fd(void) {
    for (int i = 0; i < VFS_MAX_OPEN_FILES; i++) {
        if (!vfs_fd_table[i].in_use) {
            vfs_fd_table[i].in_use = true;
            return i;
        }
    }
    return VFS_ERR_TOO_MANY;
}

/**
 * Free a file descriptor
 */
static void vfs_free_fd(int fd) {
    if (fd >= 0 && fd < VFS_MAX_OPEN_FILES) {
        vfs_fd_table[fd].in_use = false;
        vfs_fd_table[fd].node = NULL;
        vfs_fd_table[fd].offset = 0;
        vfs_fd_table[fd].flags = 0;
    }
}

/**
 * Open a file
 */
int vfs_open(const char *path, uint32_t flags) {
    if (!path) {
        return VFS_ERR_INVALID;
    }

    // Resolve path
    vfs_node_t *node = vfs_resolve_path(path);

    if (!node) {
        // File doesn't exist
        if (flags & VFS_FLAG_CREATE) {
            // TODO: Create the file
            return VFS_ERR_NOT_FOUND;  // Not implemented yet
        }
        return VFS_ERR_NOT_FOUND;
    }

    // Check if it's a directory
    if (node->type == VFS_DIRECTORY) {
        return VFS_ERR_IS_DIR;
    }

    // Allocate file descriptor
    int fd = vfs_alloc_fd();
    if (fd < 0) {
        return fd;
    }

    // Initialize file descriptor
    vfs_fd_table[fd].node = node;
    vfs_fd_table[fd].offset = 0;
    vfs_fd_table[fd].flags = flags;

    // Increment reference count
    node->refcount++;

    // Call filesystem-specific open if available
    if (node->ops && node->ops->open) {
        int result = node->ops->open(node, flags);
        if (result < 0) {
            vfs_free_fd(fd);
            node->refcount--;
            return result;
        }
    }

    return fd;
}

/**
 * Close a file descriptor
 */
int vfs_close(int fd) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES || !vfs_fd_table[fd].in_use) {
        return VFS_ERR_BAD_FD;
    }

    vfs_node_t *node = vfs_fd_table[fd].node;

    // Call filesystem-specific close if available
    if (node && node->ops && node->ops->close) {
        node->ops->close(node);
    }

    // Decrement reference count
    if (node) {
        node->refcount--;
    }

    // Free file descriptor
    vfs_free_fd(fd);

    return VFS_ERR_SUCCESS;
}

/**
 * Read from a file
 */
int64_t vfs_read(int fd, void *buffer, uint64_t size) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES || !vfs_fd_table[fd].in_use) {
        return VFS_ERR_BAD_FD;
    }

    vfs_fd_t *file = &vfs_fd_table[fd];
    vfs_node_t *node = file->node;

    if (!node) {
        return VFS_ERR_INVALID;
    }

    // Check if opened for reading
    if (!(file->flags & VFS_FLAG_READ)) {
        return VFS_ERR_INVALID;
    }

    // Call filesystem-specific read if available
    if (node->ops && node->ops->read) {
        int64_t bytes_read = node->ops->read(node, file->offset, size, buffer);
        if (bytes_read > 0) {
            file->offset += bytes_read;
        }
        return bytes_read;
    }

    return VFS_ERR_INVALID;
}

/**
 * Write to a file
 */
int64_t vfs_write(int fd, const void *buffer, uint64_t size) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES || !vfs_fd_table[fd].in_use) {
        return VFS_ERR_BAD_FD;
    }

    vfs_fd_t *file = &vfs_fd_table[fd];
    vfs_node_t *node = file->node;

    if (!node) {
        return VFS_ERR_INVALID;
    }

    // Check if opened for writing
    if (!(file->flags & VFS_FLAG_WRITE)) {
        return VFS_ERR_INVALID;
    }

    // Call filesystem-specific write if available
    if (node->ops && node->ops->write) {
        int64_t bytes_written = node->ops->write(node, file->offset, size, buffer);
        if (bytes_written > 0) {
            file->offset += bytes_written;
            if (file->offset > node->size) {
                node->size = file->offset;
            }
        }
        return bytes_written;
    }

    return VFS_ERR_INVALID;
}

/**
 * Seek to a position in a file
 */
int64_t vfs_seek(int fd, int64_t offset, int whence) {
    if (fd < 0 || fd >= VFS_MAX_OPEN_FILES || !vfs_fd_table[fd].in_use) {
        return VFS_ERR_BAD_FD;
    }

    vfs_fd_t *file = &vfs_fd_table[fd];
    vfs_node_t *node = file->node;

    if (!node) {
        return VFS_ERR_INVALID;
    }

    int64_t new_offset;

    switch (whence) {
        case VFS_SEEK_SET:
            new_offset = offset;
            break;

        case VFS_SEEK_CUR:
            new_offset = file->offset + offset;
            break;

        case VFS_SEEK_END:
            new_offset = node->size + offset;
            break;

        default:
            return VFS_ERR_INVALID;
    }

    if (new_offset < 0) {
        return VFS_ERR_INVALID;
    }

    file->offset = new_offset;
    return new_offset;
}

/**
 * Get file status
 */
int vfs_stat(const char *path, vfs_node_t *out_node) {
    if (!path || !out_node) {
        return VFS_ERR_INVALID;
    }

    vfs_node_t *node = vfs_resolve_path(path);
    if (!node) {
        return VFS_ERR_NOT_FOUND;
    }

    // Copy node information
    memcpy(out_node, node, sizeof(vfs_node_t));

    return VFS_ERR_SUCCESS;
}

/**
 * Create a directory
 */
int vfs_mkdir(const char *path) {
    // TODO: Implement directory creation
    (void)path;
    return VFS_ERR_INVALID;
}

/**
 * Remove a directory
 */
int vfs_rmdir(const char *path) {
    // TODO: Implement directory removal
    (void)path;
    return VFS_ERR_INVALID;
}

/**
 * Remove a file
 */
int vfs_unlink(const char *path) {
    // TODO: Implement file removal
    (void)path;
    return VFS_ERR_INVALID;
}

/**
 * Mount a filesystem
 */
int vfs_mount(const char *source, const char *target, const char *fstype) {
    // TODO: Implement filesystem mounting
    (void)source;
    (void)target;
    (void)fstype;
    return VFS_ERR_INVALID;
}

/**
 * Unmount a filesystem
 */
int vfs_unmount(const char *target) {
    // TODO: Implement filesystem unmounting
    (void)target;
    return VFS_ERR_INVALID;
}
