#pragma once
#include <string.h>
#include "UARTDriver.hpp"
#include "driver/gpio.h"

 /**
  * @brief Calibration structure for NPK sensor
  */
enum class MeasurementType {
    Nitrogen,
    Phosphorus,
    Potassium,
    Moisture,
    PH,
    Temperature
};

/**
 * @brief Table linking MeasurementType enums to string names.
 * Acts like a 2D array: each row contains the enum and its string.
 */
struct MeasurementEntry {
    MeasurementType type;
    const char* name;
};



constexpr const char* NITROGEN = "nitrogen";
constexpr const char* PHOSPHORUS = "phosphorus";
constexpr const char* POTASSIUM = "potassium";
constexpr const char* MOISTURE = "moisture";
constexpr const char* PH = "ph";
constexpr const char* TEMPERATURE = "temperature";

// Static constexpr table for compile-time mapping
static constexpr MeasurementEntry MEASUREMENT_TABLE[] = {
    { MeasurementType::Nitrogen,    NITROGEN },
    { MeasurementType::Moisture,    MOISTURE},
    { MeasurementType::PH,           PH},
    { MeasurementType::Phosphorus,  PHOSPHORUS},
    { MeasurementType::Potassium,  POTASSIUM},
    { MeasurementType::Temperature, TEMPERATURE}
};


class ReadingPacket;

/**
 * @brief Class for handling NPK sensor operations
 */
class NPK {
public:
    /**
     * @brief Constructor for NPK class
     */
    NPK();
    /**
     * @brief Collect NPK readings and store them in the provided ReadingPacket
     * @param readings Reference to ReadingPacket to store the readings
     */
    void npk_collect(ReadingPacket& readings);

    /**
     * @brief Calibrate the NPK sensor
     */
    void npk_calib();
private:
    /**
     * @brief 
     */
    UARTDriver rs485_uart { UART_NUM_2 };   // UART connected to Quectel
};
