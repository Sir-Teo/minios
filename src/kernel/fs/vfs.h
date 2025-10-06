#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Maximum path length
#define VFS_MAX_PATH        256
#define VFS_MAX_NAME        64
#define VFS_MAX_OPEN_FILES  128

// File types
#define VFS_FILE            0x01
#define VFS_DIRECTORY       0x02
#define VFS_CHARDEVICE      0x03
#define VFS_BLOCKDEVICE     0x04
#define VFS_PIPE            0x05
#define VFS_SYMLINK         0x06
#define VFS_MOUNTPOINT      0x08

// File flags
#define VFS_FLAG_READ       0x01
#define VFS_FLAG_WRITE      0x02
#define VFS_FLAG_APPEND     0x04
#define VFS_FLAG_CREATE     0x08
#define VFS_FLAG_TRUNCATE   0x10
#define VFS_FLAG_EXCL       0x20

// Seek modes
#define VFS_SEEK_SET        0
#define VFS_SEEK_CUR        1
#define VFS_SEEK_END        2

// Error codes
#define VFS_ERR_SUCCESS     0
#define VFS_ERR_NOT_FOUND   -1
#define VFS_ERR_NO_MEM      -2
#define VFS_ERR_INVALID     -3
#define VFS_ERR_NOT_DIR     -4
#define VFS_ERR_IS_DIR      -5
#define VFS_ERR_EXISTS      -6
#define VFS_ERR_NO_SPACE    -7
#define VFS_ERR_READ_ONLY   -8
#define VFS_ERR_BAD_FD      -9
#define VFS_ERR_TOO_MANY    -10

// Forward declarations
struct vfs_node;
struct vfs_dirent;

// VFS operations structure (function pointers for filesystem-specific operations)
typedef struct vfs_operations {
    // File operations
    int (*open)(struct vfs_node *node, uint32_t flags);
    void (*close)(struct vfs_node *node);
    int64_t (*read)(struct vfs_node *node, uint64_t offset, uint64_t size, void *buffer);
    int64_t (*write)(struct vfs_node *node, uint64_t offset, uint64_t size, const void *buffer);

    // Directory operations
    struct vfs_node *(*finddir)(struct vfs_node *node, const char *name);
    int (*readdir)(struct vfs_node *node, uint32_t index, struct vfs_dirent *dirent);

    // Node operations
    int (*create)(struct vfs_node *parent, const char *name, uint32_t type);
    int (*unlink)(struct vfs_node *parent, const char *name);
    int (*mkdir)(struct vfs_node *parent, const char *name);
    int (*rmdir)(struct vfs_node *parent, const char *name);
} vfs_operations_t;

// VFS node (inode-like structure)
typedef struct vfs_node {
    char name[VFS_MAX_NAME];        // Node name
    uint32_t type;                  // File type (VFS_FILE, VFS_DIRECTORY, etc.)
    uint32_t permissions;           // Permissions (Unix-style)
    uint32_t uid;                   // User ID
    uint32_t gid;                   // Group ID
    uint64_t size;                  // Size in bytes
    uint64_t inode;                 // Inode number
    uint32_t flags;                 // Flags

    // Timestamps
    uint64_t atime;                 // Access time
    uint64_t mtime;                 // Modification time
    uint64_t ctime;                 // Creation time

    // VFS operations
    vfs_operations_t *ops;          // Filesystem-specific operations

    // Filesystem-specific data
    void *fs_data;                  // Filesystem-specific node data
    void *mount_data;               // Mount point data

    // Linked list (for directories)
    struct vfs_node *parent;        // Parent directory
    struct vfs_node *children;      // First child (for directories)
    struct vfs_node *next;          // Next sibling

    uint32_t refcount;              // Reference count
} vfs_node_t;

// Directory entry structure
typedef struct vfs_dirent {
    char name[VFS_MAX_NAME];        // Entry name
    uint64_t inode;                 // Inode number
    uint32_t type;                  // Entry type
} vfs_dirent_t;

