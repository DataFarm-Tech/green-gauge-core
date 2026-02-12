#pragma once

#include <unistd.h>

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
    bool sendPayloadAndWaitResponse(const uint8_t* payload, size_t payload_len);
};