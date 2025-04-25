#include <Arduino.h>

#include "cli_th.h"
#include "config.h"
#include "th_handler.h"
#include "utils.h"

#define CLI_BUFFER_SIZE 128

char cli_buffer[CLI_BUFFER_SIZE];
uint8_t cli_pos = 0;

/**
 * @brief Trim the newline characters from the end of a string.
 * @param str The string to trim.
 */
void trim_newline(char* str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[--len] = '\0';
    }
}

/**
 * @brief Handle the command input from the user.
 * @param cmd The command string to handle.
 */
void handle_cmd(const char* cmd) 
{
    cli_printf("\n");

    char cmd_copy[CLI_BUFFER_SIZE];
    strncpy(cmd_copy, cmd, CLI_BUFFER_SIZE);
    cmd_copy[CLI_BUFFER_SIZE - 1] = '\0';

    char* token = strtok(cmd_copy, " ");
    if (token == NULL) return;

    trim_newline(token);

    if (strncmp(token, "help", sizeof(token)) == 0) 
    {
        print_help();
        return;
    }

    if (strncmp(token, "exit", sizeof(token)) == 0)
    {
        cli_print("Exiting CLI...\n");
        delay(1000);
        delete_th(read_serial_cli_th); // Delete the CLI thread
        // ESP.restart();
        return;
    }

    /* Will have to move this reboot portion into a function.
    Since reboot funtionality may be used in other areas.*/
    if (strncmp(token, "reboot", sizeof(token)) == 0)
    {
        cli_print("CLI Triggered Reboot\n");
        delay(1000);
        ESP.restart();
        return;
    }

    //Ping functionality (check if google.com)
    //clear
    //view queue & size of queue
    //what threads are running?
    //view node-list
    //

    if (strncmp(token, "ping", sizeof(token)) == 0)
    {
        /* code */
    }
    
}

/**
 * @brief Read from the serial port and handle commands.
 * This function runs in a separate task and continuously reads from the serial port.
 * It processes user input, handles backspace, and executes commands.
 * @param param Unused parameter.
 */
void read_serial_cli(void *param) 
{
    cli_printf("%s> ", ID);  // Print initial prompt
    
    while (1) 
    {
        bool processedCommand = false; // Track if a command was processed

        while (Serial.available()) 
        {
            char c = Serial.read();
            
            if (c == '\b' || c == 127) 
            {
                if (cli_pos > 0) 
                {
                    cli_pos--;
                    cli_buffer[cli_pos] = '\0';
                    cli_print("\b \b");
                }
                continue;
            }

            if (c == '\r' || c == '\n') 
            {
                cli_buffer[cli_pos] = '\0';

                if (cli_pos > 0) 
                {
                    handle_cmd(cli_buffer);
                    cli_pos = 0;
                    processedCommand = true; // Mark that a command was processed
                }
                continue;
            }

            if (cli_pos < CLI_BUFFER_SIZE - 1 && isprint(c)) 
            {
                cli_buffer[cli_pos++] = c;
                cli_print(c); // Echo
            }
        }

        // If command was processed, print the prompt again
        if (processedCommand) 
        {
            cli_printf("%s> ", ID);
            processedCommand = false; // Reset the flag
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Print welcome message.
 */
void print_motd()
{
    cli_printf("Welcome to the DataFarm CLI!\n");
    cli_printf("\n");
}

/**
 * @brief A function to print the help text
 */
void print_help()
{
    cli_printf("Available commands:\n");
    cli_printf("  help - Show this help message\n");
    cli_printf("  exit - Exit the CLI\n");
    cli_printf("  reboot - Reboot This Device\n");
}