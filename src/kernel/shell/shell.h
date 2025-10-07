#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include <stdbool.h>

/**
 * miniOS Shell - Simple interactive command interpreter
 *
 * Features:
 * - Built-in commands (help, clear, echo, ls, cat, etc.)
 * - Command history (up/down arrows)
 * - Line editing (backspace)
 * - Tab completion (future)
 */

#define SHELL_MAX_INPUT     256
#define SHELL_MAX_ARGS      16
#define SHELL_HISTORY_SIZE  10
#define SHELL_PROMPT        "minios> "

/**
 * Command handler function type
 *
 * @param argc Argument count
 * @param argv Argument vector (argv[0] is command name)
 * @return 0 on success, non-zero on error
 */
typedef int (*shell_command_handler_t)(int argc, char **argv);

/**
 * Built-in command structure
 */
typedef struct {
    const char *name;
    const char *description;
    shell_command_handler_t handler;
} shell_command_t;

/**
 * Initialize the shell subsystem
 */
void shell_init(void);

/**
 * Run the interactive shell
 * (This function does not return)
 */
void shell_run(void);

/**
 * Execute a single command line
 *
 * @param line Command line to execute
 * @return 0 on success, non-zero on error
 */
int shell_execute(const char *line);

#endif // SHELL_H
