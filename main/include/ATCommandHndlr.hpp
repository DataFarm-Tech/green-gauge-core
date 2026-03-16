#pragma once

#include <unistd.h>
#include <functional>
#include <string>

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
}

/**
 * @enum MsgType
 * @brief Represents a type of AT command
 */
enum class MsgType : uint8_t {
    INIT,
    STATUS,
    NETREG,
    SHUTDOWN,
    DATA
};

/**
 * @struct ATCommand_t
 * @brief Represents an AT command.
 */
typedef struct {
    const char * cmd;          // AT command string
    const char * expect;       // Expected response substring
    int timeout_ms;            // Timeout in milliseconds
    MsgType msg_type;          // Purpose of this command
    const uint8_t* payload;    // Optional payload data (nullptr if not needed)
    size_t payload_len;        // Length of payload (0 if not needed)
} ATCommand_t;

using ModemChunkCallback = std::function<bool(const uint8_t*, size_t)>;

/**
 * @class ATCommandHndlr
 * @brief Handles AT command transmission and response parsing for cellular modems.
 *
 * Provides methods to send AT commands via UART and parse responses with
 * timeout handling and expected response validation.
 */
class ATCommandHndlr {
public:
    /**
     * @brief Sends an AT command through the UART driver and waits for response.
     * 
     * If the command includes a payload, waits for '>' prompt before sending data.
     * 
     * @param atCmd The AT command structure containing command string, expected response, and timeout
     * @return true if the command succeeded and expected response was received, false otherwise
     */
    bool send(const ATCommand_t& atCmd);

    /**
     * @brief Send an AT command and capture the first line that contains the expected response.
     *
     * This is useful for commands like AT+QGPSLOC? which return a single-line response
     * containing the required data instead of an OK termination.
     *
     * @param atCmd AT command to send
     * @param out_buf Buffer to receive the response line
     * @param out_len Length of the provided buffer
     * @return true if an expected response line was captured, false otherwise
     */
    bool sendAndCapture(const ATCommand_t& atCmd, char* out_buf, size_t out_len);

    /**
     * @brief Opens an IP socket (TCP/UDP) using the modem.
     *
     * Sends `AT+QIOPEN` and validates the asynchronous `+QIOPEN: <id>,<err>` result.
     *
     * @param protocol Protocol string accepted by modem, e.g. "TCP" or "UDP"
     * @param host Remote host/IP
     * @param port Remote port
     * @param context_id PDP context id (default 1)
     * @param connect_id Socket id (default 0)
     * @param timeout_ms Timeout waiting for +QIOPEN URC
     * @return true if socket opened successfully (`err == 0`), false otherwise
     */
    bool openIPSocket(const char* protocol,
                      const char* host,
                      uint16_t port,
                      uint8_t context_id = 1,
                      uint8_t connect_id = 0,
                      int timeout_ms = 15000);

    /**
     * @brief Convenience wrapper for opening a TCP socket.
     */
    bool openTCPSocket(const char* host,
                       uint16_t port,
                       uint8_t context_id = 1,
                       uint8_t connect_id = 0,
                       int timeout_ms = 15000);

    /**
     * @brief Opens a modem-managed SSL/TLS socket using QSSLOPEN.
     *
     * Configures TLS context with permissive defaults for OTA fetches and
     * waits for asynchronous +QSSLOPEN result.
     */
    bool openSSLSocket(const char* host,
                       uint16_t port,
                       uint8_t ssl_context_id = 1,
                       uint8_t connect_id = 1,
                       int timeout_ms = 45000);

    /**
     * @brief Sends raw bytes through an already-open modem socket.
     */
    bool sendSocketData(uint8_t connect_id,
                        const uint8_t* payload,
                        size_t payload_len,
                        int timeout_ms = 5000);

    /**
     * @brief Reads pending bytes from an already-open modem socket using AT+QIRD.
     *
     * @param connect_id Socket id
     * @param out_buf Destination buffer
     * @param out_len Destination buffer length
     * @param out_read Receives number of bytes copied to out_buf
     * @param timeout_ms Read timeout
     * @param max_read Max bytes requested from modem in one QIRD
     * @return true if command completed (including zero bytes read), false on error/timeout
     */
    bool readSocketData(uint8_t connect_id,
                        char* out_buf,
                        size_t out_len,
                        size_t* out_read,
                        int timeout_ms = 3000,
                        size_t max_read = 256);

    /**
     * @brief Download HTTPS content via modem HTTP stack and stream body bytes.
     */
    bool httpGetStream(const std::string& url,
                       const ModemChunkCallback& onChunk,
                       int timeout_seconds = 180);

    /**
     * @brief Table of all AT commands for the FSM.
     */
    static inline const ATCommand_t at_command_table[] = {
        { "AT", "OK", 1000, MsgType::INIT, nullptr, 0 },
        { "ATE0", "OK", 1000, MsgType::INIT, nullptr, 0 },
        { "AT+CPIN?", "READY", 2000, MsgType::STATUS, nullptr, 0 },
        { "AT+QPING=1,\"8.8.8.8\",4,1", "+QPING:", 10000, MsgType::STATUS, nullptr, 0 },
        { "AT+CEREG?", ",1", 3000, MsgType::NETREG, nullptr, 0 },
        { "AT+CEREG?", ",5", 3000, MsgType::NETREG, nullptr, 0 },
        { "AT+CFUN=0", "OK", 3000, MsgType::SHUTDOWN, nullptr, 0 }
    };
    
private:
    /**
     * @struct ResponseState
     * @brief Maintains the state of response parsing for an AT command.
     */
    struct ResponseState {
        char line_buffer[256];      ///< Buffer for accumulating received line
        size_t line_len = 0;        ///< Current length of data in buffer
        bool got_expected = false;  ///< Flag indicating expected response was received
        bool success = false;       ///< Flag indicating overall command success
    };

    /**
     * @brief Processes incoming UART data byte-by-byte and accumulates response lines.
     * 
     * @param state Current response parsing state
     * @param atCmd The AT command being processed
     * @return true if response processing is complete (success or error), false if still waiting
     */
    bool processResponse(ResponseState& state, const ATCommand_t& atCmd);
    
    /**
     * @brief Handles a complete line received from the modem.
     * 
     * Checks for ERROR, expected response substring, and OK termination.
     * 
     * @param state Current response parsing state
     * @param atCmd The AT command being processed
     * @return true if command processing is complete, false to continue waiting for more lines
     */
    bool handleCompleteLine(ResponseState& state, const ATCommand_t& atCmd);

    /**
     * @brief Waits for '>' prompt when sending data commands.
     * 
     * @param timeout_ms Timeout in milliseconds
     * @return true if prompt received, false on timeout
     */
    bool waitForPrompt(int timeout_ms);

    /**
     * @brief Sends binary payload data and waits for confirmation.
     * 
     * @param payload Pointer to payload data
     * @param payload_len Length of payload
     * @return true if data sent and acknowledged, false otherwise
     */
    bool sendPayloadAndWaitResponse(const uint8_t* payload, size_t payload_len, int target_socket_id = -1);

    /**
     * @brief Ensures the command mutex is available.
     * @return true if mutex is available, false otherwise
     */
    bool ensureCmdMutex();
    
    /**
     * @brief Locks the command mutex to ensure exclusive access to the modem.
     * @return true if lock acquired, false on failure
     */
    bool lockCmd();

    /**
     * @brief Unlocks the command mutex after command processing is complete.
     * @return true if mutex released successfully, false otherwise
     */
    void unlockCmd();

    static SemaphoreHandle_t s_cmd_mutex;
};