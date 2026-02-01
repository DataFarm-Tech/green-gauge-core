#pragma once

#include <stdlib.h>

enum PktType
{
	Activate,
	Reading
};

class Coap
{
public:
	static void buildCoapBuffer(uint8_t coap_buffer[]);
	static std::string getUriPath(PktType pkt_type);
};