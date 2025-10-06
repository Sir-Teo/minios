#include "simplefs.h"
#include "vfs.h"
#include "../kprintf.h"
#include "../mm/kmalloc.h"
#include "../../drivers/disk/ata.h"
#include <stddef.h>

// Freestanding C library functions
extern void *memset(void *dest, int c, size_t n);
extern void *memcpy(void *restrict dest, const void *restrict src, size_t n);
extern int strcmp(const char *s1, const char *s2);
extern size_t strlen(const char *s);
extern char *strncpy(char *dest, const char *src, size_t n);

// Global filesystem state
static sfs_state_t g_sfs = {0};

// Helper: Calculate number of blocks needed for bitmap
static uint32_t bitmap_blocks(uint32_t num_items) {
    uint32_t bits = num_items;
    uint32_t bytes = (bits + 7) / 8;
    return (bytes + SFS_BLOCK_SIZE - 1) / SFS_BLOCK_SIZE;
}

// Helper: Set bit in bitmap
static void bitmap_set(uint8_t *bitmap, uint32_t bit) {
    bitmap[bit / 8] |= (1 << (bit % 8));
}

// Helper: Clear bit in bitmap
static void bitmap_clear(uint8_t *bitmap, uint32_t bit) {
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

// Helper: Test bit in bitmap
static bool bitmap_test(const uint8_t *bitmap, uint32_t bit) {
    return (bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

// Helper: Find first free bit in bitmap
static int32_t bitmap_find_free(const uint8_t *bitmap, uint32_t num_bits) {
    for (uint32_t i = 0; i < num_bits; i++) {
        if (!bitmap_test(bitmap, i)) {
            return i;
        }
    }
    return -1;
}

/**
 * Read a block from disk
 */
static int sfs_read_block(uint32_t block_num, void *buffer) {
    if (!g_sfs.mounted) {
        return SFS_ERR_NOT_MOUNTED;
    }

    uint64_t lba = block_num * SFS_SECTORS_PER_BLOCK;
    int result = ata_read_sectors(g_sfs.drive, lba, SFS_SECTORS_PER_BLOCK, buffer);

    return (result == 0) ? SFS_ERR_SUCCESS : SFS_ERR_IO;
}

/**
 * Write a block to disk
 */
static int sfs_write_block(uint32_t block_num, const void *buffer) {
    if (!g_sfs.mounted) {
        return SFS_ERR_NOT_MOUNTED;
    }

    uint64_t lba = block_num * SFS_SECTORS_PER_BLOCK;
    int result = ata_write_sectors(g_sfs.drive, lba, SFS_SECTORS_PER_BLOCK, buffer);

    return (result == 0) ? SFS_ERR_SUCCESS : SFS_ERR_IO;
}

/**
 * Allocate a data block
 */
static int32_t sfs_alloc_block(void) {
    int32_t block = bitmap_find_free(g_sfs.data_bitmap, g_sfs.sb.total_blocks);
    if (block < 0) {
        return SFS_ERR_NO_SPACE;
    }

    bitmap_set(g_sfs.data_bitmap, block);
    g_sfs.sb.free_blocks--;

    return block;
}

/**
 * Free a data block
 */
static void sfs_free_block(uint32_t block) {
    if (block < g_sfs.sb.total_blocks && bitmap_test(g_sfs.data_bitmap, block)) {
        bitmap_clear(g_sfs.data_bitmap, block);
        g_sfs.sb.free_blocks++;
    }
}

/**
 * Allocate an inode
 */
static int32_t sfs_alloc_inode(void) {
    int32_t inode_num = bitmap_find_free(g_sfs.inode_bitmap, g_sfs.sb.total_inodes);
    if (inode_num < 0) {
        return SFS_ERR_NO_SPACE;
    }

    bitmap_set(g_sfs.inode_bitmap, inode_num);
    g_sfs.sb.free_inodes--;

    return inode_num;
}

/**
 * Free an inode
 */
static void sfs_free_inode(uint32_t inode_num) {
    if (inode_num < g_sfs.sb.total_inodes && bitmap_test(g_sfs.inode_bitmap, inode_num)) {
        bitmap_clear(g_sfs.inode_bitmap, inode_num);
        g_sfs.sb.free_inodes++;
    }
}

/**
 * Read an inode from disk
 */
static int sfs_read_inode(uint32_t inode_num, sfs_inode_t *inode) {
    if (inode_num >= g_sfs.sb.total_inodes) {
        return SFS_ERR_INVALID;
    }

    // Calculate which block contains this inode
    uint32_t inodes_per_block = SFS_BLOCK_SIZE / sizeof(sfs_inode_t);
    uint32_t block = g_sfs.sb.inode_table_block + (inode_num / inodes_per_block);
    uint32_t offset = (inode_num % inodes_per_block) * sizeof(sfs_inode_t);

    // Read the block
    uint8_t *buffer = (uint8_t *)kmalloc(SFS_BLOCK_SIZE);
    if (!buffer) {
        return SFS_ERR_NO_MEM;
    }

    int result = sfs_read_block(block, buffer);
    if (result != SFS_ERR_SUCCESS) {
        kfree(buffer);
        return result;
    }

    // Copy the inode
    memcpy(inode, buffer + offset, sizeof(sfs_inode_t));
    kfree(buffer);

    return SFS_ERR_SUCCESS;
}

/**
 * Write an inode to disk
 */
static int sfs_write_inode(uint32_t inode_num, const sfs_inode_t *inode) {
    if (inode_num >= g_sfs.sb.total_inodes) {
        return SFS_ERR_INVALID;
    }

    // Calculate which block contains this inode
    uint32_t inodes_per_block = SFS_BLOCK_SIZE / sizeof(sfs_inode_t);
    uint32_t block = g_sfs.sb.inode_table_block + (inode_num / inodes_per_block);
    uint32_t offset = (inode_num % inodes_per_block) * sizeof(sfs_inode_t);

    // Read the block
    uint8_t *buffer = (uint8_t *)kmalloc(SFS_BLOCK_SIZE);
    if (!buffer) {
        return SFS_ERR_NO_MEM;
    }

    int result = sfs_read_block(block, buffer);
    if (result != SFS_ERR_SUCCESS) {
        kfree(buffer);
        return result;
    }

    // Update the inode
    memcpy(buffer + offset, inode, sizeof(sfs_inode_t));

    // Write back
    result = sfs_write_block(block, buffer);
    kfree(buffer);

    return result;
}

/**
 * Format a disk with SimpleFS
 */
int sfs_format(uint8_t drive, uint32_t total_blocks) {
    kprintf("[SIMPLEFS] Formatting drive %d...\n", drive);

    // Get drive info
    const ata_drive_t *drive_info = ata_get_drive_info(drive);
    if (!drive_info || !drive_info->present) {
        kprintf("[SIMPLEFS] ERROR: Drive %d not present\n", drive);
        return SFS_ERR_INVALID;
    }

    // Auto-detect total blocks if not specified
    if (total_blocks == 0) {
        // Use up to 512 MB or drive capacity, whichever is smaller
        uint64_t max_sectors = (512ULL * 1024 * 1024) / 512;  // 512 MB
        if (drive_info->sectors < max_sectors) {
            max_sectors = drive_info->sectors;
        }
        total_blocks = max_sectors / SFS_SECTORS_PER_BLOCK;
    }

    // Limit to SFS_MAX_BLOCKS
    if (total_blocks > SFS_MAX_BLOCKS) {
        total_blocks = SFS_MAX_BLOCKS;
    }

    kprintf("[SIMPLEFS] Total blocks: %u (%u MB)\n", total_blocks,
            (total_blocks * SFS_BLOCK_SIZE) / (1024 * 1024));

    // Calculate layout
    uint32_t inode_bitmap_blocks = bitmap_blocks(SFS_MAX_INODES);
    uint32_t data_bitmap_blocks = bitmap_blocks(total_blocks);
    uint32_t inodes_per_block = SFS_BLOCK_SIZE / sizeof(sfs_inode_t);
    uint32_t inode_table_blocks = (SFS_MAX_INODES + inodes_per_block - 1) / inodes_per_block;

    // Create superblock
    sfs_superblock_t sb = {0};
    sb.magic = SFS_MAGIC;
    sb.version = 1;
    sb.block_size = SFS_BLOCK_SIZE;
    sb.total_blocks = total_blocks;
    sb.total_inodes = SFS_MAX_INODES;
    sb.free_blocks = total_blocks - 1 - inode_bitmap_blocks - data_bitmap_blocks - inode_table_blocks;
    sb.free_inodes = SFS_MAX_INODES - 1;  // Root inode is allocated
    sb.inode_bitmap_block = 1;
    sb.data_bitmap_block = 1 + inode_bitmap_blocks;
    sb.inode_table_block = 1 + inode_bitmap_blocks + data_bitmap_blocks;
    sb.data_blocks_start = 1 + inode_bitmap_blocks + data_bitmap_blocks + inode_table_blocks;
    sb.drive_number = drive;

    kprintf("[SIMPLEFS] Layout:\n");
    kprintf("[SIMPLEFS]   Superblock: block 0\n");
    kprintf("[SIMPLEFS]   Inode bitmap: blocks %u-%u\n", sb.inode_bitmap_block,
            sb.inode_bitmap_block + inode_bitmap_blocks - 1);
    kprintf("[SIMPLEFS]   Data bitmap: blocks %u-%u\n", sb.data_bitmap_block,
            sb.data_bitmap_block + data_bitmap_blocks - 1);
    kprintf("[SIMPLEFS]   Inode table: blocks %u-%u\n", sb.inode_table_block,
            sb.inode_table_block + inode_table_blocks - 1);
    kprintf("[SIMPLEFS]   Data blocks: blocks %u-%u\n", sb.data_blocks_start, total_blocks - 1);
    kprintf("[SIMPLEFS]   Free blocks: %u\n", sb.free_blocks);

    // Write superblock
    uint8_t *buffer = (uint8_t *)kmalloc(SFS_BLOCK_SIZE);
    if (!buffer) {
        return SFS_ERR_NO_MEM;
    }

    memset(buffer, 0, SFS_BLOCK_SIZE);
    memcpy(buffer, &sb, sizeof(sfs_superblock_t));

    uint64_t lba = 0;
    int result = ata_write_sectors(drive, lba, SFS_SECTORS_PER_BLOCK, buffer);
    if (result != 0) {
        kfree(buffer);
        kprintf("[SIMPLEFS] ERROR: Failed to write superblock\n");
        return SFS_ERR_IO;
    }

    // Initialize and write inode bitmap (root inode allocated)
    memset(buffer, 0, SFS_BLOCK_SIZE);
    buffer[0] = 0x01;  // Root inode (inode 0) is allocated

    for (uint32_t i = 0; i < inode_bitmap_blocks; i++) {
        lba = (sb.inode_bitmap_block + i) * SFS_SECTORS_PER_BLOCK;
        result = ata_write_sectors(drive, lba, SFS_SECTORS_PER_BLOCK, buffer);
        if (result != 0) {
            kfree(buffer);
            return SFS_ERR_IO;
        }
        memset(buffer, 0, SFS_BLOCK_SIZE);  // Clear for next blocks
    }

    // Initialize and write data bitmap (all free)
    memset(buffer, 0, SFS_BLOCK_SIZE);

    for (uint32_t i = 0; i < data_bitmap_blocks; i++) {
        lba = (sb.data_bitmap_block + i) * SFS_SECTORS_PER_BLOCK;
        result = ata_write_sectors(drive, lba, SFS_SECTORS_PER_BLOCK, buffer);
        if (result != 0) {
            kfree(buffer);
            return SFS_ERR_IO;
        }
    }

    // Create root directory inode
    sfs_inode_t root_inode = {0};
    root_inode.type = SFS_TYPE_DIR;
    root_inode.size = 0;
    root_inode.blocks = 0;
    root_inode.links_count = 1;

    // Write root inode
    memset(buffer, 0, SFS_BLOCK_SIZE);
    memcpy(buffer, &root_inode, sizeof(sfs_inode_t));

    lba = sb.inode_table_block * SFS_SECTORS_PER_BLOCK;
    result = ata_write_sectors(drive, lba, SFS_SECTORS_PER_BLOCK, buffer);
    if (result != 0) {
        kfree(buffer);
        return SFS_ERR_IO;
    }

    // Clear remaining inode table blocks
    memset(buffer, 0, SFS_BLOCK_SIZE);
    for (uint32_t i = 1; i < inode_table_blocks; i++) {
        lba = (sb.inode_table_block + i) * SFS_SECTORS_PER_BLOCK;
        result = ata_write_sectors(drive, lba, SFS_SECTORS_PER_BLOCK, buffer);
        if (result != 0) {
            kfree(buffer);
            return SFS_ERR_IO;
        }
    }

    kfree(buffer);

    kprintf("[SIMPLEFS] Format complete!\n");
    return SFS_ERR_SUCCESS;
}

/**
 * Mount a SimpleFS filesystem
 */
int sfs_mount(uint8_t drive, const char *mount_point) {
    kprintf("[SIMPLEFS] Mounting drive %d at %s...\n", drive, mount_point);

    // Check if already mounted
    if (g_sfs.mounted) {
        kprintf("[SIMPLEFS] ERROR: Filesystem already mounted\n");
        return SFS_ERR_INVALID;
    }

    // Read superblock
    uint8_t *buffer = (uint8_t *)kmalloc(SFS_BLOCK_SIZE);
    if (!buffer) {
        return SFS_ERR_NO_MEM;
    }

    uint64_t lba = 0;
    int result = ata_read_sectors(drive, lba, SFS_SECTORS_PER_BLOCK, buffer);
    if (result != 0) {
        kfree(buffer);
        kprintf("[SIMPLEFS] ERROR: Failed to read superblock\n");
        return SFS_ERR_IO;
    }

    // Verify magic number
    sfs_superblock_t *sb = (sfs_superblock_t *)buffer;
    if (sb->magic != SFS_MAGIC) {
        kfree(buffer);
        kprintf("[SIMPLEFS] ERROR: Invalid magic number (expected 0x%x, got 0x%x)\n",
                SFS_MAGIC, sb->magic);
        return SFS_ERR_INVALID;
    }

    // Copy superblock
    memcpy(&g_sfs.sb, sb, sizeof(sfs_superblock_t));
    kfree(buffer);

    kprintf("[SIMPLEFS] Filesystem info:\n");
    kprintf("[SIMPLEFS]   Version: %u\n", g_sfs.sb.version);
    kprintf("[SIMPLEFS]   Block size: %u bytes\n", g_sfs.sb.block_size);
    kprintf("[SIMPLEFS]   Total blocks: %u\n", g_sfs.sb.total_blocks);
    kprintf("[SIMPLEFS]   Free blocks: %u\n", g_sfs.sb.free_blocks);
    kprintf("[SIMPLEFS]   Total inodes: %u\n", g_sfs.sb.total_inodes);
    kprintf("[SIMPLEFS]   Free inodes: %u\n", g_sfs.sb.free_inodes);

    // Allocate and load inode bitmap
    uint32_t inode_bitmap_blocks = bitmap_blocks(g_sfs.sb.total_inodes);
    uint32_t inode_bitmap_size = inode_bitmap_blocks * SFS_BLOCK_SIZE;
    g_sfs.inode_bitmap = (uint8_t *)kmalloc(inode_bitmap_size);
    if (!g_sfs.inode_bitmap) {
        return SFS_ERR_NO_MEM;
    }

    buffer = (uint8_t *)kmalloc(SFS_BLOCK_SIZE);
    if (!buffer) {
        kfree(g_sfs.inode_bitmap);
        g_sfs.inode_bitmap = NULL;
        return SFS_ERR_NO_MEM;
    }

    for (uint32_t i = 0; i < inode_bitmap_blocks; i++) {
        lba = (g_sfs.sb.inode_bitmap_block + i) * SFS_SECTORS_PER_BLOCK;
        result = ata_read_sectors(drive, lba, SFS_SECTORS_PER_BLOCK, buffer);
        if (result != 0) {
            kfree(buffer);
            kfree(g_sfs.inode_bitmap);
            g_sfs.inode_bitmap = NULL;
            return SFS_ERR_IO;
        }
        memcpy(g_sfs.inode_bitmap + i * SFS_BLOCK_SIZE, buffer, SFS_BLOCK_SIZE);
    }

    // Allocate and load data bitmap
    uint32_t data_bitmap_blocks = bitmap_blocks(g_sfs.sb.total_blocks);
    uint32_t data_bitmap_size = data_bitmap_blocks * SFS_BLOCK_SIZE;
    g_sfs.data_bitmap = (uint8_t *)kmalloc(data_bitmap_size);
    if (!g_sfs.data_bitmap) {
        kfree(buffer);
        kfree(g_sfs.inode_bitmap);
        g_sfs.inode_bitmap = NULL;
        return SFS_ERR_NO_MEM;
    }

    for (uint32_t i = 0; i < data_bitmap_blocks; i++) {
        lba = (g_sfs.sb.data_bitmap_block + i) * SFS_SECTORS_PER_BLOCK;
        result = ata_read_sectors(drive, lba, SFS_SECTORS_PER_BLOCK, buffer);
        if (result != 0) {
            kfree(buffer);
            kfree(g_sfs.inode_bitmap);
            kfree(g_sfs.data_bitmap);
            g_sfs.inode_bitmap = NULL;
            g_sfs.data_bitmap = NULL;
            return SFS_ERR_IO;
        }
        memcpy(g_sfs.data_bitmap + i * SFS_BLOCK_SIZE, buffer, SFS_BLOCK_SIZE);
    }

    kfree(buffer);

    g_sfs.drive = drive;
    g_sfs.mounted = true;

    kprintf("[SIMPLEFS] Mount successful!\n");
    return SFS_ERR_SUCCESS;
}

/**
 * Unmount the filesystem
 */
void sfs_unmount(void) {
    if (!g_sfs.mounted) {
        return;
    }

    kprintf("[SIMPLEFS] Unmounting filesystem...\n");

    // Free bitmaps
    if (g_sfs.inode_bitmap) {
        kfree(g_sfs.inode_bitmap);
        g_sfs.inode_bitmap = NULL;
    }

    if (g_sfs.data_bitmap) {
        kfree(g_sfs.data_bitmap);
        g_sfs.data_bitmap = NULL;
    }

    g_sfs.mounted = false;

    kprintf("[SIMPLEFS] Unmount complete\n");
}

/**
 * Get filesystem state
 */
const sfs_state_t *sfs_get_state(void) {
    return &g_sfs;
}

/**
 * Read directory entries from inode
 */
static int sfs_read_dir_entries(sfs_inode_t *inode, sfs_dirent_t **entries, uint32_t *count) {
    if (inode->type != SFS_TYPE_DIR) {
        return SFS_ERR_INVALID;
    }

    uint32_t num_entries = inode->size / sizeof(sfs_dirent_t);
    if (num_entries == 0) {
        *entries = NULL;
        *count = 0;
        return SFS_ERR_SUCCESS;
    }

    *entries = (sfs_dirent_t *)kmalloc(inode->size);
    if (!*entries) {
        return SFS_ERR_NO_MEM;
    }

    // Read directory data from blocks
    uint8_t *buffer = (uint8_t *)kmalloc(SFS_BLOCK_SIZE);
    if (!buffer) {
        kfree(*entries);
        return SFS_ERR_NO_MEM;
    }

    uint32_t bytes_read = 0;
    for (uint32_t i = 0; i < SFS_DIRECT_BLOCKS && bytes_read < inode->size; i++) {
        if (inode->direct[i] == 0) {
            break;
        }

        int result = sfs_read_block(g_sfs.sb.data_blocks_start + inode->direct[i], buffer);
        if (result != SFS_ERR_SUCCESS) {
            kfree(buffer);
            kfree(*entries);
            return result;
        }

        uint32_t bytes_to_copy = SFS_BLOCK_SIZE;
        if (bytes_read + bytes_to_copy > inode->size) {
            bytes_to_copy = inode->size - bytes_read;
        }

        memcpy((uint8_t *)(*entries) + bytes_read, buffer, bytes_to_copy);
        bytes_read += bytes_to_copy;
    }

    kfree(buffer);
    *count = num_entries;
    return SFS_ERR_SUCCESS;
}

/**
 * Find a directory entry by name
 */
static int sfs_find_dirent(uint32_t dir_inode_num, const char *name, uint32_t *out_inode) {
    sfs_inode_t dir_inode;
    int result = sfs_read_inode(dir_inode_num, &dir_inode);
    if (result != SFS_ERR_SUCCESS) {
        return result;
    }

    if (dir_inode.type != SFS_TYPE_DIR) {
        return SFS_ERR_INVALID;
    }

    // Read directory entries
    sfs_dirent_t *entries = NULL;
    uint32_t count = 0;
    result = sfs_read_dir_entries(&dir_inode, &entries, &count);
    if (result != SFS_ERR_SUCCESS) {
        return result;
    }

    // Search for entry
    for (uint32_t i = 0; i < count; i++) {
        if (entries[i].inode != 0 && strcmp(entries[i].name, name) == 0) {
            *out_inode = entries[i].inode;
            kfree(entries);
            return SFS_ERR_SUCCESS;
        }
    }

    kfree(entries);
    return SFS_ERR_NOT_FOUND;
}

/**
 * Create a new file in a directory
 */
int sfs_create_file(const char *path, uint32_t type) {
    if (!g_sfs.mounted) {
        return SFS_ERR_NOT_MOUNTED;
    }

    // For now, only support files in root directory
    if (path[0] != '/' || strlen(path) < 2) {
        return SFS_ERR_INVALID;
    }

    const char *filename = path + 1;

    // Check if file already exists
    uint32_t existing_inode;
    if (sfs_find_dirent(SFS_ROOT_INODE, filename, &existing_inode) == SFS_ERR_SUCCESS) {
        return SFS_ERR_EXISTS;
    }

    // Allocate inode
    int32_t inode_num = sfs_alloc_inode();
    if (inode_num < 0) {
        return inode_num;
    }

    // Create inode
    sfs_inode_t new_inode = {0};
    new_inode.type = type;
    new_inode.size = 0;
    new_inode.blocks = 0;
    new_inode.links_count = 1;

    int result = sfs_write_inode(inode_num, &new_inode);
    if (result != SFS_ERR_SUCCESS) {
        sfs_free_inode(inode_num);
        return result;
    }

    // Add directory entry to root
    sfs_inode_t root_inode;
    result = sfs_read_inode(SFS_ROOT_INODE, &root_inode);
    if (result != SFS_ERR_SUCCESS) {
        sfs_free_inode(inode_num);
        return result;
    }

    // Check if we need to allocate a new block for the directory
    uint32_t new_size = root_inode.size + sizeof(sfs_dirent_t);
    uint32_t blocks_needed = (new_size + SFS_BLOCK_SIZE - 1) / SFS_BLOCK_SIZE;

    if (blocks_needed > root_inode.blocks) {
        // Allocate new block
        if (root_inode.blocks >= SFS_DIRECT_BLOCKS) {
            sfs_free_inode(inode_num);
            return SFS_ERR_NO_SPACE;
        }

        int32_t block_num = sfs_alloc_block();
        if (block_num < 0) {
            sfs_free_inode(inode_num);
            return block_num;
        }

        root_inode.direct[root_inode.blocks] = block_num;
        root_inode.blocks++;
    }

    // Read the last block of the directory
    uint8_t *buffer = (uint8_t *)kmalloc(SFS_BLOCK_SIZE);
    if (!buffer) {
        sfs_free_inode(inode_num);
        return SFS_ERR_NO_MEM;
    }

    uint32_t last_block = root_inode.direct[root_inode.blocks - 1];
    result = sfs_read_block(g_sfs.sb.data_blocks_start + last_block, buffer);
    if (result != SFS_ERR_SUCCESS) {
        kfree(buffer);
        sfs_free_inode(inode_num);
        return result;
    }

    // Add new entry
    uint32_t offset = root_inode.size % SFS_BLOCK_SIZE;
    sfs_dirent_t *new_entry = (sfs_dirent_t *)(buffer + offset);
    new_entry->inode = inode_num;
    strncpy(new_entry->name, filename, SFS_MAX_FILENAME - 1);
    new_entry->name[SFS_MAX_FILENAME - 1] = '\0';

    // Write back
    result = sfs_write_block(g_sfs.sb.data_blocks_start + last_block, buffer);
    kfree(buffer);

    if (result != SFS_ERR_SUCCESS) {
        sfs_free_inode(inode_num);
        return result;
    }

    // Update root inode
    root_inode.size = new_size;
    result = sfs_write_inode(SFS_ROOT_INODE, &root_inode);
    if (result != SFS_ERR_SUCCESS) {
        sfs_free_inode(inode_num);
        return result;
    }

    return SFS_ERR_SUCCESS;
}

/**
 * Read file data
 */
int sfs_read_file(const char *path, uint64_t offset, uint64_t size, void *buffer) {
    if (!g_sfs.mounted) {
        return SFS_ERR_NOT_MOUNTED;
    }

    // Find file
    if (path[0] != '/' || strlen(path) < 2) {
        return SFS_ERR_INVALID;
    }

    const char *filename = path + 1;
    uint32_t inode_num;
    int result = sfs_find_dirent(SFS_ROOT_INODE, filename, &inode_num);
    if (result != SFS_ERR_SUCCESS) {
        return result;
    }

    // Read inode
    sfs_inode_t inode;
    result = sfs_read_inode(inode_num, &inode);
    if (result != SFS_ERR_SUCCESS) {
        return result;
    }

    if (inode.type != SFS_TYPE_FILE) {
        return SFS_ERR_INVALID;
    }

    // Check bounds
    if (offset >= inode.size) {
        return 0;  // EOF
    }

    if (offset + size > inode.size) {
        size = inode.size - offset;
    }

    // Read data from blocks
    uint8_t *block_buffer = (uint8_t *)kmalloc(SFS_BLOCK_SIZE);
    if (!block_buffer) {
        return SFS_ERR_NO_MEM;
    }

    uint64_t bytes_read = 0;
    while (bytes_read < size) {
        uint64_t current_offset = offset + bytes_read;
        uint32_t block_index = current_offset / SFS_BLOCK_SIZE;
        uint32_t block_offset = current_offset % SFS_BLOCK_SIZE;

        if (block_index >= SFS_DIRECT_BLOCKS || inode.direct[block_index] == 0) {
            break;
        }

        result = sfs_read_block(g_sfs.sb.data_blocks_start + inode.direct[block_index], block_buffer);
        if (result != SFS_ERR_SUCCESS) {
            kfree(block_buffer);
            return result;
        }

        uint32_t bytes_to_copy = SFS_BLOCK_SIZE - block_offset;
        if (bytes_read + bytes_to_copy > size) {
            bytes_to_copy = size - bytes_read;
        }

        memcpy((uint8_t *)buffer + bytes_read, block_buffer + block_offset, bytes_to_copy);
        bytes_read += bytes_to_copy;
    }

    kfree(block_buffer);
    return bytes_read;
}

/**
 * Write file data
 */
int sfs_write_file(const char *path, uint64_t offset, uint64_t size, const void *buffer) {
    if (!g_sfs.mounted) {
        return SFS_ERR_NOT_MOUNTED;
    }

    // Find file
    if (path[0] != '/' || strlen(path) < 2) {
        return SFS_ERR_INVALID;
    }

    const char *filename = path + 1;
    uint32_t inode_num;
    int result = sfs_find_dirent(SFS_ROOT_INODE, filename, &inode_num);
    if (result != SFS_ERR_SUCCESS) {
        return result;
    }

    // Read inode
    sfs_inode_t inode;
    result = sfs_read_inode(inode_num, &inode);
    if (result != SFS_ERR_SUCCESS) {
        return result;
    }

    if (inode.type != SFS_TYPE_FILE) {
        return SFS_ERR_INVALID;
    }

    // Calculate needed blocks
    uint64_t end_offset = offset + size;
    uint32_t blocks_needed = (end_offset + SFS_BLOCK_SIZE - 1) / SFS_BLOCK_SIZE;

    // Allocate additional blocks if needed
    while (inode.blocks < blocks_needed) {
        if (inode.blocks >= SFS_DIRECT_BLOCKS) {
            return SFS_ERR_NO_SPACE;  // File too large
        }

        int32_t block_num = sfs_alloc_block();
        if (block_num < 0) {
            return block_num;
        }

        inode.direct[inode.blocks] = block_num;
        inode.blocks++;
    }

    // Write data to blocks
    uint8_t *block_buffer = (uint8_t *)kmalloc(SFS_BLOCK_SIZE);
    if (!block_buffer) {
        return SFS_ERR_NO_MEM;
    }

    uint64_t bytes_written = 0;
    while (bytes_written < size) {
        uint64_t current_offset = offset + bytes_written;
        uint32_t block_index = current_offset / SFS_BLOCK_SIZE;
        uint32_t block_offset = current_offset % SFS_BLOCK_SIZE;

        if (block_index >= SFS_DIRECT_BLOCKS) {
            break;
        }

        // Read existing block if we're not writing a full block
        if (block_offset != 0 || (size - bytes_written) < SFS_BLOCK_SIZE) {
            result = sfs_read_block(g_sfs.sb.data_blocks_start + inode.direct[block_index], block_buffer);
            if (result != SFS_ERR_SUCCESS) {
                memset(block_buffer, 0, SFS_BLOCK_SIZE);  // Zero if read fails
            }
        }

        uint32_t bytes_to_copy = SFS_BLOCK_SIZE - block_offset;
        if (bytes_written + bytes_to_copy > size) {
            bytes_to_copy = size - bytes_written;
        }

        memcpy(block_buffer + block_offset, (const uint8_t *)buffer + bytes_written, bytes_to_copy);

        result = sfs_write_block(g_sfs.sb.data_blocks_start + inode.direct[block_index], block_buffer);
        if (result != SFS_ERR_SUCCESS) {
            kfree(block_buffer);
            return result;
        }

        bytes_written += bytes_to_copy;
    }

    kfree(block_buffer);

    // Update file size if necessary
    if (end_offset > inode.size) {
        inode.size = end_offset;
    }

    result = sfs_write_inode(inode_num, &inode);
    if (result != SFS_ERR_SUCCESS) {
        return result;
    }

    return bytes_written;
}

/**
 * List files in root directory
 */
void sfs_list_files(void) {
    if (!g_sfs.mounted) {
        kprintf("[SIMPLEFS] ERROR: Filesystem not mounted\n");
        return;
    }

    sfs_inode_t root_inode;
    int result = sfs_read_inode(SFS_ROOT_INODE, &root_inode);
    if (result != SFS_ERR_SUCCESS) {
        kprintf("[SIMPLEFS] ERROR: Failed to read root inode\n");
        return;
    }

    sfs_dirent_t *entries = NULL;
    uint32_t count = 0;
    result = sfs_read_dir_entries(&root_inode, &entries, &count);
    if (result != SFS_ERR_SUCCESS) {
        kprintf("[SIMPLEFS] ERROR: Failed to read directory entries\n");
        return;
    }

    kprintf("[SIMPLEFS] Files in root directory:\n");
    for (uint32_t i = 0; i < count; i++) {
        if (entries[i].inode != 0) {
            sfs_inode_t file_inode;
            result = sfs_read_inode(entries[i].inode, &file_inode);
            if (result == SFS_ERR_SUCCESS) {
                const char *type = (file_inode.type == SFS_TYPE_DIR) ? "DIR " : "FILE";
                kprintf("[SIMPLEFS]   %s  %8u bytes  %s\n", type, file_inode.size, entries[i].name);
            }
        }
    }

    kfree(entries);
}

/**
 * Initialize SimpleFS subsystem
 */
void sfs_init(void) {
    kprintf("[SIMPLEFS] Initializing SimpleFS subsystem\n");

    memset(&g_sfs, 0, sizeof(sfs_state_t));

    kprintf("[SIMPLEFS] Block size: %u bytes\n", SFS_BLOCK_SIZE);
    kprintf("[SIMPLEFS] Max inodes: %u\n", SFS_MAX_INODES);
    kprintf("[SIMPLEFS] Max filesystem size: %u MB\n",
            (SFS_MAX_BLOCKS * SFS_BLOCK_SIZE) / (1024 * 1024));

    kprintf("[SIMPLEFS] SimpleFS initialized\n");
}
