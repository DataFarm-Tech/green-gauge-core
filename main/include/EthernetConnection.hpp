#pragma once

#include "IConnection.hpp"

class EthernetConnection : public IConnection {
public:
    bool connect() override;
    bool isConnected() override;
    void disconnect() override;
};
