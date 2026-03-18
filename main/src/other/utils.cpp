#include "Utils.hpp"
#include "CoapPktAssm.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>
#include <cstdlib>
#include <cstdio>

void Utils::printMotd() {
    printf("**********************************************************\n");
    printf("*                                                        *\n");
    printf("*  ____        _        _____                           *\n");
    printf("* |  _ \\  __ _| |_ __ _|  ___|_ _ _ __ _ __ ___        *\n");
    printf("* | | | |/ _` | __/ _` | |_ / _` | '__| '_ ` _ \\       *\n");
    printf("* | |_| | (_| | || (_| |  _| (_| | |  | | | | | |      *\n");
    printf("* |____/ \\__,_|\\__\\__,_|_|  \\__,_|_|  |_| |_| |_|      *\n");
    printf("*                                                        *\n");
    printf("*        All activity may be monitored.                 *\n");
    printf("*        Unauthorized access is prohibited.             *\n");
    printf("**********************************************************\n");
}

unsigned short Utils::hexStringToInt(const std::string &hexStr)
{
	unsigned short result = static_cast<unsigned short>(std::stoul(hexStr, nullptr, 16));
	return result;
}

std::string Utils::bytesToHexString(unsigned char high, unsigned char low)
{
	std::stringstream ss;

	ss << std::hex << std::setfill('0') << std::setw(2) << (int)high
	   << std::setw(2) << (int)low;

	return ss.str();
}

size_t Utils::extractCoapPayloadChunk(const char *src, size_t src_len, std::string &out)
{
	out.clear();

	if (!src || src_len == 0)
	{
		return 0;
	}

	size_t payload_start = src_len;
	for (size_t i = 0; i < src_len; ++i)
	{
		if (static_cast<uint8_t>(src[i]) == COAP_PAYLOAD_MARKER)
		{
			payload_start = i + 1;
			break;
		}
	}

	if (payload_start >= src_len)
	{
		return 0;
	}

	out.assign(src + payload_start, src_len - payload_start);

	return out.size();
}

std::string Utils::toLowerAscii(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return value;
}

bool Utils::parseHttpsUrl(const std::string &url,
	                      std::string &host,
	                      uint16_t &port,
	                      std::string &path)
{
	host.clear();
	path.clear();
	port = 443;

	static const std::string scheme = "https://";
	if (url.rfind(scheme, 0) != 0)
	{
		return false;
	}

	const size_t host_start = scheme.size();
	size_t path_start = url.find('/', host_start);

	std::string host_port;
	if (path_start == std::string::npos)
	{
		host_port = url.substr(host_start);
		path = "/";
	}
	else
	{
		host_port = url.substr(host_start, path_start - host_start);
		path = url.substr(path_start);
	}

	if (host_port.empty())
	{
		return false;
	}

	const size_t colon_pos = host_port.rfind(':');
	if (colon_pos != std::string::npos)
	{
		const std::string port_text = host_port.substr(colon_pos + 1);
		char *end_ptr = nullptr;
		const unsigned long parsed_port = std::strtoul(port_text.c_str(), &end_ptr, 10);
		if (end_ptr == nullptr || *end_ptr != '\0' || parsed_port == 0 || parsed_port > 65535)
		{
			return false;
		}

		host = host_port.substr(0, colon_pos);
		port = static_cast<uint16_t>(parsed_port);
	}
	else
	{
		host = host_port;
	}

	return !host.empty();
}

std::string Utils::buildHttpsGetRequest(const std::string &host,
	                                   const std::string &path,
	                                   const std::string &userAgent)
{
	const std::string request_path = path.empty() ? "/" : path;

	return "GET " + request_path + " HTTP/1.1\r\n"
	       "Host: " + host + "\r\n"
	       "User-Agent: " + userAgent + "\r\n"
	       "Connection: close\r\n"
	       "Accept: */*\r\n\r\n";
}

void Utils::trimTrailingWhitespace(std::string &value)
{
	while (!value.empty() &&
		   (value.back() == '\r' ||
			value.back() == '\n' ||
			value.back() == ' ' ||
			value.back() == '\t'))
	{
		value.pop_back();
	}
}

