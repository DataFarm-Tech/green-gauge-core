#include "Utils.hpp"

#include <cmath>
#include <limits>
#include <cstdlib>
#include <cstdio>

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