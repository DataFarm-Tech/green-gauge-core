#pragma once

#include "IConnection.hpp"
#include "UARTDriver.hpp"

/**
 * @enum SimStatus
 * @brief Represents the current status of the SIM connection.
 */
enum class SimStatus : uint8_t {
    DISCONNECTED,    // Modem off / UART not init
    INIT,            // UART init, modem ping
    NOTREADY,        // SIM not ready (CPIN != READY)
    REGISTERING,     // SIM ready, waiting for network
    CONNECTED,       // Registered on network
    ERROR            // Fatal / unrecoverable
};

/**
 * @enum MsgType
 * @brief Represents a type of AT command
 */
enum class MsgType : uint8_t {
    INIT,
    STATUS,
    NETREG,
    SHUTDOWN
};

/**
 * @struct ATCommand_t
 * @brief Represents a AT command.
 */
typedef struct {
    const char* cmd;        // AT command string
    const char* expect;     // Expected response substring
    int timeout_ms;         // Timeout in milliseconds
    MsgType msg_type;       // Purpose of this command
} ATCommand_t;

/**
 * @class SimConnection
 * @brief Implements a cellular SIM-based network connection.
 *
 * Provides methods to connect, check connection status, and disconnect
 * from a SIM-based cellular network.
 */
class SimConnection : public IConnection {
public:
    /**
     * @brief Establishes a connection to the SIM network.
     *
     * @return true if the connection was successful, false otherwise.
     */
    bool connect() override;

    /**
     * @brief Checks if the SIM network connection is currently active.
     *
     * @return true if connected, false otherwise.
     */
    bool isConnected() override;

    /**
     * @brief Disconnects from the SIM network.
     */
    void disconnect() override;

private:
    static constexpr uint32_t MAX_RETRIES = 5;
    UARTDriver m_modem_uart { UART_NUM_1 };   // UART connected to Quectel
    SimStatus   m_status { SimStatus::DISCONNECTED };
    uint32_t    m_retry_count { 0 };
    TaskHandle_t m_task { nullptr };
    SemaphoreHandle_t m_mutex { nullptr }; // protects m_status, m_retry_count


    /**
     * @brief Table of all AT commands for the FSM.
     */
    static inline const ATCommand_t at_command_table[] = {
        { "AT", "OK", 1000, MsgType::INIT },
        { "ATE0", "OK", 1000, MsgType::INIT },
        { "AT+CPIN?", "READY", 2000, MsgType::STATUS },
        { "AT+CEREG?", ",1", 3000, MsgType::NETREG },
        { "AT+CEREG?", ",5", 3000, MsgType::NETREG },
        { "AT+CFUN=0", "OK", 3000, MsgType::SHUTDOWN }
    };

    /**
     * @brief Send an AT command and wait for expected response.
     */
    bool sendAT(const ATCommand_t& atCmd);

    /**
     * @brief Finite State Machine task for managing SIM connection.
     */
    static void tick(void * arg);
};
