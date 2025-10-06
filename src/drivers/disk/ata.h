#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include <stdbool.h>

// ATA I/O Ports (Primary Bus)
#define ATA_PRIMARY_DATA        0x1F0  // Data register (R/W)
#define ATA_PRIMARY_ERROR       0x1F1  // Error register (R)
#define ATA_PRIMARY_FEATURES    0x1F1  // Features register (W)
#define ATA_PRIMARY_SECTOR_COUNT 0x1F2 // Sector count
#define ATA_PRIMARY_LBA_LOW     0x1F3  // LBA low byte
#define ATA_PRIMARY_LBA_MID     0x1F4  // LBA mid byte
#define ATA_PRIMARY_LBA_HIGH    0x1F5  // LBA high byte
#define ATA_PRIMARY_DRIVE       0x1F6  // Drive/Head register
#define ATA_PRIMARY_STATUS      0x1F7  // Status register (R)
#define ATA_PRIMARY_COMMAND     0x1F7  // Command register (W)
#define ATA_PRIMARY_CONTROL     0x3F6  // Device control (W)
#define ATA_PRIMARY_ALT_STATUS  0x3F6  // Alternate status (R)

// ATA I/O Ports (Secondary Bus)
#define ATA_SECONDARY_DATA      0x170
#define ATA_SECONDARY_ERROR     0x171
#define ATA_SECONDARY_FEATURES  0x171
#define ATA_SECONDARY_SECTOR_COUNT 0x172
#define ATA_SECONDARY_LBA_LOW   0x173
#define ATA_SECONDARY_LBA_MID   0x174
#define ATA_SECONDARY_LBA_HIGH  0x175
#define ATA_SECONDARY_DRIVE     0x176
#define ATA_SECONDARY_STATUS    0x177
#define ATA_SECONDARY_COMMAND   0x177
#define ATA_SECONDARY_CONTROL   0x376
#define ATA_SECONDARY_ALT_STATUS 0x376

// ATA Status Register Bits
#define ATA_STATUS_ERR   0x01  // Error
#define ATA_STATUS_IDX   0x02  // Index (obsolete)
#define ATA_STATUS_CORR  0x04  // Corrected data (obsolete)
#define ATA_STATUS_DRQ   0x08  // Data request ready
#define ATA_STATUS_SRV   0x10  // Service request (overlapped/queued)
#define ATA_STATUS_DF    0x20  // Drive fault
#define ATA_STATUS_RDY   0x40  // Drive ready
#define ATA_STATUS_BSY   0x80  // Busy

// ATA Error Register Bits
#define ATA_ERROR_AMNF   0x01  // Address mark not found
#define ATA_ERROR_TK0NF  0x02  // Track 0 not found
#define ATA_ERROR_ABRT   0x04  // Aborted command
#define ATA_ERROR_MCR    0x08  // Media change request
#define ATA_ERROR_IDNF   0x10  // ID not found
#define ATA_ERROR_MC     0x20  // Media changed
#define ATA_ERROR_UNC    0x40  // Uncorrectable data error
#define ATA_ERROR_BBK    0x80  // Bad block detected

// ATA Commands
#define ATA_CMD_READ_PIO        0x20  // Read sectors with retry
#define ATA_CMD_READ_PIO_EXT    0x24  // Read sectors ext (48-bit LBA)
#define ATA_CMD_WRITE_PIO       0x30  // Write sectors with retry
#define ATA_CMD_WRITE_PIO_EXT   0x34  // Write sectors ext (48-bit LBA)
#define ATA_CMD_CACHE_FLUSH     0xE7  // Flush cache
#define ATA_CMD_IDENTIFY        0xEC  // Identify device
#define ATA_CMD_IDENTIFY_PACKET 0xA1  // Identify packet device

// Drive Selection
#define ATA_DRIVE_MASTER        0xA0  // Select master drive
#define ATA_DRIVE_SLAVE         0xB0  // Select slave drive

// Sector size
#define ATA_SECTOR_SIZE         512

// ATA drive information
typedef struct {
    bool present;
    bool is_slave;
    uint16_t base_port;
    uint16_t control_port;
    uint64_t sectors;        // Total sectors (LBA28 or LBA48)
    bool lba48_supported;
    char model[41];          // Model string (40 chars + null)
    char serial[21];         // Serial number (20 chars + null)
} ata_drive_t;

/**
 * Initialize the ATA driver
 */
void ata_init(void);

/**
 * Read sectors from disk
 *
 * @param drive Drive number (0 = primary master, 1 = primary slave, etc.)
 * @param lba Logical Block Address
 * @param sectors Number of sectors to read
 * @param buffer Buffer to read into (must be at least sectors * 512 bytes)
 * @return 0 on success, negative error code on failure
 */
int ata_read_sectors(uint8_t drive, uint64_t lba, uint32_t sectors, void *buffer);

/**
 * Write sectors to disk
 *
 * @param drive Drive number (0 = primary master, 1 = primary slave, etc.)
 * @param lba Logical Block Address
 * @param sectors Number of sectors to write
 * @param buffer Buffer to write from (must be at least sectors * 512 bytes)
 * @return 0 on success, negative error code on failure
 */
int ata_write_sectors(uint8_t drive, uint64_t lba, uint32_t sectors, const void *buffer);

/**
 * Get drive information
 *
 * @param drive Drive number
 * @return Pointer to drive info, or NULL if drive doesn't exist
 */
const ata_drive_t *ata_get_drive_info(uint8_t drive);

/**
 * Print drive information for debugging
 */
void ata_print_drives(void);

#endif // ATA_H
