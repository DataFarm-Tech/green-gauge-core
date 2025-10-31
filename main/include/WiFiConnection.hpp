#pragma once

#include "IConnection.hpp"

class WifiConnection : public IConnection {
public:
    bool connect() override;
    bool isConnected() override;
    void disconnect() override;
};
