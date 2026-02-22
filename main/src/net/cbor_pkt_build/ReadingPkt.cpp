#include "ReadingPkt.hpp"
#include "psa/crypto.h"
#include "Config.hpp"

const uint8_t * ReadingPkt::toBuffer()
{
    CborEncoder encoder, mapEncoder, arrayEncoder;
    cbor_encoder_init(&encoder, buffer, GEN_BUFFER_SIZE, 0);

    if (cbor_encoder_create_map(&encoder, &mapEncoder, 4) != CborNoError)
        return nullptr;

    // node_id
    if (cbor_encode_text_stringz(&mapEncoder, "node_id") != CborNoError ||
        cbor_encode_text_stringz(&mapEncoder, this->node_id.c_str()) != CborNoError)
        return nullptr;

    // m_type
    if (cbor_encode_text_stringz(&mapEncoder, "m_type") != CborNoError ||
        cbor_encode_text_stringz(&mapEncoder, mTypeToString()) != CborNoError)
        return nullptr;

    // readings array
    if (cbor_encode_text_stringz(&mapEncoder, "readings") != CborNoError)
        return nullptr;

    if (cbor_encoder_create_array(&mapEncoder, &arrayEncoder, NPK_COLLECT_SIZE) != CborNoError)
        return nullptr;

    for (size_t i = 0; i < NPK_COLLECT_SIZE; i++)
    {
        if (cbor_encode_float(&arrayEncoder, this->reading[i]) != CborNoError)
            return nullptr;
    }

    // Close readings array before writing any more map entries
    if (cbor_encoder_close_container(&mapEncoder, &arrayEncoder) != CborNoError)
        return nullptr;

    // session
    if (cbor_encode_text_stringz(&mapEncoder, "session") != CborNoError ||
        cbor_encode_int(&mapEncoder, this->session_count) != CborNoError)
        return nullptr;

    // Close root map
    if (cbor_encoder_close_container(&encoder, &mapEncoder) != CborNoError)
        return nullptr;

    bufferLength = cbor_encoder_get_buffer_size(&encoder, buffer);
    if (bufferLength > GEN_BUFFER_SIZE)
        return nullptr;

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
