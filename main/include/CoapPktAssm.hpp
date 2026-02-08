#pragma once

#include <stdlib.h>
#include <string>
#include <unistd.h>

// CoAP Protocol Constants
#define COAP_VERSION                1
#define COAP_TYPE_CON               0
#define COAP_TYPE_NON               1
#define COAP_TYPE_ACK               2
#define COAP_TYPE_RST               3

// CoAP Method Codes
#define COAP_METHOD_GET             1
#define COAP_METHOD_POST            2
#define COAP_METHOD_PUT             3
#define COAP_METHOD_DELETE          4

// CoAP Code Classes
#define COAP_CODE_CLASS_REQUEST     0
#define COAP_CODE_CLASS_SUCCESS     2
#define COAP_CODE_CLASS_CLIENT_ERR  4
#define COAP_CODE_CLASS_SERVER_ERR  5

// CoAP Code Details
#define COAP_CODE_DETAIL_POST       2
#define COAP_CODE_DETAIL_CREATED    1
#define COAP_CODE_DETAIL_CHANGED    4

// CoAP Option Numbers
#define COAP_OPTION_URI_PATH        11
#define COAP_OPTION_CONTENT_FORMAT  12

// CoAP Option Deltas
#define COAP_DELTA_URI_PATH         11
#define COAP_DELTA_CONTENT_FORMAT   1

// CoAP Content Formats
#define COAP_CONTENT_FORMAT_TEXT    0
#define COAP_CONTENT_FORMAT_LINK    40
#define COAP_CONTENT_FORMAT_XML     41
#define COAP_CONTENT_FORMAT_OCTET   42
#define COAP_CONTENT_FORMAT_JSON    50
#define COAP_CONTENT_FORMAT_CBOR    60

// CoAP Protocol Limits
#define COAP_MAX_TOKEN_LEN          8
#define COAP_DEFAULT_TOKEN_LEN      4
#define COAP_OPTION_EXTENDED_LEN    13
#define COAP_OPTION_MAX_STANDARD    12

// CoAP Special Values
#define COAP_PAYLOAD_MARKER         0xFF
#define COAP_INITIAL_MSG_ID         0x1234

// Token Constants
#define COAP_TOKEN_BYTE_0           0x01
#define COAP_TOKEN_BYTE_1           0x02
#define COAP_TOKEN_BYTE_2           0x03
#define COAP_TOKEN_BYTE_3           0x04

// Bit Masks
#define COAP_VERSION_MASK           0x03
#define COAP_TYPE_MASK              0x03
#define COAP_TOKEN_LEN_MASK         0x0F
#define COAP_CODE_CLASS_MASK        0x07
#define COAP_CODE_DETAIL_MASK       0x1F
#define COAP_OPTION_DELTA_MASK      0x0F
#define COAP_OPTION_LENGTH_MASK     0x0F

// Bit Shifts
#define COAP_VERSION_SHIFT          6
#define COAP_TYPE_SHIFT             4
#define COAP_CODE_CLASS_SHIFT       5
#define COAP_OPTION_DELTA_SHIFT     4


enum PktType
{
	Activate,
	Reading
};

class CoapPktAssm
{
public:
	// static void buildCoapBuffer(uint8_t coap_buffer[]);
	/**
	 * @brief 
	 * @param coap_buffer 
	 * @param pkt_type
	 * @param buffer
	 * @param buffler_len
	 */
	static size_t buildCoapBuffer( uint8_t coap_buffer[], 
								   PktType pkt_type, 
								   const uint8_t * buffer, 
								   const size_t buffer_len);
	static std::string getUriPath(PktType pkt_type);


private:
	static size_t setHeader(uint8_t *buffer, uint8_t version, uint8_t type, uint8_t token_len);
	static size_t setCode(uint8_t *buffer, uint8_t code_class, uint8_t code_detail);
	static size_t setMessageId(uint8_t *buffer, uint16_t msg_id);
	static size_t setToken(uint8_t *buffer, const uint8_t *token, uint8_t token_len);
	static size_t setUriPathOption(uint8_t *buffer, const std::string &uri_path, uint8_t delta);
	static size_t setContentFormatOption(uint8_t *buffer, uint8_t content_format, uint8_t delta);
	static size_t setPayloadMarker(uint8_t *buffer);
	static size_t setPayload(uint8_t *buffer, const uint8_t *payload, size_t payload_len);
	
	// Helper methods
	static size_t setOption(uint8_t *buffer, uint8_t delta, uint8_t length, const uint8_t *value);
	static uint16_t getNextMessageId();
};