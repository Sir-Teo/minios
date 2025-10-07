#include "shell.h"
#include "../kprintf.h"
#include "../fs/vfs.h"
#include "../fs/simplefs.h"
#include "../../drivers/keyboard/ps2_keyboard.h"
#include <stddef.h>

// Freestanding C library functions
extern void *memset(void *dest, int c, size_t n);
extern int strcmp(const char *s1, const char *s2);
extern size_t strlen(const char *s);
extern char *strchr(const char *s, int c);

// Serial output (for shell output)
extern void serial_write(const char *s);

// Shell state
static char input_buffer[SHELL_MAX_INPUT];
static char history[SHELL_HISTORY_SIZE][SHELL_MAX_INPUT];
static int history_index = 0;
static int history_count = 0;

/**
 * Built-in command: help
 */
static int cmd_help(int argc, char **argv) {
    (void)argc;
    (void)argv;

    kprintf("miniOS Shell - Built-in Commands:\n");
    kprintf("  help              - Display this help message\n");
    kprintf("  clear             - Clear the screen\n");
    kprintf("  echo <text>       - Echo text to console\n");
    kprintf("  uname             - Display system information\n");
    kprintf("  uptime            - Show system uptime\n");
    kprintf("  free              - Display memory information\n");
    kprintf("  ls                - List files in filesystem\n");
    kprintf("  cat <file>        - Display file contents\n");
    kprintf("  create <file>     - Create a new file\n");
    kprintf("  write <file> <data> - Write data to a file\n");
    kprintf("  mkdir <dir>       - Create directory (not yet implemented)\n");
    kprintf("  rm <file>         - Remove file (not yet implemented)\n");
    kprintf("  mount             - Mount filesystem\n");
    kprintf("  unmount           - Unmount filesystem\n");
    kprintf("  format            - Format disk with SimpleFS\n");
    kprintf("  reboot            - Reboot the system (not yet implemented)\n");
    kprintf("  shutdown          - Halt the system\n");

    return 0;
}

/**
 * Built-in command: clear
 */
static int cmd_clear(int argc, char **argv) {
    (void)argc;
    (void)argv;

    // Send ANSI escape code to clear screen
    kprintf("\033[2J\033[H");

    return 0;
}

/**
 * Built-in command: echo
 */
static int cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        kprintf("%s", argv[i]);
        if (i < argc - 1) {
            kprintf(" ");
        }
    }
    kprintf("\n");

    return 0;
}

/**
 * Built-in command: uname
 */
static int cmd_uname(int argc, char **argv) {
    (void)argc;
    (void)argv;

    kprintf("miniOS x86_64 v0.11.0\n");
    kprintf("A modern operating system from scratch\n");

    return 0;
}

/**
 * Built-in command: uptime
 */
static int cmd_uptime(int argc, char **argv) {
    (void)argc;
    (void)argv;

    extern uint64_t pit_get_ticks(void);

    uint64_t ticks = pit_get_ticks();
    uint64_t seconds = ticks / 100;  // Assuming 100Hz timer
    uint64_t minutes = seconds / 60;
    uint64_t hours = minutes / 60;

    seconds %= 60;
    minutes %= 60;

    kprintf("Uptime: %llu:%02llu:%02llu (%llu ticks)\n", hours, minutes, seconds, ticks);

    return 0;
}

/**
 * Built-in command: free
 */
static int cmd_free(int argc, char **argv) {
    (void)argc;
    (void)argv;

    extern uint64_t pmm_get_total_memory(void);
    extern uint64_t pmm_get_free_memory(void);

    uint64_t total = pmm_get_total_memory();
    uint64_t free = pmm_get_free_memory();
    uint64_t used = total - free;

    kprintf("Memory:\n");
    kprintf("  Total: %llu MiB\n", total / 1024 / 1024);
    kprintf("  Used:  %llu MiB\n", used / 1024 / 1024);
    kprintf("  Free:  %llu MiB\n", free / 1024 / 1024);

    return 0;
}

/**
 * Built-in command: ls
 */
static int cmd_ls(int argc, char **argv) {
    (void)argc;
    (void)argv;

    const sfs_state_t *state = sfs_get_state();

    if (!state || !state->mounted) {
        kprintf("Error: No filesystem mounted\n");
        kprintf("Use 'mount' to mount a filesystem first\n");
        return 1;
    }

    sfs_list_files();

    return 0;
}

/**
 * Built-in command: cat
 */
