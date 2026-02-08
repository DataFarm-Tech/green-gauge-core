#include "ReadingPkt.hpp"
#include "psa/crypto.h"
#include "Config.hpp"

const uint8_t * ReadingPkt::toBuffer()
{
    CborEncoder encoder, mapEncoder, arrayEncoder;
    cbor_encoder_init(&encoder, buffer, GEN_BUFFER_SIZE, 0);

    // Create root map with 3 entries: node_id, measurement_type, and readings
    if (cbor_encoder_create_map(&encoder, &mapEncoder, 3) != CborNoError) 
    {
        // ESP_LOGE(TAG.c_str(), "Failed to create root map");
        return nullptr;
    }

    // Encode node_id
    if (cbor_encode_text_stringz(&mapEncoder, "node_id") != CborNoError ||
        cbor_encode_text_stringz(&mapEncoder, this->node_id.c_str()) != CborNoError)
    {
        // ESP_LOGE(TAG.c_str(), "Failed to encode node_id");
        return nullptr;
    }

    if (cbor_encode_text_stringz(&mapEncoder, "m_type") != CborNoError ||
    cbor_encode_text_stringz(&mapEncoder, mTypeToString()) != CborNoError)
    {
        return nullptr;
    }


    // Encode readings array (just the float values)
    if (cbor_encode_text_stringz(&mapEncoder, "readings") != CborNoError)
    {
        // ESP_LOGE(TAG.c_str(), "Failed to encode 'readings' key");
        return nullptr;
    }

    if (cbor_encoder_create_array(&mapEncoder, &arrayEncoder, NPK_COLLECT_SIZE) != CborNoError)
    {
        // ESP_LOGE(TAG.c_str(), "Failed to create readings array");
        return nullptr;
    }

    // Encode each reading value
    for (size_t i = 0; i < NPK_COLLECT_SIZE; i++)
    {
        if (cbor_encode_float(&arrayEncoder, this->reading[i]) != CborNoError)
        {
            // ESP_LOGE(TAG.c_str(), "Failed to encode reading %d", (int)i);
            return nullptr;
        }
    }

    // Close readings array
    if (cbor_encoder_close_container(&mapEncoder, &arrayEncoder) != CborNoError)
    {
        // ESP_LOGE(TAG.c_str(), "Failed to close readings array");
        return nullptr;
    }

    // Close root map
    if (cbor_encoder_close_container(&encoder, &mapEncoder) != CborNoError)
    {
        // ESP_LOGE(TAG.c_str(), "Failed to close root map");
        return nullptr;
    }

    bufferLength = cbor_encoder_get_buffer_size(&encoder, buffer);
    if (bufferLength > GEN_BUFFER_SIZE) {
        // ESP_LOGE(TAG.c_str(), "CBOR buffer overflow: %d bytes (max %d)", (int)bufferLength, GEN_BUFFER_SIZE);
        return nullptr;
    }
    
    // ESP_LOGI(TAG.c_str(), "CBOR encoding complete: %d bytes for %s", (int)bufferLength, measurementType.c_str());
    return buffer;
}


const char* ReadingPkt::mTypeToString() const
{
    switch (m_type)
    {
        case MeasurementType::Nitrogen:
            return NITROGEN;
        case MeasurementType::Phosphorus:
            return PHOSPHORUS;
        case MeasurementType::Potassium:
            return POTASSIUM;
        case MeasurementType::Moisture:
            return MOISTURE;
        case MeasurementType::PH:
            return PH;
        case MeasurementType::Temperature:
            return TEMPERATURE;
        default:
            return "unknown";
    }
}
