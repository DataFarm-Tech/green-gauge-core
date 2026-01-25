#include "ActivatePacket.hpp"
#include "psa/crypto.h"
#include "Config.hpp"
#include "EEPROMConfig.hpp"

void ActivatePacket::computeKey(uint8_t * out_hmac) const {
    psa_status_t status;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key_id = 0;
    size_t mac_length = 0;

    // Initialize PSA Crypto (safe to call multiple times)
    psa_crypto_init();

    // Configure key attributes for HMAC-SHA256
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH);
    psa_set_key_algorithm(&attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));
    psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
    psa_set_key_bits(&attributes, HMAC_KEY_SIZE * 8);

    // Import the secret key
    status = psa_import_key(&attributes, secretKey, HMAC_KEY_SIZE, &key_id);
    if (status != PSA_SUCCESS) {
        ESP_LOGE(TAG.c_str(), "Failed to import HMAC key: %d", status);
        psa_reset_key_attributes(&attributes);
        return;
    }

    // Since you're not hashing any additional data (no update calls),
    // we compute HMAC of an empty message
    status = psa_mac_compute(
        key_id,
        PSA_ALG_HMAC(PSA_ALG_SHA_256),
        NULL,           // No input data (empty message)
        0,              // Zero length
        out_hmac,
        32,             // SHA256 produces 32 bytes
        &mac_length
    );

    if (status != PSA_SUCCESS) {
        ESP_LOGE(TAG.c_str(), "Failed to compute HMAC: %d", status);
    }

    // Cleanup
    psa_destroy_key(key_id);
    psa_reset_key_attributes(&attributes);
}

const uint8_t * ActivatePacket::toBuffer()
{
    CborEncoder encoder, mapEncoder;
    cbor_encoder_init(&encoder, buffer, GEN_BUFFER_SIZE, 0);

    // Root map with 3 elements: node_id, gps, key
    if (cbor_encoder_create_map(&encoder, &mapEncoder, 4) != CborNoError) 
    {
        ESP_LOGE(TAG.c_str(), "Failed to create root map");
        return nullptr;
    }

    uint8_t hmac[HMAC_KEY_SIZE];
    computeKey(hmac);

    // TODO: Get GPS Coordinates from GPS MOD
    std::string gps_coor = "37.421998, -122.084000";

    // node_id
    cbor_encode_text_stringz(&mapEncoder, "node_id");
    printf("nodeid: %s\n", nodeId.c_str());
    cbor_encode_text_stringz(&mapEncoder, nodeId.c_str());

    // gps
    cbor_encode_text_stringz(&mapEncoder, "gps");
    cbor_encode_text_stringz(&mapEncoder, gps_coor.c_str());

    // key as raw bytes
    cbor_encode_text_stringz(&mapEncoder, "key");
    cbor_encode_byte_string(&mapEncoder, hmac, sizeof(hmac));

    cbor_encode_text_stringz(&mapEncoder, "secretkey");
    cbor_encode_text_stringz(&mapEncoder, g_device_config.manf_info.secretkey.value);

    // Close map
    cbor_encoder_close_container(&encoder, &mapEncoder);

    bufferLength = cbor_encoder_get_buffer_size(&encoder, buffer);
    if (bufferLength > GEN_BUFFER_SIZE) {
        ESP_LOGE(TAG.c_str(), "CBOR buffer overflow: %d bytes (max %d)", (int)bufferLength, GEN_BUFFER_SIZE);
        return nullptr;
    }

    return buffer;
}