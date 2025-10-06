#include "ata.h"
#include "../../kernel/kprintf.h"
#include <stddef.h>

// Freestanding C library functions
extern void *memcpy(void *restrict dest, const void *restrict src, size_t n);
extern void *memset(void *dest, int c, size_t n);

// Port I/O functions
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

// Delay function (400ns delay by reading alternate status)
static void ata_io_wait(uint16_t control_port) {
    for (int i = 0; i < 4; i++) {
        inb(control_port);
    }
}

// ATA drives (max 4: primary master/slave, secondary master/slave)
static ata_drive_t drives[4] = {0};

/**
 * Wait for drive to be ready (not busy)
 */
static bool ata_wait_ready(uint16_t status_port, uint32_t timeout_ms) {
    uint32_t timeout = timeout_ms * 1000;  // Approximate
    while (timeout-- > 0) {
        uint8_t status = inb(status_port);
        if (!(status & ATA_STATUS_BSY)) {
            return true;
        }
    }
    return false;
}

/**
 * Wait for drive to signal data ready
 */
static bool ata_wait_drq(uint16_t status_port, uint32_t timeout_ms) {
    uint32_t timeout = timeout_ms * 1000;  // Approximate
    while (timeout-- > 0) {
        uint8_t status = inb(status_port);
        if (status & ATA_STATUS_DRQ) {
            return true;
        }
        if (status & ATA_STATUS_ERR) {
            return false;
        }
    }
    return false;
}

/**
 * Identify an ATA drive
 */
static bool ata_identify(uint8_t drive_num) {
    ata_drive_t *drive = &drives[drive_num];
    uint16_t base = drive->base_port;
    uint16_t control = drive->control_port;

    // Select drive
    uint8_t drive_select = drive->is_slave ? ATA_DRIVE_SLAVE : ATA_DRIVE_MASTER;
    outb(base + 6, drive_select);
    ata_io_wait(control);

    // Send IDENTIFY command
    outb(base + 7, ATA_CMD_IDENTIFY);
    ata_io_wait(control);

    // Check if drive exists
    uint8_t status = inb(base + 7);
    if (status == 0) {
        return false;  // No drive
    }

    // Wait for BSY to clear
    if (!ata_wait_ready(base + 7, 1000)) {
        return false;
    }

    // Check for non-ATA drives (ATAPI)
    uint8_t lba_mid = inb(base + 4);
    uint8_t lba_high = inb(base + 5);
    if (lba_mid != 0 || lba_high != 0) {
        // ATAPI or other device, not supported
        return false;
    }

    // Wait for DRQ
    if (!ata_wait_drq(base + 7, 1000)) {
        return false;
    }

    // Read 256 words of identification data
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(base);
    }

    // Extract model string (words 27-46, 40 chars)
    for (int i = 0; i < 20; i++) {
        uint16_t word = identify_data[27 + i];
        drive->model[i * 2] = (word >> 8) & 0xFF;
        drive->model[i * 2 + 1] = word & 0xFF;
    }
    drive->model[40] = '\0';

    // Trim trailing spaces
    for (int i = 39; i >= 0 && drive->model[i] == ' '; i--) {
        drive->model[i] = '\0';
    }

    // Extract serial number (words 10-19, 20 chars)
    for (int i = 0; i < 10; i++) {
        uint16_t word = identify_data[10 + i];
        drive->serial[i * 2] = (word >> 8) & 0xFF;
        drive->serial[i * 2 + 1] = word & 0xFF;
    }
    drive->serial[20] = '\0';

    // Trim trailing spaces
    for (int i = 19; i >= 0 && drive->serial[i] == ' '; i--) {
        drive->serial[i] = '\0';
    }

    // Check for LBA48 support (bit 10 of word 83)
    drive->lba48_supported = (identify_data[83] & (1 << 10)) != 0;

    // Get number of sectors
    if (drive->lba48_supported) {
        // LBA48: 48-bit addressing (words 100-103)
        drive->sectors = ((uint64_t)identify_data[103] << 48) |
                        ((uint64_t)identify_data[102] << 32) |
                        ((uint64_t)identify_data[101] << 16) |
                        identify_data[100];
    } else {
        // LBA28: 28-bit addressing (words 60-61)
        drive->sectors = ((uint32_t)identify_data[61] << 16) | identify_data[60];
    }

    drive->present = true;
    return true;
}

/**
 * Initialize the ATA driver
 */
void ata_init(void) {
    kprintf("[ATA] Initializing ATA disk driver\n");

    // Initialize drive structures
    drives[0] = (ata_drive_t){
        .is_slave = false,
        .base_port = ATA_PRIMARY_DATA,
        .control_port = ATA_PRIMARY_CONTROL
    };
    drives[1] = (ata_drive_t){
        .is_slave = true,
        .base_port = ATA_PRIMARY_DATA,
        .control_port = ATA_PRIMARY_CONTROL
    };
    drives[2] = (ata_drive_t){
        .is_slave = false,
        .base_port = ATA_SECONDARY_DATA,
        .control_port = ATA_SECONDARY_CONTROL
    };
    drives[3] = (ata_drive_t){
        .is_slave = true,
        .base_port = ATA_SECONDARY_DATA,
        .control_port = ATA_SECONDARY_CONTROL
    };

    // Detect drives
    int drive_count = 0;
    for (int i = 0; i < 4; i++) {
        if (ata_identify(i)) {
            drive_count++;
            kprintf("[ATA] Drive %d detected: %s\n", i, drives[i].model);
            kprintf("[ATA]   Serial: %s\n", drives[i].serial);
            kprintf("[ATA]   Sectors: %lu (%lu MB)\n",
                    drives[i].sectors,
                    (drives[i].sectors * 512) / (1024 * 1024));
            kprintf("[ATA]   LBA48: %s\n", drives[i].lba48_supported ? "Yes" : "No");
        }
    }

    if (drive_count == 0) {
        kprintf("[ATA] No ATA drives detected\n");
    } else {
        kprintf("[ATA] ATA driver initialized, %d drive(s) found\n", drive_count);
    }
}

