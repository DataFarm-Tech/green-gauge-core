// SimConnection.hpp
#pragma once

#include "IConnection.hpp"

class SimConnection : public IConnection {
public:
    bool connect() override;
    bool isConnected() override;
    void disconnect() override;
};
