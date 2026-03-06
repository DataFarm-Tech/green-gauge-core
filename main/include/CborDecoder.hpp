#pragma once

#include <string>

struct CborStringTransform {
    std::string incoming;
    std::string outgoing;
};

class CborDecoder {
public:
    /**
     * @brief Decodes transform.incoming as CBOR text/bytes into transform.outgoing.
     */
    static bool decodeText(CborStringTransform& transform);
    
    /**
     * @brief Decodes transform.incoming as CBOR byte string into transform.outgoing.
     */
    static bool decodeBytes(CborStringTransform& transform);
};
