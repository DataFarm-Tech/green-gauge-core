#include "ReadingPacket.hpp"
#include "NPK.hpp"
#include <stdio.h>
#include "Logger.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "EEPROMConfig.hpp"


NPK::NPK() 
{
    // Constructor can initialize calibration, if needed
}

/**
 * @brief Collect sensor readings for all measurement types.
 * Iterates over the mapping table and sends readings.
 */
void NPK::npk_collect(ReadingPacket& readings)
{   
    g_logger.info("Collecting sensor readings...");

    for (const auto& entry : MEASUREMENT_TABLE)
    {
        g_logger.info("Reading measurement type: %s", entry.name);

        readings.setMeasurementType(entry.name);
        readings.readSensor();
        readings.applyCalibration(g_device_config.calib, entry.type);
        readings.sendPacket();

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
