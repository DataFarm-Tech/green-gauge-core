// Communication.hpp
#pragma once

#include <memory>
#include "IConnection.hpp"

enum class ConnectionType {
    WIFI,
    SIM
};

class Communication {
public:
    Communication(ConnectionType type);
    bool connect();
    bool isConnected();
    void disconnect();

private:
    std::unique_ptr<IConnection> connection;
};
