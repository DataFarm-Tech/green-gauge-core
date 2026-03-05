#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include <stdint.h>

class Utils
{
public:
	static unsigned short hexStringToInt(const std::string &hexStr);
	static std::string bytesToHexString(unsigned char high, unsigned char low);
	static void trimTrailingWhitespace(std::string &value);
	static std::string parseGPSLine(const std::string &line);
};