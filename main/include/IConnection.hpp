#pragma once

class IConnection {
public:
    virtual ~IConnection() = default;
    virtual bool connect() = 0;
    virtual bool isConnected() = 0;
    virtual void disconnect() = 0;
};
