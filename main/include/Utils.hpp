#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdexcept>

class Utils
{
public:
	static uint16_t hexStringToInt(const std::string &hexStr);
	static std::string bytesToHexString(uint8_t high, uint8_t low);
};