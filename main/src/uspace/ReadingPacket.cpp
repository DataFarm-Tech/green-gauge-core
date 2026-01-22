#include <cbor.h>
#include <cstring>
#include <esp_log.h>
#include <cbor.h>
#include <sys/socket.h>
#include <cstdint>
#include <cstddef>
#include "ReadingPacket.hpp"
#include "Config.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstdlib>
#include <ctime>
#include "NPK.hpp"
#include "Logger.hpp"
#include "UARTDriver.hpp"
#include "driver/gpio.h"

const uint8_t * ReadingPacket::toBuffer()
{
    CborEncoder encoder, mapEncoder, arrayEncoder;
    cbor_encoder_init(&encoder, buffer, GEN_BUFFER_SIZE, 0);

    // Create root map with 3 entries: node_id, measurement_type, and readings
    if (cbor_encoder_create_map(&encoder, &mapEncoder, 3) != CborNoError) 
    {
        ESP_LOGE(TAG.c_str(), "Failed to create root map");
        return nullptr;
    }

    // Encode node_id
    if (cbor_encode_text_stringz(&mapEncoder, "node_id") != CborNoError ||
        cbor_encode_text_stringz(&mapEncoder, nodeId.c_str()) != CborNoError)
    {
        ESP_LOGE(TAG.c_str(), "Failed to encode node_id");
        return nullptr;
    }

    // Encode measurement_type
    if (cbor_encode_text_stringz(&mapEncoder, "m_type") != CborNoError ||
        cbor_encode_text_stringz(&mapEncoder, measurementType.c_str()) != CborNoError)
    {
        ESP_LOGE(TAG.c_str(), "Failed to encode measurement_type");
        return nullptr;
    }

    // Encode readings array (just the float values)
    if (cbor_encode_text_stringz(&mapEncoder, "readings") != CborNoError)
    {
        ESP_LOGE(TAG.c_str(), "Failed to encode 'readings' key");
        return nullptr;
    }

    if (cbor_encoder_create_array(&mapEncoder, &arrayEncoder, READING_SIZE) != CborNoError)
    {
        ESP_LOGE(TAG.c_str(), "Failed to create readings array");
        return nullptr;
    }

    // Encode each reading value
    for (size_t i = 0; i < READING_SIZE; i++)
    {
        if (cbor_encode_float(&arrayEncoder, readingList[i]) != CborNoError)
        {
            ESP_LOGE(TAG.c_str(), "Failed to encode reading %d", (int)i);
            return nullptr;
        }
    }

    // Close readings array
    if (cbor_encoder_close_container(&mapEncoder, &arrayEncoder) != CborNoError)
    {
        ESP_LOGE(TAG.c_str(), "Failed to close readings array");
        return nullptr;
    }

    // Close root map
    if (cbor_encoder_close_container(&encoder, &mapEncoder) != CborNoError)
    {
        ESP_LOGE(TAG.c_str(), "Failed to close root map");
        return nullptr;
    }

    bufferLength = cbor_encoder_get_buffer_size(&encoder, buffer);
    if (bufferLength > GEN_BUFFER_SIZE) {
        ESP_LOGE(TAG.c_str(), "CBOR buffer overflow: %d bytes (max %d)", (int)bufferLength, GEN_BUFFER_SIZE);
        return nullptr;
    }
    
    ESP_LOGI(TAG.c_str(), "CBOR encoding complete: %d bytes for %s", (int)bufferLength, measurementType.c_str());
    return buffer;
}

void ReadingPacket::applyCalibration(NPK_Calib_t calib, MeasurementType m_type) {
    float gain = 1.0f;
    float offset = 0.0f;
    bool found = false;

    // Search through the calibration list for matching measurement type
    for (size_t i = 0; i < 6; i++) {
        if (calib.calib_list[i].m_type == m_type) {
            gain = calib.calib_list[i].gain;
            offset = calib.calib_list[i].offset;
            found = true;
            
            // Find the name for logging
            const char* type_name = "UNKNOWN";
            for (const auto& entry : MEASUREMENT_TABLE) {
                if (entry.type == m_type) {
                    type_name = entry.name;
                    break;
                }
            }
            
            ESP_LOGI(TAG.c_str(), "Applying calibration for %s: gain=%.4f, offset=%.4f", 
                     type_name, static_cast<double>(gain), static_cast<double>(offset));
            break;
        }
    }

    if (!found) {
        ESP_LOGW(TAG.c_str(), "No calibration found for measurement type, using defaults");
    }

    // Apply calibration to all readings
    for (size_t i = 0; i < READING_SIZE; i++) {
        readingList[i] = readingList[i] * gain + offset;
    }
}

void ReadingPacket::readSensor(UARTDriver& rs485_uart)
{
    g_logger.info("Starting sensor reading sequence for %s...", measurementType.c_str());

    //Create UARTDriver instance and initialize it
    //Then write bytes to UARTDriver, Then do a read

    // TODO: remove this for production
    srand(static_cast<unsigned>(time(nullptr)));

    for (size_t i = 0; i < READING_SIZE; i++)
    {
        float value;
        
        value = 5.0f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 75.0f;
        
        readingList.at(i) = value;

        ESP_LOGI(TAG.c_str(), "Reading %02d -> %s: %.2f", (int)i, measurementType.c_str(), static_cast<double>(value));

        vTaskDelay(pdMS_TO_TICKS(10)); // 10 ms delay between readings
    }

    ESP_LOGI(TAG.c_str(), "Finished collecting %d %s readings", (int)READING_SIZE, measurementType.c_str());
}