static int cmd_cat(int argc, char **argv) {
    if (argc < 2) {
        kprintf("Usage: cat <file>\n");
        return 1;
    }

    const char *filename = argv[1];
    char path[64];

    // Prepend "/" if not present
    if (filename[0] != '/') {
        path[0] = '/';
        size_t i;
        for (i = 0; i < 62 && filename[i] != '\0'; i++) {
            path[i + 1] = filename[i];
        }
        path[i + 1] = '\0';
    } else {
        size_t i;
        for (i = 0; i < 63 && filename[i] != '\0'; i++) {
            path[i] = filename[i];
        }
        path[i] = '\0';
    }

    // Read file
    char buffer[4096];
    int bytes_read = sfs_read_file(path, 0, sizeof(buffer) - 1, buffer);

    if (bytes_read < 0) {
        kprintf("Error: Cannot read file '%s'\n", path);
        return 1;
    }

    buffer[bytes_read] = '\0';
    kprintf("%s", buffer);

    if (bytes_read > 0 && buffer[bytes_read - 1] != '\n') {
        kprintf("\n");
    }

    return 0;
}

/**
 * Built-in command: create
 */
static int cmd_create(int argc, char **argv) {
    if (argc < 2) {
        kprintf("Usage: create <file>\n");
        return 1;
    }

    const char *filename = argv[1];
    char path[64];

    // Prepend "/" if not present
    if (filename[0] != '/') {
        path[0] = '/';
        size_t i;
        for (i = 0; i < 62 && filename[i] != '\0'; i++) {
            path[i + 1] = filename[i];
        }
        path[i + 1] = '\0';
    } else {
        size_t i;
        for (i = 0; i < 63 && filename[i] != '\0'; i++) {
            path[i] = filename[i];
        }
        path[i] = '\0';
    }

    extern int sfs_create_file(const char *path, uint32_t type);
    #define SFS_TYPE_FILE 1

    int result = sfs_create_file(path, SFS_TYPE_FILE);

    if (result < 0) {
        kprintf("Error: Cannot create file '%s'\n", path);
        return 1;
    }

    kprintf("Created file: %s\n", path);

    return 0;
}

/**
 * Built-in command: write
 */
static int cmd_write(int argc, char **argv) {
    if (argc < 3) {
        kprintf("Usage: write <file> <data>\n");
        return 1;
    }

    const char *filename = argv[1];
    char path[64];

    // Prepend "/" if not present
    if (filename[0] != '/') {
        path[0] = '/';
        size_t i;
        for (i = 0; i < 62 && filename[i] != '\0'; i++) {
            path[i + 1] = filename[i];
        }
        path[i + 1] = '\0';
    } else {
        size_t i;
        for (i = 0; i < 63 && filename[i] != '\0'; i++) {
            path[i] = filename[i];
        }
        path[i] = '\0';
    }

    // Concatenate all remaining arguments as data
    char data[256];
    size_t offset = 0;

    for (int i = 2; i < argc && offset < sizeof(data) - 1; i++) {
        if (i > 2) {
            data[offset++] = ' ';
        }

        const char *arg = argv[i];
        for (size_t j = 0; arg[j] != '\0' && offset < sizeof(data) - 1; j++) {
            data[offset++] = arg[j];
        }
    }
    data[offset] = '\0';

    int bytes_written = sfs_write_file(path, 0, offset, data);

    if (bytes_written < 0) {
        kprintf("Error: Cannot write to file '%s'\n", path);
        return 1;
    }

    kprintf("Wrote %d bytes to %s\n", bytes_written, path);

    return 0;
}

/**
 * Built-in command: mount
 */
static int cmd_mount(int argc, char **argv) {
    (void)argc;
    (void)argv;

    extern int sfs_mount(uint8_t drive, const char *mount_point);

    int result = sfs_mount(0, "/disk");

    if (result < 0) {
        kprintf("Error: Cannot mount filesystem\n");
        kprintf("Tip: Use 'format' to create a filesystem first\n");
        return 1;
    }

    kprintf("Filesystem mounted successfully\n");

    return 0;
}

/**
 * Built-in command: unmount
 */
static int cmd_unmount(int argc, char **argv) {
    (void)argc;
    (void)argv;

    extern void sfs_unmount(void);

    sfs_unmount();
    kprintf("Filesystem unmounted\n");

    return 0;
}

/**
 * Built-in command: format
 */
static int cmd_format(int argc, char **argv) {
    (void)argc;
    (void)argv;

    kprintf("WARNING: This will erase all data on drive 0!\n");
    kprintf("Formatting drive 0 with SimpleFS...\n");

    extern int sfs_format(uint8_t drive, uint32_t total_blocks);

    int result = sfs_format(0, 16384);  // 64 MB filesystem

    if (result < 0) {
        kprintf("Error: Format failed\n");
        return 1;
    }

    kprintf("Format complete!\n");
    kprintf("Use 'mount' to mount the filesystem\n");

    return 0;
}

/**
 * Built-in command: shutdown
 */
