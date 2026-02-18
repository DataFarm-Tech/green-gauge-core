#include "GpsUpdatePkt.hpp"
#include "psa/crypto.h"
#include "Key.hpp"
#include "EEPROMConfig.hpp"

const uint8_t * GpsUpdatePkt::toBuffer()
{
    CborEncoder encoder, mapEncoder;
    cbor_encoder_init(&encoder, buffer, GEN_BUFFER_SIZE, 0);
    
    // Root map with 3 elements: node_id, gps, key
    if (cbor_encoder_create_map(&encoder, &mapEncoder, 3) != CborNoError)
    {
        ESP_LOGI("OK", "Failed to create root map");
        return nullptr;
    }

    if (strlen((const char*)g_device_config.secretKey) == 0) {
        ESP_LOGI("OK", "Secret key is empty, cannot encode GpsUpdatePkt");
        return nullptr;
    }

    // node_id
    cbor_encode_text_stringz(&mapEncoder, NODE_ID_KEY);
    cbor_encode_text_stringz(&mapEncoder, node_id.c_str());

    // gps
    cbor_encode_text_stringz(&mapEncoder, GPS_KEY);
    cbor_encode_text_stringz(&mapEncoder, GPSCoord.c_str());

    // key as raw bytes
    cbor_encode_text_stringz(&mapEncoder, KEY_KEY);
    cbor_encode_byte_string(&mapEncoder, g_device_config.secretKey, sizeof(g_device_config.secretKey));

    // Close map
    cbor_encoder_close_container(&encoder, &mapEncoder);

    bufferLength = cbor_encoder_get_buffer_size(&encoder, buffer);
    if (bufferLength > GEN_BUFFER_SIZE)
    {
        ESP_LOGI("OK", "CBOR OVERFLOW Failed to create root map");
        return nullptr;
    }

    return buffer;
}