/**
 * Read sectors from disk using PIO mode
 */
int ata_read_sectors(uint8_t drive_num, uint64_t lba, uint32_t sectors, void *buffer) {
    if (drive_num >= 4 || !drives[drive_num].present) {
        return -1;  // Invalid drive
    }

    ata_drive_t *drive = &drives[drive_num];
    uint16_t base = drive->base_port;
    uint16_t control = drive->control_port;
    uint16_t *buf = (uint16_t *)buffer;

    // For simplicity, only support LBA28 for now (up to 128GB)
    if (lba >= 0x10000000 || !drive->lba48_supported) {
        if (lba >= 0x10000000) {
            kprintf("[ATA] Error: LBA %lu requires LBA48 support\n", lba);
            return -2;  // LBA too large for LBA28
        }
    }

    // Wait for drive to be ready
    if (!ata_wait_ready(base + 7, 1000)) {
        kprintf("[ATA] Error: Drive not ready for read\n");
        return -3;
    }

    // Select drive and set LBA mode
    uint8_t drive_select = (drive->is_slave ? 0x10 : 0x00) | 0xE0 | ((lba >> 24) & 0x0F);
    outb(base + 6, drive_select);
    ata_io_wait(control);

    // Send sector count and LBA
    outb(base + 2, (uint8_t)sectors);
    outb(base + 3, (uint8_t)(lba & 0xFF));
    outb(base + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(base + 5, (uint8_t)((lba >> 16) & 0xFF));

    // Send READ command
    outb(base + 7, ATA_CMD_READ_PIO);
    ata_io_wait(control);

    // Read each sector
    for (uint32_t i = 0; i < sectors; i++) {
        // Wait for DRQ
        if (!ata_wait_drq(base + 7, 1000)) {
            uint8_t error = inb(base + 1);
            kprintf("[ATA] Error reading sector %lu: error=0x%x\n", lba + i, error);
            return -4;
        }

        // Read 256 words (512 bytes)
        for (int j = 0; j < 256; j++) {
            buf[i * 256 + j] = inw(base);
        }

        // Small delay
        ata_io_wait(control);
    }

    return 0;  // Success
}

/**
 * Write sectors to disk using PIO mode
 */
int ata_write_sectors(uint8_t drive_num, uint64_t lba, uint32_t sectors, const void *buffer) {
    if (drive_num >= 4 || !drives[drive_num].present) {
        return -1;  // Invalid drive
    }

    ata_drive_t *drive = &drives[drive_num];
    uint16_t base = drive->base_port;
    uint16_t control = drive->control_port;
    const uint16_t *buf = (const uint16_t *)buffer;

    // For simplicity, only support LBA28 for now
    if (lba >= 0x10000000) {
        kprintf("[ATA] Error: LBA %lu requires LBA48 support\n", lba);
        return -2;  // LBA too large for LBA28
    }

    // Wait for drive to be ready
    if (!ata_wait_ready(base + 7, 1000)) {
        kprintf("[ATA] Error: Drive not ready for write\n");
        return -3;
    }

    // Select drive and set LBA mode
    uint8_t drive_select = (drive->is_slave ? 0x10 : 0x00) | 0xE0 | ((lba >> 24) & 0x0F);
    outb(base + 6, drive_select);
    ata_io_wait(control);

    // Send sector count and LBA
    outb(base + 2, (uint8_t)sectors);
    outb(base + 3, (uint8_t)(lba & 0xFF));
    outb(base + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(base + 5, (uint8_t)((lba >> 16) & 0xFF));

    // Send WRITE command
    outb(base + 7, ATA_CMD_WRITE_PIO);
    ata_io_wait(control);

    // Write each sector
    for (uint32_t i = 0; i < sectors; i++) {
        // Wait for DRQ
        if (!ata_wait_drq(base + 7, 1000)) {
            uint8_t error = inb(base + 1);
            kprintf("[ATA] Error writing sector %lu: error=0x%x\n", lba + i, error);
            return -4;
        }

        // Write 256 words (512 bytes)
        for (int j = 0; j < 256; j++) {
            outw(base, buf[i * 256 + j]);
        }

        // Small delay
        ata_io_wait(control);
    }

    // Flush cache
    outb(base + 7, ATA_CMD_CACHE_FLUSH);
    if (!ata_wait_ready(base + 7, 1000)) {
        kprintf("[ATA] Warning: Cache flush timeout\n");
    }

    return 0;  // Success
}

/**
 * Get drive information
 */
const ata_drive_t *ata_get_drive_info(uint8_t drive) {
    if (drive >= 4 || !drives[drive].present) {
        return NULL;
    }
    return &drives[drive];
}

/**
 * Print drive information for debugging
 */
void ata_print_drives(void) {
    kprintf("[ATA] Detected drives:\n");
    for (int i = 0; i < 4; i++) {
        if (drives[i].present) {
            kprintf("[ATA]   Drive %d: %s (%lu MB)\n",
                    i, drives[i].model,
                    (drives[i].sectors * 512) / (1024 * 1024));
        }
    }
}
