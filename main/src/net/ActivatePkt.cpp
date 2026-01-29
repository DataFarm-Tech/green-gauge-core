#include "ActivatePkt.hpp"
#include "psa/crypto.h"
#include "Config.hpp"


const uint8_t * ActivatePkt::toBuffer()
{
    CborEncoder encoder, mapEncoder;
    cbor_encoder_init(&encoder, buffer, GEN_BUFFER_SIZE, 0);

    // Root map with 3 elements: node_id, gps, key
    if (cbor_encoder_create_map(&encoder, &mapEncoder, 2) != CborNoError) 
    {
        ESP_LOGE(TAG.c_str(), "Failed to create root map");
        return nullptr;
    }

    // TODO: Get GPS Coordinates from GPS MOD
    std::string gps_coor = "37.421998, -122.084000";

    // node_id
    cbor_encode_text_stringz(&mapEncoder, "node_id");
    cbor_encode_text_stringz(&mapEncoder, nodeId.c_str());

    // gps
    cbor_encode_text_stringz(&mapEncoder, "gps");
    cbor_encode_text_stringz(&mapEncoder, gps_coor.c_str());

    // Close map
    cbor_encoder_close_container(&encoder, &mapEncoder);

    bufferLength = cbor_encoder_get_buffer_size(&encoder, buffer);
    if (bufferLength > GEN_BUFFER_SIZE) {
        ESP_LOGE(TAG.c_str(), "CBOR buffer overflow: %d bytes (max %d)", (int)bufferLength, GEN_BUFFER_SIZE);
        return nullptr;
    }

    return buffer;
}