#pragma once

#include <sys/socket.h>
#include <string>
#include "IPacket.hpp"

#include <cbor.h>
#include "esp_log.h"
#include <cstdint>
#include <cstddef>
#include "GPS.hpp"
#include "Key.hpp"

/**
 * @class BatteryPacket
 * @brief Represents a battery data packet that can be serialized to CBOR and sent via CoAP.
 *
 * The BatteryPacket class extends the IPacket interface and provides functionality for reading
 * battery data (level, health) and encoding it into CBOR format before transmission over CoAP.
 */
class ActivatePkt : public IPacket
{
private:
    std::string secretKeyUser;
    std::string GPSCoord;
    std::string HwVer;
    std::string simModSN;
    std::string simCardSN;
    std::string chassisVer;
    
    static constexpr const char* NODE_ID_KEY = "node_id";
    static constexpr const char* GPS_KEY = "gps";
    static constexpr const char* SECRET_KEY_KEY = "secretkey";
    static constexpr const char* KEY_KEY = "key";
    static constexpr const char* FW_VER_KEY = "fw_ver";
    static constexpr const char* HW_VER_KEY = "hw_ver";
    static constexpr const char* SIM_MOD_SN_KEY = "sim_mod_sn";
    static constexpr const char* SIM_CARD_SN_KEY = "sim_card_sn";
    static constexpr const char* CHASSIS_VER_KEY = "chassis_ver";

public:
    ActivatePkt(PktType _pkt_type, std::string _node_id, std::string _uri, std::string _secret_key, std::string _gps_coord, std::string _hw_ver, std::string _sim_mod_sn, std::string _sim_card_sn, std::string _chassis_ver)
        : IPacket(_pkt_type, _node_id, _uri), secretKeyUser(_secret_key), GPSCoord(_gps_coord), HwVer(_hw_ver), simModSN(_sim_mod_sn), simCardSN(_sim_card_sn), chassisVer(_chassis_ver)
    {
    }

    /**
     * @brief Converts the current BatteryPacket data to a CBOR-encoded byte buffer.
     *
     * @return Pointer to the encoded CBOR buffer, or nullptr if encoding fails.
     *
     * This method encodes the node ID, battery level, and battery health into a CBOR payload
     * suitable for transmission over a CoAP connection.
     */
    const uint8_t *toBuffer() override;
};