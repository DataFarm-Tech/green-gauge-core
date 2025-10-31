#pragma once

#include <cstdint>
#include <cstddef>
#include <cbor.h>
#include <sys/socket.h>
#include "esp_log.h"
#include "coap3/coap.h"

class Packet {
public:
    Packet();
    ~Packet() = default;

    /**
     * @brief Creates a CBOR payload containing temperature and humidity data.
     *
     * This function encodes the temperature and humidity values into the internal buffer
     * using CBOR format. The buffer length is updated accordingly.
     *
     * @param temp The temperature value to encode.
     * @param humidity The humidity value to encode.
     * @return true if the payload was successfully created, false otherwise.
     */
    bool createPayload(int temp, int humidity);

    /**
     * @brief Returns a pointer to the internal data buffer.
     *
     * This function provides read-only access to the encoded CBOR payload.
     *
     * @return Pointer to the buffer containing the payload.
     */
    const uint8_t * data() const;
    
    /**
     * @brief Returns the length of the data in the internal buffer.
     *
     * @return Number of bytes currently stored in the buffer.
     */
    size_t length() const;

    /**
     * @brief Sends the current payload over CoAP.
     *
     * This function creates a CoAP client session and sends the internal buffer as
     * a CoAP POST message. Response handling is done via the internal message handler.
     */
    void sendCoAP();

private:
    static constexpr size_t BUFFER_SIZE = 128;
    uint8_t buffer[BUFFER_SIZE];
    size_t bufferLength;
        
    /**
     * @brief CoAP response handler for received messages.
     *
     * This static function is used as a callback for CoAP responses. It is called
     * when a response is received for a sent CoAP message.
     *
     * @param session The CoAP session used for communication.
     * @param sent The original CoAP PDU that was sent.
     * @param received The CoAP PDU received in response.
     * @param mid The message ID of the sent message.
     * @return A coap_response_t indicating the handling status.
     */
    static coap_response_t message_handler(coap_session_t * session, const coap_pdu_t * sent, const coap_pdu_t * received, const coap_mid_t mid);

    static int resp_wait;           /**< Flag to indicate response waiting state */
    static coap_optlist_t *optlist; /**< Pointer to CoAP option list for message creation */
    static int wait_ms;             /**< Timeout in milliseconds for waiting for CoAP response */
};