// File descriptor structure
typedef struct vfs_fd {
    vfs_node_t *node;               // VFS node
    uint64_t offset;                // Current position
    uint32_t flags;                 // Open flags
    bool in_use;                    // Is this FD allocated?
} vfs_fd_t;

/**
 * Initialize the VFS subsystem
 */
void vfs_init(void);

/**
 * Open a file
 *
 * @param path File path
 * @param flags Open flags (VFS_FLAG_*)
 * @return File descriptor, or negative error code
 */
int vfs_open(const char *path, uint32_t flags);

/**
 * Close a file descriptor
 *
 * @param fd File descriptor
 * @return 0 on success, negative error code on failure
 */
int vfs_close(int fd);

/**
 * Read from a file
 *
 * @param fd File descriptor
 * @param buffer Buffer to read into
 * @param size Number of bytes to read
 * @return Number of bytes read, or negative error code
 */
int64_t vfs_read(int fd, void *buffer, uint64_t size);

/**
 * Write to a file
 *
 * @param fd File descriptor
 * @param buffer Buffer to write from
 * @param size Number of bytes to write
 * @return Number of bytes written, or negative error code
 */
int64_t vfs_write(int fd, const void *buffer, uint64_t size);

/**
 * Seek to a position in a file
 *
 * @param fd File descriptor
 * @param offset Offset to seek to
 * @param whence Seek mode (VFS_SEEK_SET, VFS_SEEK_CUR, VFS_SEEK_END)
 * @return New file position, or negative error code
 */
int64_t vfs_seek(int fd, int64_t offset, int whence);

/**
 * Get file status
 *
 * @param path File path
 * @param node Output node information
 * @return 0 on success, negative error code on failure
 */
int vfs_stat(const char *path, vfs_node_t *node);

/**
 * Create a directory
 *
 * @param path Directory path
 * @return 0 on success, negative error code on failure
 */
int vfs_mkdir(const char *path);

/**
 * Remove a directory
 *
 * @param path Directory path
 * @return 0 on success, negative error code on failure
 */
int vfs_rmdir(const char *path);

/**
 * Remove a file
 *
 * @param path File path
 * @return 0 on success, negative error code on failure
 */
int vfs_unlink(const char *path);

/**
 * Mount a filesystem
 *
 * @param source Source device/path
 * @param target Mount point path
 * @param fstype Filesystem type
 * @return 0 on success, negative error code on failure
 */
int vfs_mount(const char *source, const char *target, const char *fstype);

/**
 * Unmount a filesystem
 *
 * @param target Mount point path
 * @return 0 on success, negative error code on failure
 */
int vfs_unmount(const char *target);

/**
 * Get the root VFS node
 *
 * @return Root node
 */
vfs_node_t *vfs_get_root(void);

/**
 * Resolve a path to a VFS node
 *
 * @param path Path to resolve
 * @return VFS node, or NULL if not found
 */
vfs_node_t *vfs_resolve_path(const char *path);

/**
 * Create a new VFS node
 *
 * @param name Node name
 * @param type Node type
 * @return New node, or NULL on error
 */
vfs_node_t *vfs_create_node(const char *name, uint32_t type);

/**
 * Destroy a VFS node
 *
 * @param node Node to destroy
 */
void vfs_destroy_node(vfs_node_t *node);

/**
 * Add a child to a directory node
 *
 * @param parent Parent directory
 * @param child Child node
 * @return 0 on success, negative error code on failure
 */
int vfs_add_child(vfs_node_t *parent, vfs_node_t *child);

/**
 * Remove a child from a directory node
 *
 * @param parent Parent directory
 * @param child Child node
 * @return 0 on success, negative error code on failure
 */
int vfs_remove_child(vfs_node_t *parent, vfs_node_t *child);

#endif // VFS_H
