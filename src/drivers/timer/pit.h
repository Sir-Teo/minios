#ifndef DRIVERS_TIMER_PIT_H
#define DRIVERS_TIMER_PIT_H

#include <stdint.h>

// PIT (Programmable Interval Timer) I/O port addresses
#define PIT_CHANNEL_0  0x40  // Channel 0 data port (IRQ 0)
#define PIT_CHANNEL_1  0x41  // Channel 1 data port (refresh DRAM)
#define PIT_CHANNEL_2  0x42  // Channel 2 data port (PC speaker)
#define PIT_COMMAND    0x43  // Mode/Command register

// PIT command register bits
#define PIT_CMD_BINARY       0x00  // Use binary (not BCD) mode
#define PIT_CMD_MODE_0       0x00  // Mode 0: Interrupt on terminal count
#define PIT_CMD_MODE_2       0x04  // Mode 2: Rate generator (most common)
#define PIT_CMD_MODE_3       0x06  // Mode 3: Square wave generator
#define PIT_CMD_RW_LOW       0x10  // Read/Write low byte only
#define PIT_CMD_RW_HIGH      0x20  // Read/Write high byte only
#define PIT_CMD_RW_BOTH      0x30  // Read/Write low byte then high byte
#define PIT_CMD_CHANNEL_0    0x00  // Select channel 0
#define PIT_CMD_CHANNEL_1    0x40  // Select channel 1
#define PIT_CMD_CHANNEL_2    0x80  // Select channel 2

// PIT base frequency (1.193182 MHz)
#define PIT_BASE_FREQ    1193182

/**
 * Initialize the PIT to generate timer interrupts at the specified frequency.
 *
 * @param frequency Desired interrupt frequency in Hz (e.g., 100 for 100Hz)
 */
void pit_init(uint32_t frequency);

/**
 * Get the current tick count (number of timer interrupts since boot).
 *
 * @return Number of ticks
 */
uint64_t pit_get_ticks(void);

/**
 * Sleep for the specified number of ticks.
 *
 * @param ticks Number of ticks to sleep
 */
void pit_sleep(uint64_t ticks);

/**
 * Set a callback function to be called on each timer tick.
 *
 * @param callback Function pointer to call on each tick (NULL to disable)
 */
typedef void (*pit_callback_t)(void);
void pit_set_callback(pit_callback_t callback);

#endif
