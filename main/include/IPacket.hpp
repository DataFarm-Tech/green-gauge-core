#pragma once

#include <string>
#include <cstdint>
#include "coap3/coap.h"
#include <cbor.h>
#include <sys/socket.h>
#include "esp_log.h"
#include "coap3/coap.h"
#include <cstdint>
#include <cstddef>

class IPacket {
protected:
    std::string nodeId;
    size_t bufferLength = 0;

    static coap_response_t message_handler(
        coap_session_t * session, 
        const coap_pdu_t * sent, 
        const coap_pdu_t * received, 
        const coap_mid_t mid
    );

public:
    virtual ~IPacket() = default;

    virtual uint8_t* toBuffer() = 0;

    void sendPacket();
    
    size_t getBufferLength() const { return bufferLength; }
};
