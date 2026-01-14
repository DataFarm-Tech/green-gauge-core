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

const uint8_t * ReadingPacket::toBuffer()
{
    CborEncoder encoder, mapEncoder, arrayEncoder, readingEncoder;
    cbor_encoder_init(&encoder, buffer, GEN_BUFFER_SIZE, 0);  // use member buffer

    if (cbor_encoder_create_map(&encoder, &mapEncoder, 2) != CborNoError) 
    {
        ESP_LOGE(TAG.c_str(), "Failed to create root map");
        return nullptr;
    }

    cbor_encode_text_stringz(&mapEncoder, "node_id");
    cbor_encode_text_stringz(&mapEncoder, nodeId.c_str());

    cbor_encode_text_stringz(&mapEncoder, "readings");
    cbor_encoder_create_array(&mapEncoder, &arrayEncoder, readingList.size());

    for (size_t i = 0; i < READING_SIZE; i++)
    {
        cbor_encoder_create_map(&arrayEncoder, &readingEncoder, 2);

        cbor_encode_text_stringz(&readingEncoder, "temperature");
        cbor_encode_float(&readingEncoder, readingList[i].temp);

        cbor_encode_text_stringz(&readingEncoder, "ph");
        cbor_encode_float(&readingEncoder, readingList[i].ph);

        cbor_encoder_close_container(&arrayEncoder, &readingEncoder);
    }

    cbor_encoder_close_container(&mapEncoder, &arrayEncoder);
    cbor_encoder_close_container(&encoder, &mapEncoder);

    bufferLength = cbor_encoder_get_buffer_size(&encoder, buffer);
    if (bufferLength > GEN_BUFFER_SIZE) {
        ESP_LOGE(TAG.c_str(), "CBOR buffer overflow: %d bytes (max %d)", (int)bufferLength, GEN_BUFFER_SIZE);
        return nullptr;
    }
    
    return buffer;
}


void ReadingPacket::readSensor() 
{
    sensorReading reading;
    
    ESP_LOGI(TAG.c_str(), "Starting sensor reading sequence...");

    // TODO: remove this for production
    srand(static_cast<unsigned>(time(nullptr)));

    for (size_t i = 0; i < READING_SIZE; i++)
    {
        // Generate temperature [20.0, 30.0] and pH [5.0, 8.0]
        reading.temp = 20.0f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 10.0f; // [20–30]
        reading.ph   = 5.0f  + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 3.0f;  // [5–8]

        readingList.at(i) = reading;

        ESP_LOGI(TAG.c_str(),
         "Reading %02d -> Temp: %.2f °C, pH: %.2f",
         (int)i,
         static_cast<double>(reading.temp),
         static_cast<double>(reading.ph));


        vTaskDelay(pdMS_TO_TICKS(10)); // 10 ms delay between readings
    }

    ESP_LOGI(TAG.c_str(), "Finished collecting %d readings", (int)READING_SIZE);
}
