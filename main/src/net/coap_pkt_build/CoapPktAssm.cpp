#include "CoapPktAssm.hpp"
#include <unistd.h>
#include <cstring>
#include "Logger.hpp"

PktEntry_t activate_entry = {PktType::Activate, CoapMethod::POST};
PktEntry_t reading_entry = {PktType::Reading, CoapMethod::POST};
PktEntry_t gpsupdate_entry = {PktType::GpsUpdate, CoapMethod::PUT};


size_t CoapPktAssm::buildCoapBuffer(uint8_t coap_buffer[], const uint8_t *buffer, const size_t buffer_len, PktEntry_t pkt_config) 
{
	g_logger.info("Building CoAP packet from CBOR payload (%zu bytes)\n", buffer_len);

	size_t offset = 0;
	uint8_t code_detail = 0;
	
	// CoAP Header
	offset += setHeader(&coap_buffer[offset], COAP_VERSION, COAP_TYPE_CON, COAP_DEFAULT_TOKEN_LEN);
	
	// Method Code: POST (0.02) or PUT (0.03)
	switch (pkt_config.method)
	{
	case CoapMethod::PUT:
		code_detail = COAP_CODE_DETAIL_PUT;
		break;
	case CoapMethod::POST:
		code_detail = COAP_CODE_DETAIL_POST;
		break;
	default:
		break;
	}

	offset += setCode(&coap_buffer[offset], COAP_CODE_CLASS_REQUEST, code_detail);
	
	// Message ID
	uint16_t msg_id = getNextMessageId();
	offset += setMessageId(&coap_buffer[offset], msg_id);
	
	// Token (4 bytes)
	const uint8_t token[COAP_DEFAULT_TOKEN_LEN] = {
		COAP_TOKEN_BYTE_0, 
		COAP_TOKEN_BYTE_1, 
		COAP_TOKEN_BYTE_2, 
		COAP_TOKEN_BYTE_3
	};
	offset += setToken(&coap_buffer[offset], token, COAP_DEFAULT_TOKEN_LEN);
	
	// Uri-Path option
	std::string uri_path = getUriPath(pkt_config.pkt_type);
	offset += setUriPathOption(&coap_buffer[offset], uri_path, COAP_DELTA_URI_PATH);
	
	// Content-Format option (CBOR)
	offset += setContentFormatOption(&coap_buffer[offset], COAP_CONTENT_FORMAT_CBOR, COAP_DELTA_CONTENT_FORMAT);
	
	// Payload marker
	offset += setPayloadMarker(&coap_buffer[offset]);
	
	// CBOR payload
	offset += setPayload(&coap_buffer[offset], buffer, buffer_len);
	
	g_logger.info("CoAP packet built: %zu bytes total\n", offset);
	
	return offset;
}

size_t CoapPktAssm::setHeader(uint8_t *buffer, uint8_t version, uint8_t type, uint8_t token_len)
{
	// Byte 0: Ver(2 bits) | Type(2 bits) | TKL(4 bits)
	buffer[0] = ((version & COAP_VERSION_MASK) << COAP_VERSION_SHIFT) | 
	            ((type & COAP_TYPE_MASK) << COAP_TYPE_SHIFT) | 
	            (token_len & COAP_TOKEN_LEN_MASK);
	return 1;
}

size_t CoapPktAssm::setCode(uint8_t *buffer, uint8_t code_class, uint8_t code_detail)
{
	// Byte 1: Class(3 bits) | Detail(5 bits)
	buffer[0] = ((code_class & COAP_CODE_CLASS_MASK) << COAP_CODE_CLASS_SHIFT) | 
	            (code_detail & COAP_CODE_DETAIL_MASK);
	return 1;
}

size_t CoapPktAssm::setMessageId(uint8_t *buffer, uint16_t msg_id)
{
	// Bytes 2-3: Message ID (big-endian)
	buffer[0] = static_cast<uint8_t>((msg_id >> 8) & 0xFF);
	buffer[1] = static_cast<uint8_t>(msg_id & 0xFF);
	return 2;
}

size_t CoapPktAssm::setToken(uint8_t *buffer, const uint8_t *token, uint8_t token_len)
{
	if (token_len > COAP_MAX_TOKEN_LEN)
	{
		g_logger.error("Token length exceeds maximum of %d bytes\n", COAP_MAX_TOKEN_LEN);
		return 0;
	}
	
	memcpy(buffer, token, token_len);
	return token_len;
}

size_t CoapPktAssm::setUriPathOption(uint8_t *buffer, const std::string &uri_path, uint8_t delta)
{
	size_t uri_len = uri_path.length();
	
	if (uri_len > COAP_OPTION_MAX_STANDARD)
	{
		// Extended length encoding needed
		g_logger.warning("Uri-Path length > %d, using extended encoding\n", COAP_OPTION_MAX_STANDARD);
		buffer[0] = ((delta & COAP_OPTION_DELTA_MASK) << COAP_OPTION_DELTA_SHIFT) | COAP_OPTION_EXTENDED_LEN;
		buffer[1] = static_cast<uint8_t>(uri_len - COAP_OPTION_EXTENDED_LEN);
		memcpy(&buffer[2], uri_path.c_str(), uri_len);
		return 2 + uri_len;
	}
	else
	{
		// Standard encoding: delta(4 bits) | length(4 bits)
		buffer[0] = ((delta & COAP_OPTION_DELTA_MASK) << COAP_OPTION_DELTA_SHIFT) | 
		            (uri_len & COAP_OPTION_LENGTH_MASK);
		memcpy(&buffer[1], uri_path.c_str(), uri_len);
		return 1 + uri_len;
	}
}

size_t CoapPktAssm::setContentFormatOption(uint8_t *buffer, uint8_t content_format, uint8_t delta)
{
	// Option header: delta(4 bits) | length(4 bits)
	buffer[0] = ((delta & COAP_OPTION_DELTA_MASK) << COAP_OPTION_DELTA_SHIFT) | 0x01; // length = 1
	buffer[1] = content_format;
	return 2;
}

size_t CoapPktAssm::setPayloadMarker(uint8_t *buffer)
{
	buffer[0] = COAP_PAYLOAD_MARKER;
	return 1;
}

size_t CoapPktAssm::setPayload(uint8_t *buffer, const uint8_t *payload, size_t payload_len)
{
	memcpy(buffer, payload, payload_len);
	return payload_len;
}

size_t CoapPktAssm::setOption(uint8_t *buffer, uint8_t delta, uint8_t length, const uint8_t *value)
{
	size_t offset = 0;
	
	// Option header
	buffer[offset++] = ((delta & COAP_OPTION_DELTA_MASK) << COAP_OPTION_DELTA_SHIFT) | 
	                   (length & COAP_OPTION_LENGTH_MASK);
	
	// Option value
	if (value && length > 0)
	{
		memcpy(&buffer[offset], value, length);
		offset += length;
	}
	
	return offset;
}

uint16_t CoapPktAssm::getNextMessageId()
{
	static uint16_t msg_id = COAP_INITIAL_MSG_ID;
	return msg_id++;
}

std::string CoapPktAssm::getUriPath(PktType pkt_type)
{
	switch (pkt_type)
	{
	case PktType::Activate:
		return "activate";
	case PktType::Reading:
		return "reading";
	case PktType::GpsUpdate:
		return "gps-update";
	default:
		return "";
	}
}