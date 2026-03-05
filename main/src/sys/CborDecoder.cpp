#include "CborDecoder.hpp"

#include <cbor.h>

bool CborDecoder::decodeText(CborStringTransform& transform)
{
    transform.outgoing.clear();

    if (transform.incoming.empty())
    {
        return false;
    }

    CborParser parser;
    CborValue root;
    if (cbor_parser_init(reinterpret_cast<const uint8_t*>(transform.incoming.data()), transform.incoming.size(), 0, &parser, &root) != CborNoError)
    {
        return false;
    }

    if (cbor_value_is_text_string(&root))
    {
        size_t str_len = 0;
        if (cbor_value_calculate_string_length(&root, &str_len) != CborNoError)
        {
            return false;
        }

        transform.outgoing.resize(str_len);
        if (cbor_value_copy_text_string(&root, transform.outgoing.data(), &str_len, nullptr) != CborNoError)
        {
            transform.outgoing.clear();
            return false;
        }

        transform.outgoing.resize(str_len);
        return true;
    }

    if (cbor_value_is_byte_string(&root))
    {
        size_t bytes_len = 0;
        if (cbor_value_calculate_string_length(&root, &bytes_len) != CborNoError)
        {
            return false;
        }

        std::string tmp(bytes_len, '\0');
        if (cbor_value_copy_byte_string(&root, reinterpret_cast<uint8_t*>(tmp.data()), &bytes_len, nullptr) != CborNoError)
        {
            return false;
        }

        tmp.resize(bytes_len);
        transform.outgoing = tmp;
        return true;
    }

    return false;
}

bool CborDecoder::decodeBytes(CborStringTransform& transform)
{
    transform.outgoing.clear();

    if (transform.incoming.empty())
    {
        return false;
    }

    CborParser parser;
    CborValue root;
    if (cbor_parser_init(reinterpret_cast<const uint8_t*>(transform.incoming.data()), transform.incoming.size(), 0, &parser, &root) != CborNoError)
    {
        return false;
    }

    if (!cbor_value_is_byte_string(&root))
    {
        return false;
    }

    size_t bytes_len = 0;
    if (cbor_value_calculate_string_length(&root, &bytes_len) != CborNoError)
    {
        return false;
    }

    transform.outgoing.resize(bytes_len);
    if (cbor_value_copy_byte_string(&root, reinterpret_cast<uint8_t*>(transform.outgoing.data()), &bytes_len, nullptr) != CborNoError)
    {
        transform.outgoing.clear();
        return false;
    }

    transform.outgoing.resize(bytes_len);
    return true;
}
