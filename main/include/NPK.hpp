#pragma once
#include <cstdint>
#include <cstring>
#include "UARTDriver.hpp"
#include "driver/gpio.h"
#include <vector>
#include <array>

#include "Config.hpp"
/**
 * @brief Measurement types supported by the NPK sensor
 */
enum class MeasurementType
{
    Nitrogen,
    Phosphorus,
    Potassium,
    Moisture,
    PH,
    Temperature
};

// Measurement type name constants
constexpr const char *NITROGEN = "nitrogen";
constexpr const char *PHOSPHORUS = "phosphorus";
constexpr const char *POTASSIUM = "potassium";
constexpr const char *MOISTURE = "moisture";
constexpr const char *PH = "ph";
constexpr const char *TEMPERATURE = "temperature";

// Forward declaration
class ReadingPacket;

/**
 * @brief Class for handling NPK soil sensor operations via RS485/Modbus RTU
 *
 * This class handles communication with the CWT NPK soil sensor which measures:
 * - Nitrogen (N), Phosphorus (P), Potassium (K)
 * - Humidity/Moisture
 * - Temperature
 * - pH
 *
 * Default sensor settings:
 * - Device Address: 0x01
 * - Baud Rate: 4800
 * - Format: 8N1
 */
class NPK
{
public:
    // Packet size constants (must be defined before MEASUREMENT_TABLE)
    static constexpr size_t PACKET_SIZE = 8;
    static constexpr size_t SINGLE_RESPONSE_SIZE = 7;
    static constexpr size_t ALL_RESPONSE_SIZE = 19;
    static constexpr size_t READING_SIZE = 30;
    std::array<float, READING_SIZE> readingList;

    /**
     * @brief Table entry linking measurement type, name, and Modbus packet
     */
    struct MeasurementEntry
    {
        MeasurementType type;
        const uint8_t *packet;
        size_t packet_size;
    };

    /**
     * @brief Constructor for NPK class
     */
    NPK();

    /**
     * @brief Collect NPK readings and store them in the provided ReadingPacket
     * @param readings Reference to ReadingPacket to store the readings
     */
    static void npk_collect(const MeasurementEntry m_entry, uint8_t reading[NPK_COLLECT_SIZE]);

    /**
     * @brief Calibrate the NPK sensor
     */
    void npk_calib();

private:
    // Modbus RTU packet definitions (must be defined before MEASUREMENT_TABLE)

    // Read single humidity value (Register 0x0000)
    static constexpr uint8_t READ_HUMIDITY[PACKET_SIZE] = {
        0x01,       // Device Address
        0x03,       // Function Code (Read Holding Registers)
        0x00, 0x00, // Start Address (Humidity)
        0x00, 0x01, // Number of registers to read (1)
        0x84, 0x0A  // CRC16 (Low, High)
    };

    // Read single temperature value (Register 0x0001)
    static constexpr uint8_t READ_TEMPERATURE[PACKET_SIZE] = {
        0x01,       // Device Address
        0x03,       // Function Code
        0x00, 0x01, // Start Address (Temperature)
        0x00, 0x01, // Number of registers
        0xD5, 0xCA  // CRC16
    };

    // Read single PH value (Register 0x0003)
    static constexpr uint8_t READ_PH[PACKET_SIZE] = {
        0x01,       // Device Address
        0x03,       // Function Code
        0x00, 0x03, // Start Address (PH)
        0x00, 0x01, // Number of registers
        0x74, 0x0A  // CRC16
    };

    // Read single Nitrogen value (Register 0x0004)
    static constexpr uint8_t READ_NITROGEN[PACKET_SIZE] = {
        0x01,       // Device Address
        0x03,       // Function Code
        0x00, 0x04, // Start Address (Nitrogen)
        0x00, 0x01, // Number of registers
        0xC5, 0xCB  // CRC16
    };

    // Read single Phosphorus value (Register 0x0005)
    static constexpr uint8_t READ_PHOSPHORUS[PACKET_SIZE] = {
        0x01,       // Device Address
        0x03,       // Function Code
        0x00, 0x05, // Start Address (Phosphorus)
        0x00, 0x01, // Number of registers
        0x94, 0x0B  // CRC16
    };

    // Read single Potassium value (Register 0x0006)
    static constexpr uint8_t READ_POTASSIUM[PACKET_SIZE] = {
        0x01,       // Device Address
        0x03,       // Function Code
        0x00, 0x06, // Start Address (Potassium)
        0x00, 0x01, // Number of registers
        0x64, 0x0B  // CRC16
    };

    // Read ALL sensors at once (7 registers: Humidity through Potassium)
    static constexpr uint8_t READ_ALL_SENSORS[PACKET_SIZE] = {
        0x01,       // Device Address
        0x03,       // Function Code
        0x00, 0x00, // Start Address (Humidity)
        0x00, 0x07, // Number of registers (7)
        0x04, 0x08  // CRC16
    };

public:
    // Measurement table - must be defined AFTER the packet arrays
    static constexpr MeasurementEntry MEASUREMENT_TABLE[] = {
        {MeasurementType::Nitrogen, READ_NITROGEN, PACKET_SIZE},
        {MeasurementType::Moisture, READ_HUMIDITY, PACKET_SIZE},
        {MeasurementType::PH, READ_PH, PACKET_SIZE},
        {MeasurementType::Phosphorus, READ_PHOSPHORUS, PACKET_SIZE},
        {MeasurementType::Potassium, READ_POTASSIUM, PACKET_SIZE},
        {MeasurementType::Temperature, READ_TEMPERATURE, PACKET_SIZE}};

    static constexpr size_t MEASUREMENT_TABLE_SIZE = sizeof(MEASUREMENT_TABLE) / sizeof(MeasurementEntry);
};