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
	/**
	 * @brief Print a message of the day (MOTD) to the console, including device information and build details.
	 */
	static void printMotd();
    
	/**
	 * @brief Convert a hexadecimal string to an unsigned short integer.
	 * @param hexStr The hexadecimal string to convert.
	 * @return The converted unsigned short integer.
	 * @throws std::invalid_argument if the string is not a valid hexadecimal number.
	 */
	static unsigned short hexStringToInt(const std::string &hexStr);
	
	/**
	 * @brief Convert two bytes (high and low) to a hexadecimal string representation.
	 * @param high The high byte.
	 * @param low The low byte.
	 * @return A string representing the hexadecimal value of the combined bytes.
	 */
	static std::string bytesToHexString(unsigned char high, unsigned char low);
	
	/**
	 * @brief Extract a chunk of the CoAP payload from the source string, ensuring it does not exceed the specified length.
	 * @param src The source string containing the CoAP payload.
	 * @param src_len The length of the source string.
	 * @param out A reference to a string where the extracted payload chunk will be stored.
	 * @return The size of the extracted payload chunk.
	 * @throws std::invalid_argument if the source string is empty or if the specified length
	 */
	static size_t extractCoapPayloadChunk(const char *src, size_t src_len, std::string &out);
	
	/**
	 * @brief Convert a string to lowercase ASCII characters.
	 * @param value The string to convert.
	 * @return The converted string with all characters in lowercase ASCII.
	 * @throws std::invalid_argument if the input string contains non-ASCII characters.
	 */
	static std::string toLowerAscii(std::string value);
	
	/**
	 * @brief Parse an HTTPS URL into its components: host, port, and path.
	 * @param url The HTTPS URL to parse.
	 * @param host A reference to a string where the extracted host will be stored.
	 * @param port A reference to a uint16_t where the extracted port will be stored.
	 * @param path A reference to a string where the extracted path will be stored.
	 * @return True if the URL was successfully parsed, false otherwise.
	 */

	static bool parseHttpsUrl(const std::string &url,
	                         std::string &host,
	                         uint16_t &port,
	                         std::string &path);
	
	/**
	 * @brief Build an HTTPS GET request string for the specified host and path, including a User-Agent header.
	 * @param host The host to which the GET request will be sent.
	 * @param path The path for the GET request.
	 * @param userAgent The User-Agent string to include in the request header (default is "green-gauge-ota/1.0").
	 * @return A string representing the complete
	 */
	static std::string buildHttpsGetRequest(const std::string &host,
	                                      const std::string &path,
	                                      const std::string &userAgent = "green-gauge-ota/1.0");
	
	/**
	 * @brief Trim trailing whitespace characters from a string.
	 * @param value The string from which to trim trailing whitespace.
	 * @throws std::invalid_argument if the input string is empty.
	 */
	static void trimTrailingWhitespace(std::string &value);
	
	/**
	 * @brief Parse a GPS coordinate line in the format "latitude,longitude" and return it as a string.
	 * @param line The GPS coordinate line to parse.
	 * @return A string representing the parsed GPS coordinates in the format "latitude,longitude".
	 * @throws std::invalid_argument if the input line is not in the expected format or
	 */
	static std::string parseGPSLine(const std::string &line);

	/**
	 * @brief Print the device configuration to the console for debugging purposes.
	 * @param cfg The device configuration to print.
	 * @param source A string indicating the source of the configuration (e.g., "Loaded from flash", "Default configuration")
	 */
	static void printDeviceConfig(const DeviceConfig &cfg, const char *source);
};
