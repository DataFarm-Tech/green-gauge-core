#include "NPK.hpp"
#include <stdio.h>
#include "Logger.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "EEPROMConfig.hpp"
#include "Types.hpp"
#include "UARTDriver.hpp"


NPK::NPK() 
{
    // Constructor can initialize calibration, if needed
}

/**
 * @brief Collect sensor readings for all measurement types.
 * Iterates over the mapping table and sends readings.
 */
void NPK::npk_collect()
{   
    g_logger.info("Collecting sensor readings...");
    uint8_t * npk_pkt;

    for (const auto& entry : MEASUREMENT_TABLE)
    {
        g_logger.info("Reading measurement type: %s", entry.name);

        for (size_t i = 0; i < READING_SIZE; i++)
        {
            float val;

            rs485_uart.write((const char *)entry.packet);
            
            val = 33;
            readingList.at(i) = val;

            
            //read bytes incoming after the write
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    g_logger.info("All readings sent successfully");
}

/**
 * @brief Placeholder for NPK calibration routine.
 * You can implement EEPROM load/store and gain/offset calculation here.
 */
void NPK::npk_calib() 
{
    g_logger.info("Calibration routine not implemented yet");
}
