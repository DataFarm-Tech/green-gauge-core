#include "Communication.hpp"
#include "WiFiConnection.hpp"
#include "SimConnection.hpp"

Communication::Communication(ConnectionType type) 
{
    switch (type) 
    {
        case ConnectionType::WIFI:
            connection = std::make_unique<WifiConnection>();
            break;
        case ConnectionType::SIM:
            connection = std::make_unique<SimConnection>();
            break;
    }
}

bool Communication::connect() 
{
    return connection->connect();
}

bool Communication::isConnected() 
{
    return connection->isConnected();
}

void Communication::disconnect() 
{
    connection->disconnect();
}