#include <Arduino.h>
#include "cli.h"
#include "config.h"

#define CLI_BUFFER_SIZE 128

char cli_buffer[CLI_BUFFER_SIZE];
uint8_t cli_pos = 0;

/**
 * @brief This function handles the command line interface (CLI) input.
 * It reads characters from the serial input and processes them.
 * If the input is a backspace, it removes the last character.
 * If the input is a newline or carriage return, it processes the command.
 * Otherwise, it adds the character to the buffer.
 * @param None
 * @return None
 */
void read_serial_cli(void *param) 
{
    printf("%s> ", ID); // Initial prompt

    while (1)
    {
        while (Serial.available()) 
        {
            char c = Serial.read();

            if (c == '\b' && cli_pos > 0) 
            {
                cli_pos--;
                continue;
            }

            if (c == '\n' || c == '\r') 
            {
                cli_buffer[cli_pos] = '\0';  // Null-terminate
                handleCommand(cli_buffer); 
                cli_pos = 0;

                // Reprint the prompt
                printf("%s> ", ID);
            } 
            else if (cli_pos < CLI_BUFFER_SIZE - 1) 
            {
                cli_buffer[cli_pos++] = c;
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief This function handles the command input from the CLI.
 * It processes the command and calls the appropriate function.
 * @param cmd - The command string to be processed
 * @return None
 */
void handle_cmd(const char* cmd) 
{
    Serial.println(cmd);
    return;
}
  