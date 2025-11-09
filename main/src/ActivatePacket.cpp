#include "packet/ActivatePacket.hpp"
#include <mbedtls/md.h>
#include "Config.hpp"



void ActivatePacket::computeKey(uint8_t * out_hmac) const {
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);

    // Use raw bytes of the secretKey
    mbedtls_md_hmac_starts(&ctx, secretKey, HMAC_KEY_SIZE);

    // No update, finish immediately
    mbedtls_md_hmac_finish(&ctx, out_hmac);
    mbedtls_md_free(&ctx);
}

const uint8_t * ActivatePacket::toBuffer()
{
    CborEncoder encoder, mapEncoder;
    cbor_encoder_init(&encoder, buffer, BUFFER_SIZE, 0);

    // Root map with 3 elements: node_id, gps, key
    if (cbor_encoder_create_map(&encoder, &mapEncoder, 3) != CborNoError) 
    {
        ESP_LOGE(TAG.c_str(), "Failed to create root map");
        return nullptr;
    }

    uint8_t hmac[HMAC_KEY_SIZE];
    computeKey(hmac);

    // TODO: Get GPS Coordinates from GPS MOD
    std::string gps_coor = "00.00.00.00";

    // node_id
    cbor_encode_text_stringz(&mapEncoder, "node_id");
    cbor_encode_text_stringz(&mapEncoder, nodeId.c_str());

    // gps
    cbor_encode_text_stringz(&mapEncoder, "gps");
    cbor_encode_text_stringz(&mapEncoder, gps_coor.c_str());

    // key as raw bytes
    cbor_encode_text_stringz(&mapEncoder, "key");
    cbor_encode_byte_string(&mapEncoder, hmac, sizeof(hmac));

    // Close map
    cbor_encoder_close_container(&encoder, &mapEncoder);

    bufferLength = cbor_encoder_get_buffer_size(&encoder, buffer);
    if (bufferLength > BUFFER_SIZE) {
        ESP_LOGE(TAG.c_str(), "CBOR buffer overflow: %d bytes (max %d)", (int)bufferLength, BUFFER_SIZE);
        return nullptr;
    }

    return buffer;
}
