#include "Communication.hpp"
#include "WiFiConnection.hpp"
#include "SimConnection.hpp"
#include "EthernetConnection.hpp"

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
        case ConnectionType::ETHERNET:
            connection = std::make_unique<EthernetConnection>();
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

bool Communication::sendPacket(const uint8_t * pkt, const size_t pkt_len)
{
    return connection->sendPacket(pkt, pkt_len);
}