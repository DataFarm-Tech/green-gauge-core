#include "ReadingPacket.hpp"
#include "NPK.hpp"
#include <stdio.h>
#include "Logger.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "EEPROMConfig.hpp"
#include "Types.hpp"


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

    this->rs485_uart.init(
        BAUD_4800,           // Baud rate (adjust based on your RS485 device requirements)
        GPIO_NUM_37, // TXD0 (IO37) - connects to UART2_TXD
        GPIO_NUM_36, // RXD0 (IO36) - connects to UART2_RXD
        UART_PIN_NO_CHANGE, /* rts_pin */ 
        UART_PIN_NO_CHANGE, /* cts_pin */
        GEN_BUFFER_SIZE, /* rx_buffer_size */ 
        GEN_BUFFER_SIZE  // /* tx_buffer_size */ RS485 benefits from TX buffer
    );

    for (const auto& entry : MEASUREMENT_TABLE)
    {
        g_logger.info("Reading measurement type: %s", entry.name);

        readings.setMeasurementType(entry.name);
        readings.readSensor(this->rs485_uart, entry.packet, entry.packet_size);
        // readings.applyCalibration(g_device_config.calib, entry.type);
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
