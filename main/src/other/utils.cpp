#include "Utils.hpp"

uint16_t Utils::hexStringToInt(const std::string &hexStr)
{
	uint16_t result = static_cast<uint16_t>(std::stoul(hexStr, nullptr, 16));
	return result;
}

std::string Utils::bytesToHexString(uint8_t high, uint8_t low)
{
	std::stringstream ss;

	ss << std::hex << std::setfill('0') << std::setw(2) << (int)high
	   << std::setw(2) << (int)low;

	return ss.str();
}