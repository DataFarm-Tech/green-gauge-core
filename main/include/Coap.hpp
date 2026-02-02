#pragma once

#include <stdlib.h>
#include <string>
#include <unistd.h>

enum PktType
{
	Activate,
	Reading
};

class Coap
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
			PktType pkt_type, const uint8_t *buffer, const size_t buffer_len);
	static std::string getUriPath(PktType pkt_type);
};