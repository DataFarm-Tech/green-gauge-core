#pragma once

#include <string>
#include "ATCommandHndlr.hpp"

class GPS {
public:
    GPS();
    /**
     * @brief Query modem for GPS coordinates and return formatted "lat, lon" string.
     * @param out String to receive formatted coordinates
     * @return true if coordinates retrieved and parsed, false otherwise
     */
    bool getCoordinates(std::string &out);

private:
    ATCommandHndlr m_hndlr;
    std::string parseGPSLine(const std::string &line) const;
};