std::string Utils::parseGPSLine(const std::string &line)
{
	// Expecting line like: +QGPSLOC: 051127.0,3752.4632S,14504.0347E,2.2,22.0,2,...
	size_t start = line.find(':');
	if (start == std::string::npos)
		return "";
	start++;
	while (start < line.size() && line[start] == ' ')
		start++;

	size_t c1 = line.find(',', start);
	if (c1 == std::string::npos)
		return "";
	size_t c2 = line.find(',', c1 + 1);
	if (c2 == std::string::npos)
		return "";
	size_t c3 = line.find(',', c2 + 1);
	if (c3 == std::string::npos)
		return "";

	std::string lat = line.substr(c1 + 1, c2 - c1 - 1);
	std::string lon = line.substr(c2 + 1, c3 - c2 - 1);

	while (!lat.empty() && lat.front() == ' ')
		lat.erase(0, 1);
	while (!lon.empty() && lon.front() == ' ')
		lon.erase(0, 1);

	auto convertDMM = [](const std::string &dmm) -> double
	{
		if (dmm.empty())
			return std::numeric_limits<double>::quiet_NaN();

		char hemi = dmm.back();
		std::string core = dmm;
		if (hemi == 'N' || hemi == 'S' || hemi == 'E' || hemi == 'W')
		{
			core = dmm.substr(0, dmm.size() - 1);
		}
		else
		{
			hemi = '\0';
		}

		size_t s = 0;
		while (s < core.size() && core[s] == ' ')
			s++;
		size_t e = core.size();
		while (e > s && core[e - 1] == ' ')
			e--;
		core = core.substr(s, e - s);

		size_t dot = core.find('.');
		if (dot == std::string::npos || dot < 3)
			return std::numeric_limits<double>::quiet_NaN();

		int deg_digits = (dot > 4) ? 3 : 2;
		if (core.size() <= (size_t)deg_digits)
			return std::numeric_limits<double>::quiet_NaN();

		std::string deg_str = core.substr(0, deg_digits);
		std::string min_str = core.substr(deg_digits);

		char *endptr = nullptr;
		double deg = std::strtod(deg_str.c_str(), &endptr);
		if (endptr == deg_str.c_str())
			return std::numeric_limits<double>::quiet_NaN();
		double minutes = std::strtod(min_str.c_str(), &endptr);
		if (endptr == min_str.c_str())
			return std::numeric_limits<double>::quiet_NaN();

		double dec = deg + (minutes / 60.0);
		if (hemi == 'S' || hemi == 'W')
			dec = -dec;
		return dec;
	};

	double lat_dec = convertDMM(lat);
	double lon_dec = convertDMM(lon);
	if (std::isnan(lat_dec) || std::isnan(lon_dec))
		return "";

	char buf[64];
	snprintf(buf, sizeof(buf), "%.7f, %.7f", lat_dec, lon_dec);
	return std::string(buf);
}

void Utils::printDeviceConfig(const DeviceConfig &cfg, const char *source)
{
	printf("Device config (%s):\n", (source != nullptr) ? source : "unknown");
	printf("  activated=%s\n", cfg.has_activated ? "Yes" : "No");
	printf("  gps_coord=%s\n", cfg.gps_coord.c_str());
	printf("  main_app_delay=%llu\n", static_cast<unsigned long long>(cfg.main_app_delay));
	printf("  session_count=%llu\n", static_cast<unsigned long long>(cfg.session_count));
	printf("  secretKey=%s\n", cfg.secretKey);
	printf("  manf.hw_ver=%s\n", cfg.manf_info.hw_ver.value);
	printf("  manf.hw_var=%s\n", cfg.manf_info.hw_var.value);
	printf("  manf.fw_ver=%s\n", cfg.manf_info.fw_ver.value);
	printf("  manf.nodeId=%s\n", cfg.manf_info.nodeId.value);
	printf("  manf.secretkey=%s\n", cfg.manf_info.secretkey.value);
	printf("  manf.p_code=%s\n", cfg.manf_info.p_code.value);
	printf("  manf.sim_mod_sn=%s\n", cfg.manf_info.sim_mod_sn.value);
	printf("  manf.sim_card_sn=%s\n", cfg.manf_info.sim_card_sn.value);
	printf("  manf.chassis_ver=%s\n", cfg.manf_info.chassis_ver.value);
	printf("  calib.last_cal_ts=%llu\n", static_cast<unsigned long long>(cfg.calib.last_cal_ts));
}
