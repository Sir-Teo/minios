#include "ps2_keyboard.h"
#include "../../kernel/kprintf.h"
#include <stddef.h>

// Port I/O functions
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Scancode Set 1 to ASCII mapping (US QWERTY)
static const char scancode_to_ascii[128] = {
    0,    0,   '1',  '2',  '3',  '4',  '5',  '6',   // 0x00-0x07
    '7',  '8', '9',  '0',  '-',  '=',  '\b', '\t',  // 0x08-0x0F
    'q',  'w', 'e',  'r',  't',  'y',  'u',  'i',   // 0x10-0x17
    'o',  'p', '[',  ']',  '\n', 0,    'a',  's',   // 0x18-0x1F (0x1D = Left Ctrl)
    'd',  'f', 'g',  'h',  'j',  'k',  'l',  ';',   // 0x20-0x27
    '\'', '`', 0,    '\\', 'z',  'x',  'c',  'v',   // 0x28-0x2F (0x2A = Left Shift)
    'b',  'n', 'm',  ',',  '.',  '/',  0,    '*',   // 0x30-0x37 (0x36 = Right Shift)
    0,    ' ', 0,    0,    0,    0,    0,    0,     // 0x38-0x3F (0x38 = Left Alt, 0x3A = Caps Lock)
    0,    0,   0,    0,    0,    0,    0,    '7',   // 0x40-0x47
    '8',  '9', '-',  '4',  '5',  '6',  '+',  '1',   // 0x48-0x4F
    '2',  '3', '0',  '.',  0,    0,    0,    0,     // 0x50-0x57
    0,    0,   0,    0,    0,    0,    0,    0      // 0x58-0x5F
};

// Shifted characters
static const char scancode_to_ascii_shift[128] = {
    0,    0,   '!',  '@',  '#',  '$',  '%',  '^',   // 0x00-0x07
    '&',  '*', '(',  ')',  '_',  '+',  '\b', '\t',  // 0x08-0x0F
    'Q',  'W', 'E',  'R',  'T',  'Y',  'U',  'I',   // 0x10-0x17
    'O',  'P', '{',  '}',  '\n', 0,    'A',  'S',   // 0x18-0x1F
    'D',  'F', 'G',  'H',  'J',  'K',  'L',  ':',   // 0x20-0x27
    '"',  '~', 0,    '|',  'Z',  'X',  'C',  'V',   // 0x28-0x2F
    'B',  'N', 'M',  '<',  '>',  '?',  0,    '*',   // 0x30-0x37
    0,    ' ', 0,    0,    0,    0,    0,    0,     // 0x38-0x3F
    0,    0,   0,    0,    0,    0,    0,    '7',   // 0x40-0x47
    '8',  '9', '-',  '4',  '5',  '6',  '+',  '1',   // 0x48-0x4F
    '2',  '3', '0',  '.',  0,    0,    0,    0,     // 0x50-0x57
    0,    0,   0,    0,    0,    0,    0,    0      // 0x58-0x5F
};

// Keyboard state
static struct {
    char buffer[KB_BUFFER_SIZE];
    uint16_t read_pos;
    uint16_t write_pos;
    uint8_t modifiers;
    bool extended;
} kb_state = {0};

/**
 * Check if keyboard controller is ready for reading
 */
static bool kb_can_read(void) {
    return (inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL) != 0;
}

/**
 * Check if keyboard controller is ready for writing
 */
static bool kb_can_write(void) {
    return (inb(KB_STATUS_PORT) & KB_STATUS_INPUT_FULL) == 0;
}

/**
 * Wait for keyboard to be ready for reading
 */
static void kb_wait_read(void) {
    int timeout = 100000;
    while (!kb_can_read() && timeout-- > 0);
}

/**
 * Wait for keyboard to be ready for writing
 */
static void kb_wait_write(void) {
    int timeout = 100000;
    while (!kb_can_write() && timeout-- > 0);
}

/**
 * Send a command to the keyboard
 */
static void kb_send_command(uint8_t command) {
    kb_wait_write();
    outb(KB_DATA_PORT, command);
}

/**
 * Read data from the keyboard
 */
static uint8_t kb_read_data(void) {
    kb_wait_read();
    return inb(KB_DATA_PORT);
}

/**
 * Add a character to the keyboard buffer
 */
static void kb_buffer_push(char c) {
    if (c == 0) return;  // Don't add null characters

    uint16_t next_pos = (kb_state.write_pos + 1) % KB_BUFFER_SIZE;

    // Check if buffer is full
    if (next_pos == kb_state.read_pos) {
        kprintf("[KB] Buffer full, dropping character\n");
        return;
    }

    kb_state.buffer[kb_state.write_pos] = c;
    kb_state.write_pos = next_pos;
}

/**
 * Get a character from the keyboard buffer
 */
static char kb_buffer_pop(void) {
    if (kb_state.read_pos == kb_state.write_pos) {
        return 0;  // Buffer is empty
    }

    char c = kb_state.buffer[kb_state.read_pos];
    kb_state.read_pos = (kb_state.read_pos + 1) % KB_BUFFER_SIZE;
    return c;
}

/**
 * Initialize the PS/2 keyboard driver
 */
