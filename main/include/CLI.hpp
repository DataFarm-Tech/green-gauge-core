#pragma once

/**
 * @brief Command handler function prototype.
 *
 * All CLI commands must conform to this signature.
 *
 * @param argc  Number of arguments (including command name).
 * @param argv  Argument vector.
 */
typedef void (*cmd_handler_t)(int argc, char** argv);

/**
 * @brief Defines a single CLI command entry.
 *
 * Each command consists of:
 *  - name: command string entered by the user
 *  - help: short usage / description string
 *  - handler: function invoked when the command is executed
 *  - min_args: minimum number of arguments required (excluding command name)
 */
struct Command {
    const char* name;          ///< Command name
    const char* help;          ///< Help / usage string
    cmd_handler_t handler;     ///< Command handler function
    int min_args;              ///< Minimum required argument count
};

/**
 * @class CLI
 * @brief UART-based interactive command-line interface.
 *
 * The CLI class provides a simple, lightweight command-line interface
 * over UART. It runs as a FreeRTOS task, supports command parsing,
 * line editing, command history, and dispatches commands defined in
 * the global command table.
 *
 * Typical usage:
 * @code
 *   CLI::start();
 * @endcode
 */
class CLI {
public:
    /**
     * @brief Start the CLI task.
     *
     * Creates and launches the FreeRTOS task responsible for handling
     * UART input/output, command parsing, history navigation, and
     * command execution.
     *
     * This function should be called once during system initialization,
     * after the UART console has been initialized.
     */
    static void start();
};
/**
 * @brief Global command table.
 */
extern const Command commands[];

