#pragma once

#include "IPacket.hpp"
#include <vector>
#include <array>
#include "EEPROMConfig.hpp"
#include <string>
#include "NPK.hpp"

class ReadingPacket : public IPacket {
    private:
        static constexpr size_t READING_SIZE = 35;
        std::array<float, READING_SIZE> readingList;
        std::string TAG;
        std::string measurementType;  // Added missing member

    public:
        ReadingPacket(const std::string& id, const std::string& uri_endpoint,
                    const std::string& measurement_type = "temperature",
                    const std::string& tag = "ReadingPacket")
            : TAG(tag), measurementType(measurement_type)
        {
            nodeId = id;
            uri = uri_endpoint;
        }

        const uint8_t * toBuffer() override;
        
        /**
         * Will read from sensor and store values in readingList.
         * Delay for 10ms between each reading.
         * Collects 35 readings total.
         */
        void readSensor(UARTDriver& rs485_uart, const uint8_t * msg, const size_t msg_len);

        /**
         * Apply calibration to the readings.
         * @param gain Gain factor
         * @param offset Offset value
         */
        void applyCalibration(NPK_Calib_t calib, MeasurementType m_type);

        /**
         * @brief Set the measurement type to encode
         * @param type Either "temperature" or "ph"
         */
        void setMeasurementType(const std::string& type) {
            measurementType = type;
        }

        /**
         * @brief Get the current measurement type
         * @return The measurement type string
         */
        std::string getMeasurementType() const {
            return measurementType;
        }
};