static int cmd_shutdown(int argc, char **argv) {
    (void)argc;
    (void)argv;

    kprintf("Shutting down miniOS...\n");
    kprintf("Goodbye!\n");

    // Halt the CPU
    for (;;) {
        __asm__ volatile("cli; hlt");
    }

    return 0;
}

/**
 * Command table
 */
static shell_command_t commands[] = {
    {"help",     "Display help message",              cmd_help},
    {"clear",    "Clear the screen",                  cmd_clear},
    {"echo",     "Echo text to console",              cmd_echo},
    {"uname",    "Display system information",        cmd_uname},
    {"uptime",   "Show system uptime",                cmd_uptime},
    {"free",     "Display memory information",        cmd_free},
    {"ls",       "List files",                        cmd_ls},
    {"cat",      "Display file contents",             cmd_cat},
    {"create",   "Create a new file",                 cmd_create},
    {"write",    "Write data to a file",              cmd_write},
    {"mount",    "Mount filesystem",                  cmd_mount},
    {"unmount",  "Unmount filesystem",                cmd_unmount},
    {"format",   "Format disk with SimpleFS",         cmd_format},
    {"shutdown", "Halt the system",                   cmd_shutdown},
    {NULL,       NULL,                                NULL}
};

/**
 * Parse command line into arguments
 */
static int parse_command(const char *line, char **argv, int max_args) {
    int argc = 0;
    bool in_word = false;
    static char parse_buffer[SHELL_MAX_INPUT];

    // Copy to parse buffer
    size_t i;
    for (i = 0; i < SHELL_MAX_INPUT - 1 && line[i] != '\0'; i++) {
        parse_buffer[i] = line[i];
    }
    parse_buffer[i] = '\0';

    // Parse arguments
    for (i = 0; parse_buffer[i] != '\0' && argc < max_args; i++) {
        if (parse_buffer[i] == ' ' || parse_buffer[i] == '\t') {
            if (in_word) {
                parse_buffer[i] = '\0';
                in_word = false;
            }
        } else {
            if (!in_word) {
                argv[argc++] = &parse_buffer[i];
                in_word = true;
            }
        }
    }

    return argc;
}

/**
 * Execute a single command line
 */
int shell_execute(const char *line) {
    char *argv[SHELL_MAX_ARGS];
    int argc = parse_command(line, argv, SHELL_MAX_ARGS);

    if (argc == 0) {
        return 0;  // Empty line
    }

    // Find and execute command
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            return commands[i].handler(argc, argv);
        }
    }

    kprintf("Unknown command: %s\n", argv[0]);
    kprintf("Type 'help' for a list of commands\n");

    return 1;
}

/**
 * Add command to history
 */
static void add_to_history(const char *line) {
    if (strlen(line) == 0) {
        return;
    }

    // Copy to history
    size_t i;
    for (i = 0; i < SHELL_MAX_INPUT - 1 && line[i] != '\0'; i++) {
        history[history_index][i] = line[i];
    }
    history[history_index][i] = '\0';

    history_index = (history_index + 1) % SHELL_HISTORY_SIZE;
    if (history_count < SHELL_HISTORY_SIZE) {
        history_count++;
    }
}

/**
 * Read a line from keyboard
 */
static void read_line(char *buffer, size_t max_len) {
    size_t pos = 0;

    while (true) {
        char c = keyboard_getchar_blocking();

        if (c == '\n' || c == '\r') {
            buffer[pos] = '\0';
            kprintf("\n");
            return;
        } else if (c == '\b' || c == 127) {  // Backspace
            if (pos > 0) {
                pos--;
                kprintf("\b \b");  // Erase character on screen
            }
        } else if (c >= 32 && c < 127 && pos < max_len - 1) {
            buffer[pos++] = c;
            kprintf("%c", c);
        }
        // Ignore other characters (arrows, function keys, etc.)
    }
}

/**
 * Initialize the shell subsystem
 */
void shell_init(void) {
    kprintf("[SHELL] Initializing shell subsystem\n");

    memset(input_buffer, 0, sizeof(input_buffer));
    memset(history, 0, sizeof(history));
    history_index = 0;
    history_count = 0;

    kprintf("[SHELL] Shell initialized\n");
}

/**
 * Run the interactive shell
 */
void shell_run(void) {
    kprintf("\n");
    kprintf("========================================\n");
    kprintf("       Welcome to miniOS Shell!        \n");
    kprintf("========================================\n");
    kprintf("\n");
    kprintf("Type 'help' for a list of commands\n");
    kprintf("\n");

    while (true) {
        kprintf(SHELL_PROMPT);

        read_line(input_buffer, SHELL_MAX_INPUT);

        if (strlen(input_buffer) > 0) {
            add_to_history(input_buffer);
            shell_execute(input_buffer);
        }
    }
}
