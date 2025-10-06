#ifndef SIMPLEFS_H
#define SIMPLEFS_H

#include <stdint.h>
#include <stdbool.h>
#include "vfs.h"

/**
 * SimpleFS - A simple filesystem for miniOS
 *
 * Layout:
 * - Block 0: Superblock
 * - Block 1-N: Inode bitmap (1 bit per inode)
 * - Block N+1-M: Data block bitmap (1 bit per block)
 * - Block M+1-P: Inode table
 * - Block P+1-END: Data blocks
 *
 * Block size: 4096 bytes (8 sectors)
 * Max file size: 16 MB (4096 direct blocks)
 * Max filesystem size: 512 MB
 */

#define SFS_MAGIC           0x53465330  // "SFS0"
#define SFS_BLOCK_SIZE      4096
#define SFS_SECTORS_PER_BLOCK 8
#define SFS_MAX_FILENAME    56
#define SFS_DIRECT_BLOCKS   12
#define SFS_INDIRECT_BLOCKS 1
#define SFS_BLOCKS_PER_INDIRECT (SFS_BLOCK_SIZE / sizeof(uint32_t))

// Filesystem limits
#define SFS_MAX_INODES      1024
#define SFS_MAX_BLOCKS      131072  // 512 MB / 4096 bytes

// Inode types
#define SFS_TYPE_FILE       1
#define SFS_TYPE_DIR        2

// Root inode number
#define SFS_ROOT_INODE      0

/**
 * Superblock - Block 0
 */
typedef struct {
    uint32_t magic;              // Magic number (SFS_MAGIC)
    uint32_t version;            // Filesystem version (1)
    uint32_t block_size;         // Block size in bytes
    uint32_t total_blocks;       // Total blocks in filesystem
    uint32_t total_inodes;       // Total inodes
    uint32_t free_blocks;        // Free data blocks
    uint32_t free_inodes;        // Free inodes
    uint32_t inode_bitmap_block; // First block of inode bitmap
    uint32_t data_bitmap_block;  // First block of data bitmap
    uint32_t inode_table_block;  // First block of inode table
    uint32_t data_blocks_start;  // First data block
    uint8_t  drive_number;       // ATA drive number
    uint8_t  reserved[475];      // Pad to 512 bytes
} __attribute__((packed)) sfs_superblock_t;

/**
 * Inode structure
 */
typedef struct {
    uint32_t type;               // File type (SFS_TYPE_FILE, SFS_TYPE_DIR)
    uint32_t size;               // File size in bytes
    uint32_t blocks;             // Number of blocks allocated
    uint32_t links_count;        // Hard link count
    uint32_t direct[SFS_DIRECT_BLOCKS];   // Direct block pointers
    uint32_t indirect;           // Single indirect block pointer
    uint32_t ctime;              // Creation time
    uint32_t mtime;              // Modification time
    uint8_t  reserved[32];       // Reserved for future use
} __attribute__((packed)) sfs_inode_t;

/**
 * Directory entry
 */
typedef struct {
    uint32_t inode;              // Inode number (0 = unused)
    char name[SFS_MAX_FILENAME]; // Filename (null-terminated)
} __attribute__((packed)) sfs_dirent_t;

/**
 * Filesystem state
 */
typedef struct {
    sfs_superblock_t sb;         // Cached superblock
    uint8_t *inode_bitmap;       // Inode allocation bitmap
    uint8_t *data_bitmap;        // Data block allocation bitmap
    uint8_t drive;               // ATA drive number
    bool mounted;                // Is filesystem mounted?
} sfs_state_t;

/**
 * Initialize SimpleFS subsystem
 */
void sfs_init(void);

/**
 * Format a disk with SimpleFS
 *
 * @param drive ATA drive number (0-3)
 * @param total_blocks Total blocks to use (0 = auto-detect)
 * @return 0 on success, negative on error
 */
int sfs_format(uint8_t drive, uint32_t total_blocks);

/**
 * Mount a SimpleFS filesystem
 *
 * @param drive ATA drive number (0-3)
 * @param mount_point VFS mount point (e.g., "/disk")
 * @return 0 on success, negative on error
 */
int sfs_mount(uint8_t drive, const char *mount_point);

/**
 * Unmount the filesystem
 */
void sfs_unmount(void);

/**
 * Get filesystem state
 */
const sfs_state_t *sfs_get_state(void);

/**
 * Create a file on the filesystem
 *
 * @param path File path (e.g., "/myfile.txt")
 * @param type File type (SFS_TYPE_FILE or SFS_TYPE_DIR)
 * @return 0 on success, negative on error
 */
int sfs_create_file(const char *path, uint32_t type);

/**
 * Read data from a file
 *
 * @param path File path
 * @param offset Offset in file
 * @param size Number of bytes to read
 * @param buffer Buffer to read into
 * @return Number of bytes read, or negative on error
 */
int sfs_read_file(const char *path, uint64_t offset, uint64_t size, void *buffer);

/**
 * Write data to a file
 *
 * @param path File path
 * @param offset Offset in file
 * @param size Number of bytes to write
 * @param buffer Buffer to write from
 * @return Number of bytes written, or negative on error
 */
int sfs_write_file(const char *path, uint64_t offset, uint64_t size, const void *buffer);

/**
 * List all files in root directory
 */
void sfs_list_files(void);

// Error codes
#define SFS_ERR_SUCCESS      0
#define SFS_ERR_INVALID     -1
#define SFS_ERR_NO_MEM      -2
#define SFS_ERR_IO          -3
#define SFS_ERR_NOT_FOUND   -4
#define SFS_ERR_EXISTS      -5
#define SFS_ERR_NO_SPACE    -6
#define SFS_ERR_NOT_MOUNTED -7

#endif // SIMPLEFS_H
