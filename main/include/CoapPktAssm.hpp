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
#define COAP_CODE_DETAIL_PUT        3
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
	Reading,
	GpsUpdate
};

enum CoapMethod 
{
	POST,
	PUT
};

typedef struct {
	PktType pkt_type;
	CoapMethod method;
} PktEntry_t;
class CoapPktAssm
{
public:
	/**
	 * @brief 
	 * @param coap_buffer 
	 * @param pkt_type
	 * @param buffer
	 * @param buffler_len
	 */
	static size_t buildCoapBuffer(uint8_t coap_buffer[], const uint8_t *buffer, const size_t buffer_len, PktEntry_t pkt_config);
	
	/**
	 * @brief Get the URI path string based on the packet type
	 * @param pkt_type The type of the packet (Activate or Reading)
	 * @return The corresponding URI path string (e.g., "activate" or "reading
	 */
	static std::string getUriPath(PktType pkt_type);


private:
	/**
	 * @brief Set the CoAP header fields (Version, Type, Token Length)
	 * @param buffer Pointer to the buffer where the header will be written
	 * @param version CoAP version (should be 1)
	 * @param type CoAP message type (CON, NON, ACK, RST)
	 * @param token_len Length of the token (0-8 bytes)
	 * @return Number of bytes written to the buffer (should be 1)
	 */
	static size_t setHeader(uint8_t *buffer, uint8_t version, uint8_t type, uint8_t token_len);
	/**
	 * @brief Set the CoAP code field (Class and Detail)
	 * @param buffer Pointer to the buffer where the code will be written
	 * @param code_class CoAP code class (0 for requests, 2 for success
	 * responses, 4 for client errors, 5 for server errors)
	 * @param code_detail CoAP code detail (specific to the class)
	 * @return Number of bytes written to the buffer (should be 1)
	 */
	static size_t setCode(uint8_t *buffer, uint8_t code_class, uint8_t code_detail);
	
	/**
	 * @brief Set the CoAP Message ID field
	 * @param buffer Pointer to the buffer where the Message ID will be written
	 * @param msg_id 16-bit Message ID to set
	 * @return Number of bytes written to the buffer (should be 2)
	 */
	static size_t setMessageId(uint8_t *buffer, uint16_t msg_id);
	
	/**
	 * @brief Set the CoAP Token field
	 * @param buffer Pointer to the buffer where the token will be written
	 * @param token Pointer to the token bytes
	 * @param token_len Length of the token (0-8 bytes)
	 * @return Number of bytes written to the buffer (should be equal to token_len)
	 */
	static size_t setToken(uint8_t *buffer, const uint8_t *token, uint8_t token_len);
	
	/**
	 * @brief Set the Uri-Path option in the CoAP packet
	 * @param buffer Pointer to the buffer where the option will be written
	 * @param uri_path The URI path string to set (e.g., "activate"
	 * 	or "reading")
	 * @param delta The option delta value (should be COAP_DELTA_URI_PATH)
	 * @return Number of bytes written to the buffer (varies based on uri_path length
	 */
	static size_t setUriPathOption(uint8_t *buffer, const std::string &uri_path, uint8_t delta);
	/**
	 * @brief Set the Content-Format option in the CoAP packet
	 * @param buffer Pointer to the buffer where the option will be written
	 * @param content_format The content format value (e.g., COAP_CONTENT_FORMAT_CB
	 * 	OR)
	 * @param delta The option delta value (should be COAP_DELTA_CONTENT_FORMAT)
	 * @return Number of bytes written to the buffer (should be 2)
	 */
	static size_t setContentFormatOption(uint8_t *buffer, uint8_t content_format, uint8_t delta);
	
	/**
	 * @brief Set the Payload Marker in the CoAP packet
	 * @param buffer Pointer to the buffer where the payload marker will be written
	 * @return Number of bytes written to the buffer (should be 1)
	 */
	static size_t setPayloadMarker(uint8_t *buffer);
	
	/**
	 * @brief Set the payload data in the CoAP packet
	 * @param buffer Pointer to the buffer where the payload will be written
	 * @param payload Pointer to the payload data bytes
	 * @param payload_len Length of the payload data in bytes
	 * @return Number of bytes written to the buffer (should be equal to payload_len)
	 */
	static size_t setPayload(uint8_t *buffer, const uint8_t *payload, size_t payload_len);
	
	/**
	 * @brief Set a generic CoAP option with specified delta, length, and value
	 * @param buffer Pointer to the buffer where the option will be written
	 * @param delta The option delta value (relative to the previous option)
	 * @param length The length of the option value in bytes
	 * @param value Pointer to the option value bytes
	 * @return Number of bytes written to the buffer (varies based on length)
	 */
	static size_t setOption(uint8_t *buffer, uint8_t delta, uint8_t length, const uint8_t *value);
	
	/**
	 * @brief Generate the next Message ID for CoAP packets
	 * @return A unique 16-bit Message ID value
	 */
	static uint16_t getNextMessageId();
};


extern PktEntry_t activate_entry;
extern PktEntry_t reading_entry;
extern PktEntry_t gpsupdate_entry;