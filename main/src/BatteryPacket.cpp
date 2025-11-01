#include <cbor.h>
#include <cstring>
#include <esp_log.h>
#include <cbor.h>
#include <sys/socket.h>
#include "esp_log.h"
#include <cstdint>
#include <cstddef>
#include "BatteryPacket.hpp"
#include "Config.hpp"

const uint8_t * BatteryPacket::toBuffer()
{
    CborEncoder encoder, mapEncoder, arrayEncoder, batteryEncoder;
    cbor_encoder_init(&encoder, buffer, BUFFER_SIZE, 0);  // use member buffer

    if (cbor_encoder_create_map(&encoder, &mapEncoder, 2) != CborNoError) 
    {
        ESP_LOGE(TAG.c_str(), "Failed to create root map");
        return nullptr;
    }

    cbor_encode_text_stringz(&mapEncoder, "node_id");
    cbor_encode_text_stringz(&mapEncoder, nodeId.c_str());

    cbor_encode_text_stringz(&mapEncoder, "batteries");
    cbor_encoder_create_array(&mapEncoder, &arrayEncoder, 1);
    cbor_encoder_create_map(&arrayEncoder, &batteryEncoder, 2);

    cbor_encode_text_stringz(&batteryEncoder, "bat_lvl");
    cbor_encode_int(&batteryEncoder, level);

    cbor_encode_text_stringz(&batteryEncoder, "bat_hlth");
    cbor_encode_int(&batteryEncoder, health);

    cbor_encoder_close_container(&arrayEncoder, &batteryEncoder);
    cbor_encoder_close_container(&mapEncoder, &arrayEncoder);
    cbor_encoder_close_container(&encoder, &mapEncoder);

    bufferLength = cbor_encoder_get_buffer_size(&encoder, buffer);
    ESP_LOGI(TAG.c_str(), "CBOR payload length: %d", (int)bufferLength);

    return buffer;
}


bool BatteryPacket::readFromBMS() 
{
    // TODO: implement real BMS reading (UART/I2C/CAN)
    // For now, simulate data:
    level = 90;
    health = 100;
    return true;
}