void keyboard_init(void) {
    kprintf("[KB] Initializing PS/2 keyboard driver\n");

    // Clear state
    kb_state.read_pos = 0;
    kb_state.write_pos = 0;
    kb_state.modifiers = 0;
    kb_state.extended = false;

    // Drain the output buffer
    while (kb_can_read()) {
        inb(KB_DATA_PORT);
    }

    // Enable keyboard
    kb_send_command(KB_CMD_ENABLE);
    uint8_t response = kb_read_data();

    if (response == KB_RESP_ACK) {
        kprintf("[KB] Keyboard enabled successfully\n");
    } else {
        kprintf("[KB] Warning: Keyboard enable returned 0x%x\n", response);
    }

    // Set default LEDs (all off)
    keyboard_set_leds(0);

    kprintf("[KB] PS/2 keyboard driver initialized\n");
}

/**
 * Keyboard interrupt handler (IRQ1)
 */
void keyboard_irq_handler(void) {
    uint8_t scancode = inb(KB_DATA_PORT);

    // Handle extended scancodes
    if (scancode == KB_SC_EXTENDED) {
        kb_state.extended = true;
        return;
    }

    // Check if this is a key release (high bit set)
    bool released = (scancode & 0x80) != 0;
    uint8_t key = scancode & 0x7F;

    // Handle modifier keys
    if (!kb_state.extended) {
        switch (key) {
            case 0x2A:  // Left Shift
                if (released) {
                    kb_state.modifiers &= ~KB_FLAG_SHIFT_LEFT;
                } else {
                    kb_state.modifiers |= KB_FLAG_SHIFT_LEFT;
                }
                kb_state.extended = false;
                return;

            case 0x36:  // Right Shift
                if (released) {
                    kb_state.modifiers &= ~KB_FLAG_SHIFT_RIGHT;
                } else {
                    kb_state.modifiers |= KB_FLAG_SHIFT_RIGHT;
                }
                kb_state.extended = false;
                return;

            case 0x1D:  // Left Ctrl
                if (released) {
                    kb_state.modifiers &= ~KB_FLAG_CTRL_LEFT;
                } else {
                    kb_state.modifiers |= KB_FLAG_CTRL_LEFT;
                }
                kb_state.extended = false;
                return;

            case 0x38:  // Left Alt
                if (released) {
                    kb_state.modifiers &= ~KB_FLAG_ALT_LEFT;
                } else {
                    kb_state.modifiers |= KB_FLAG_ALT_LEFT;
                }
                kb_state.extended = false;
                return;

            case 0x3A:  // Caps Lock
                if (!released) {
                    kb_state.modifiers ^= KB_FLAG_CAPS_LOCK;
                    // Update LED
                    uint8_t leds = (kb_state.modifiers & KB_FLAG_CAPS_LOCK) ? 0x04 : 0x00;
                    keyboard_set_leds(leds);
                }
                kb_state.extended = false;
                return;
        }
    } else {
        // Extended scancode modifiers
        switch (key) {
            case 0x1D:  // Right Ctrl
                if (released) {
                    kb_state.modifiers &= ~KB_FLAG_CTRL_RIGHT;
                } else {
                    kb_state.modifiers |= KB_FLAG_CTRL_RIGHT;
                }
                kb_state.extended = false;
                return;

            case 0x38:  // Right Alt
                if (released) {
                    kb_state.modifiers &= ~KB_FLAG_ALT_RIGHT;
                } else {
                    kb_state.modifiers |= KB_FLAG_ALT_RIGHT;
                }
                kb_state.extended = false;
                return;
        }
    }

    kb_state.extended = false;

    // Ignore key releases for regular keys
    if (released) {
        return;
    }

    // Convert scancode to ASCII
    char c = 0;
    if (key < 128) {
        // Check if shift is pressed or caps lock is on
        bool shift = (kb_state.modifiers & KB_SHIFT) != 0;
        bool caps = (kb_state.modifiers & KB_FLAG_CAPS_LOCK) != 0;

        // For letters, caps lock toggles shift
        c = scancode_to_ascii[key];
        if (c >= 'a' && c <= 'z') {
            if (caps) {
                shift = !shift;
            }
        }

        if (shift) {
            c = scancode_to_ascii_shift[key];
        }
    }

    // Add character to buffer
    if (c != 0) {
        kb_buffer_push(c);
    }
}

/**
 * Get a character from the keyboard buffer (non-blocking)
 */
char keyboard_getchar(void) {
    return kb_buffer_pop();
}

/**
 * Get a character from the keyboard buffer (blocking)
 */
char keyboard_getchar_blocking(void) {
    char c;
    while ((c = kb_buffer_pop()) == 0) {
        __asm__ volatile("hlt");  // Wait for interrupt
    }
    return c;
}

/**
 * Check if keyboard buffer has data available
 */
bool keyboard_has_data(void) {
    return kb_state.read_pos != kb_state.write_pos;
}

/**
 * Clear the keyboard buffer
 */
void keyboard_clear_buffer(void) {
    kb_state.read_pos = 0;
    kb_state.write_pos = 0;
}

/**
 * Get current keyboard modifier state
 */
uint8_t keyboard_get_modifiers(void) {
    return kb_state.modifiers;
}

/**
 * Set keyboard LEDs
 */
void keyboard_set_leds(uint8_t leds) {
    kb_send_command(KB_CMD_SET_LEDS);
    kb_read_data();  // Wait for ACK
    kb_send_command(leds & 0x07);
    kb_read_data();  // Wait for ACK
}
