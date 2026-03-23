#include "ActivatePkt.hpp"
#include "psa/crypto.h"
#include "Config.hpp"
// #include "Logger.hpp"
#include "GPS.hpp"
#include "EEPROMConfig.hpp"

#define IS_U8_ARR_EMPTY(arr, len) (memcmp((arr), (uint8_t[len]){0}, (len)) == 0)

const uint8_t * ActivatePkt::toBuffer()
{
    CborEncoder encoder, mapEncoder;
    cbor_encoder_init(&encoder, buffer, GEN_BUFFER_SIZE, 0);
    if (cbor_encoder_create_map(&encoder, &mapEncoder, 9) != CborNoError)
    {
        ESP_LOGI("OK", "Failed to create root map");
        return nullptr;
    }

    if (IS_U8_ARR_EMPTY(g_device_config.secretKey, sizeof(g_device_config.secretKey))) {
        printf("No secret key available for activation packet\n");
        return nullptr;
    }

    // node_id
    cbor_encode_text_stringz(&mapEncoder, NODE_ID_KEY);
    cbor_encode_text_stringz(&mapEncoder, node_id.c_str());

    cbor_encode_text_stringz(&mapEncoder, SECRET_KEY_KEY);
    cbor_encode_text_stringz(&mapEncoder, secretKeyUser.c_str());

    // gps
    cbor_encode_text_stringz(&mapEncoder, GPS_KEY);
    cbor_encode_text_stringz(&mapEncoder, GPSCoord.c_str());

    // key as raw bytes
    cbor_encode_text_stringz(&mapEncoder, KEY_KEY);
    cbor_encode_byte_string(&mapEncoder, g_device_config.secretKey, sizeof(g_device_config.secretKey));

    cbor_encode_text_stringz(&mapEncoder, FW_VER_KEY);
    cbor_encode_text_stringz(&mapEncoder, g_device_config.manf_info.fw_ver.value);

    cbor_encode_text_stringz(&mapEncoder, HW_VER_KEY);
    cbor_encode_text_stringz(&mapEncoder, HwVer.c_str());

    cbor_encode_text_stringz(&mapEncoder, SIM_MOD_SN_KEY);
    cbor_encode_text_stringz(&mapEncoder, simModSN.c_str());

    cbor_encode_text_stringz(&mapEncoder, SIM_CARD_SN_KEY);
    cbor_encode_text_stringz(&mapEncoder, simCardSN.c_str());

    cbor_encode_text_stringz(&mapEncoder, CHASSIS_VER_KEY);
    cbor_encode_text_stringz(&mapEncoder, chassisVer.c_str());

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