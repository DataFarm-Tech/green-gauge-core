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
void NPK::npk_collect(const MeasurementEntry m_entry, uint8_t reading[NPK_COLLECT_SIZE])
{
    // for (size_t i = 0; i < NPK_COLLECT_SIZE; i++) {
    //     for (size_t j = 0; j < m_entry.packet_size; j++) {
    //         rs485_uart.writeByte(m_entry.packet[j]);
    //     }
        
    // }
    
}

/**
 * @brief Placeholder for NPK calibration routine.
 * You can implement EEPROM load/store and gain/offset calculation here.
 */
void NPK::npk_calib()
{
    g_logger.info("Calibration routine not implemented yet");
}
