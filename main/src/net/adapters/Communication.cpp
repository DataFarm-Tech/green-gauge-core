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

bool Communication::startTelnetSession()
{
    return connection->startTelnetSession();
}

bool Communication::sendPacket(const uint8_t * pkt, const size_t pkt_len, PktEntry_t pkt_config)
{
    return connection->sendPacket(pkt, pkt_len, pkt_config, nullptr);
}

bool Communication::sendPacket(const uint8_t * pkt,
                               const size_t pkt_len,
                               const PktEntry_t pkt_config,
                               std::string &response)
{
    response.clear();
    return connection->sendPacket(pkt, pkt_len, pkt_config, &response);
}

bool Communication::sendPacketStream(const uint8_t * pkt,
                                     const size_t pkt_len,
                                     const PktEntry_t pkt_config,
                                     const PacketChunkCallback& onChunk)
{
    return connection->sendPacketStream(pkt, pkt_len, pkt_config, onChunk);
}