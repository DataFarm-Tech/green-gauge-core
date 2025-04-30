#include <EEPROM.h>
#include "eeprom.h"

#define EEPROM_SIZE sizeof(device_config)
#define EEPROM_ADDR 0 //start writing eeprom from the start of mem

device_config config;

/**
 * @brief The following function initialises the eeprom interface.
 * EEPROM.begin() is provided with the size of the device_config object
 */
void init_eeprom_int()
{
    EEPROM.begin(EEPROM_SIZE);
    return;
}

/**
 * @brief The following function saves the current config variable
 * into EEPROM
 */
void save_config()
{
    EEPROM.put(EEPROM_ADDR, config);
    EEPROM.commit();
} //after an apply in CLI, or when something needs to be saved to eeprom

/**
 * @brief The following function reads all the data from EEPROM and saves 
 * it into the config variable.
 */
void read_config() 
{
    EEPROM.get(EEPROM_ADDR, config);  // Load the configuration from EEPROM
}
