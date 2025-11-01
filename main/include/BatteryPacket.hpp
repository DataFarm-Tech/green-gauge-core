#pragma once

#include <string>
#include "IPacket.hpp"

#include <cbor.h>
#include <sys/socket.h>
#include "esp_log.h"
#include "coap3/coap.h"
#include <cstdint>
#include <cstddef>

class BatteryPacket : public IPacket {
private:
    uint8_t level;
    uint8_t health;

public:
    BatteryPacket(const std::string& id, uint8_t lvl = 0, uint8_t hlth = 0)
        : level(lvl), health(hlth) { nodeId = id; }

    uint8_t * toBuffer() override;

    bool readFromBMS();

    uint8_t getLevel() const { return level; }
    uint8_t getHealth() const { return health; }
};
