#include "utils.h"
#include "config.h"

/**
 * @brief The following function takes a string,
 * and prints it to the serial console.
 */
void print_prompt() { cli_printf("%s> ", ID); }

/**
 * @brief The following function prints the MOTD.
 */
void print_motd() {cli_printf("Welcome to the DataFarm CLI!\n\n");}

/**
 * @brief The following function takes a string.
 * And trims the '\n' from the end of the string.
 * @param str - The string to trim
 * @return None
 */
void trim_newline(char * str) 
{
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n')) 
    {
        str[--len] = '\0';
    }
}


