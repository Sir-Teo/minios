#ifndef PS2_KEYBOARD_H
#define PS2_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

// PS/2 Keyboard I/O Ports
#define KB_DATA_PORT    0x60  // Read/Write data
#define KB_STATUS_PORT  0x64  // Read status
#define KB_COMMAND_PORT 0x64  // Write commands

// Status Register Bits
#define KB_STATUS_OUTPUT_FULL  0x01  // Output buffer full (data available)
#define KB_STATUS_INPUT_FULL   0x02  // Input buffer full (command in progress)

// Keyboard Commands
#define KB_CMD_SET_LEDS        0xED  // Set LEDs
#define KB_CMD_ECHO            0xEE  // Echo
#define KB_CMD_SCANCODE_SET    0xF0  // Get/Set scancode set
#define KB_CMD_IDENTIFY        0xF2  // Identify keyboard
#define KB_CMD_SET_RATE        0xF3  // Set typematic rate/delay
#define KB_CMD_ENABLE          0xF4  // Enable scanning
#define KB_CMD_DISABLE         0xF5  // Disable scanning
#define KB_CMD_RESET           0xFF  // Reset keyboard

// Keyboard Responses
#define KB_RESP_ACK            0xFA  // Acknowledge
#define KB_RESP_RESEND         0xFE  // Resend request
#define KB_RESP_ERROR          0xFC  // Error

// Special Scancodes
#define KB_SC_EXTENDED         0xE0  // Extended scancode prefix
#define KB_SC_RELEASED         0xF0  // Key released prefix (scancode set 2)

// Keyboard buffer size
#define KB_BUFFER_SIZE         256

// Keyboard state flags
#define KB_FLAG_SHIFT_LEFT     0x01
#define KB_FLAG_SHIFT_RIGHT    0x02
#define KB_FLAG_CTRL_LEFT      0x04
#define KB_FLAG_CTRL_RIGHT     0x08
#define KB_FLAG_ALT_LEFT       0x10
#define KB_FLAG_ALT_RIGHT      0x20
#define KB_FLAG_CAPS_LOCK      0x40
#define KB_FLAG_NUM_LOCK       0x80

// Keyboard modifier helpers
#define KB_SHIFT (KB_FLAG_SHIFT_LEFT | KB_FLAG_SHIFT_RIGHT)
#define KB_CTRL  (KB_FLAG_CTRL_LEFT | KB_FLAG_CTRL_RIGHT)
#define KB_ALT   (KB_FLAG_ALT_LEFT | KB_FLAG_ALT_RIGHT)

/**
 * Initialize the PS/2 keyboard driver
 */
void keyboard_init(void);

/**
 * Get a character from the keyboard buffer (non-blocking)
 *
 * @return Character if available, 0 if buffer is empty
 */
char keyboard_getchar(void);

/**
 * Get a character from the keyboard buffer (blocking)
 * Waits until a character is available
 *
 * @return Character from keyboard
 */
char keyboard_getchar_blocking(void);

/**
 * Check if keyboard buffer has data available
 *
 * @return true if data is available, false otherwise
 */
bool keyboard_has_data(void);

/**
 * Clear the keyboard buffer
 */
void keyboard_clear_buffer(void);

/**
 * Get current keyboard modifier state
 *
 * @return Bitfield of KB_FLAG_* values
 */
uint8_t keyboard_get_modifiers(void);

/**
 * Set keyboard LEDs (Caps Lock, Num Lock, Scroll Lock)
 *
 * @param leds LED state (bit 0 = Scroll Lock, bit 1 = Num Lock, bit 2 = Caps Lock)
 */
void keyboard_set_leds(uint8_t leds);

/**
 * Keyboard interrupt handler (called from IRQ1)
 */
void keyboard_irq_handler(void);

#endif // PS2_KEYBOARD_H
