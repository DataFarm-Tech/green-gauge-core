#pragma once

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include <stdint.h>

#include "DeviceConfig.hpp"

class Utils
{
public:
	static void printMotd();
    static unsigned short hexStringToInt(const std::string &hexStr);
	static std::string bytesToHexString(unsigned char high, unsigned char low);
	static size_t extractCoapPayloadChunk(const char *src, size_t src_len, std::string &out);
	static std::string toLowerAscii(std::string value);
	static bool parseHttpsUrl(const std::string &url,
	                         std::string &host,
	                         uint16_t &port,
	                         std::string &path);
	static std::string buildHttpsGetRequest(const std::string &host,
	                                      const std::string &path,
	                                      const std::string &userAgent = "green-gauge-ota/1.0");
	static void trimTrailingWhitespace(std::string &value);
	static std::string parseGPSLine(const std::string &line);
	static void printDeviceConfig(const DeviceConfig &cfg, const char *source);
};
