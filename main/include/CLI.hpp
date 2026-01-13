#pragma once

// Forward declaration
class UARTConsole;

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
 *   UARTConsole console(UART_NUM_0);
 *   console.init(115200);
 *   CLI::start(console);
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
     *
     * @param console Reference to the UARTConsole instance to use for I/O.
     */
    static void start(UARTConsole& console);

    /**
     * @brief Get the current console instance.
     *
     * Returns a pointer to the UARTConsole instance being used by the CLI.
     * This allows command handlers to access the console for output.
     *
     * @return Pointer to the active UARTConsole instance, or nullptr if not initialized.
     */
    static UARTConsole* getConsole();

private:
    static UARTConsole* s_console;  ///< Console instance used by CLI
};

/**
 * @brief Global command table.
 */
extern const Command commands[];

/**
 * @brief Dispatch a subcommand from a command table.
 *
 * Helper function for commands that have subcommands. Searches the provided
 * command table for a matching subcommand and executes it.
 *
 * @param table   Command table to search.
 * @param argc    Argument count.
 * @param argv    Argument vector.
 * @param usage   Usage string to display if subcommand is missing.
 */
void dispatch_subcommand(
    const Command* table,
    int argc,
    char** argv,
    const char* usage
);