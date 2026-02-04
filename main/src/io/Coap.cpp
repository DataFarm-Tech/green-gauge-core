#include "Coap.hpp"
#include <unistd.h>
#include <cstring>
#include "Logger.hpp"

size_t Coap::buildCoapBuffer( uint8_t coap_buffer[], 
							PktType pkt_type, 
							const uint8_t *buffer, 
							const size_t buffer_len)
{
	g_logger.info("Building CoAP packet from CBOR payload (%zu bytes)\n", buffer_len);

	// Build CoAP packet with URI path

	size_t coap_len = 0;

	// CoAP Header (4 bytes minimum)
	// Byte 0: Ver(2) | Type(2) | TKL(4)
	// Version 1, CON type (0), Token length 4
	coap_buffer[coap_len++] = 0x44; // 01000100 = Ver:1, Type:CON, TKL:4

	// Byte 1: Code (0.02 = POST)
	coap_buffer[coap_len++] = 0x02;

	// Bytes 2-3: Message ID (random)
	static uint16_t msg_id = 0x1234;
	coap_buffer[coap_len++] = static_cast<uint8_t>((msg_id >> 8) & 0xFF);
	coap_buffer[coap_len++] = static_cast<uint8_t>(msg_id & 0xFF);
	msg_id++;

	// Token (4 bytes)
	coap_buffer[coap_len++] = 0x01;
	coap_buffer[coap_len++] = 0x02;
	coap_buffer[coap_len++] = 0x03;
	coap_buffer[coap_len++] = 0x04;

	// Option 11: Uri-Path = "activate" (delta=11, length=8)
	coap_buffer[coap_len++] = 0xB8; // 10111000 = delta:11, length:8

	std::string uri_path = Coap::getUriPath(pkt_type);
	for (char c : uri_path)
	{
		coap_buffer[coap_len++] = c;
	}

	// Option 12: Content-Format = 60 (CBOR) (delta=1, length=1)
	coap_buffer[coap_len++] = 0x11; // 00010001 = delta:1, length:1
	coap_buffer[coap_len++] = 60;

	// Payload marker
	coap_buffer[coap_len++] = 0xFF;

	// // Copy CBOR payload
	// if (coap_len + buffer_len > sizeof(coap_buffer))
	// {
	// 	g_logger.error("CoAP packet too large\n");
	// 	return false;
	// }
	memcpy(&coap_buffer[coap_len], buffer, buffer_len);
	coap_len += buffer_len;

	return coap_len;
}

std::string Coap::getUriPath(PktType pkt_type)
{
	switch (pkt_type)
	{
	case PktType::Activate:
		return "activate";
	case PktType::Reading:
		return "reading";
	default:
		return "";
	}
}