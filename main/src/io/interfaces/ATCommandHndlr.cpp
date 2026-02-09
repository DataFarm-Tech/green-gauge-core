#include "ATCommandHndlr.hpp"
#include "UARTDriver.hpp"
#include "Logger.hpp"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

bool ATCommandHndlr::send(const ATCommand_t& atCmd) {
    if (!atCmd.cmd || !atCmd.expect) {
        g_logger.error("Invalid AT command parameters\n");
        return false;
    }

    g_logger.info("TX: %s\n", atCmd.cmd);
    m_modem_uart.writef("%s\r\n", atCmd.cmd);

    // Check if this command includes a payload
    if (atCmd.payload != nullptr && atCmd.payload_len > 0) {
        // Wait for '>' prompt
        if (!waitForPrompt(atCmd.timeout_ms)) {
            g_logger.error("Timeout waiting for '>' prompt\n");
            return false;
        }

        // Send payload and wait for response
        return sendPayloadAndWaitResponse(atCmd.payload, atCmd.payload_len);
    } else {
        // Standard command - wait for expected response and OK
        ResponseState state = {};
        const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(atCmd.timeout_ms);

        while (xTaskGetTickCount() < deadline) {
            if (processResponse(state, atCmd)) {
                return state.success;
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        g_logger.error("AT TIMEOUT: %s (after %dms)\n", atCmd.cmd, atCmd.timeout_ms);
        return false;
    }
}

bool ATCommandHndlr::sendAndCapture(const ATCommand_t& atCmd, char* out_buf, size_t out_len) {
    if (!atCmd.cmd || !atCmd.expect || out_buf == nullptr || out_len == 0) {
        g_logger.error("Invalid parameters to sendAndCapture\n");
        return false;
    }

    g_logger.info("TX(capture): %s\n", atCmd.cmd);
    m_modem_uart.writef("%s\r\n", atCmd.cmd);

    ResponseState state = {};
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(atCmd.timeout_ms);

    while (xTaskGetTickCount() < deadline) {
        uint8_t c;
        if (!m_modem_uart.readByte(c)) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        if (c == '\n') {
            state.line_buffer[state.line_len] = '\0';
            // strip trailing CR
            if (state.line_len > 0 && state.line_buffer[state.line_len - 1] == '\r') {
                state.line_buffer[--state.line_len] = '\0';
            }

            if (state.line_len > 0) {
                g_logger.info("RX: %s\n", state.line_buffer);

                // Check for CME or generic ERROR
                if (strcmp(state.line_buffer, "ERROR") == 0 || strstr(state.line_buffer, "+CME ERROR") != nullptr) {
                    g_logger.info("AT response error (capture): %s\n", state.line_buffer);
                    return false;
                }

                // If the expected substring is present, capture and return success
                if (strstr(state.line_buffer, atCmd.expect) != nullptr) {
                    // Copy into out_buf
                    strncpy(out_buf, state.line_buffer, out_len - 1);
                    out_buf[out_len - 1] = '\0';
                    return true;
                }
            }

            state.line_len = 0;
        } else if (c != '\r') {
            if (state.line_len < sizeof(state.line_buffer) - 1) {
                state.line_buffer[state.line_len++] = c;
            } else {
                g_logger.warning("RX buffer overflow in sendAndCapture, discarding line\n");
                state.line_len = 0;
            }
        }
    }

    g_logger.error("AT TIMEOUT (capture): %s (after %dms)\n", atCmd.cmd, atCmd.timeout_ms);
    return false;
}

bool ATCommandHndlr::waitForPrompt(int timeout_ms) {
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    
    while (xTaskGetTickCount() < deadline) {
        uint8_t c;
        if (m_modem_uart.readByte(c)) {
            g_logger.info("Prompt char: 0x%02X\n", c);
            if (c == '>') {
                g_logger.info("Got '>' prompt\n");
                return true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    return false;
}
bool ATCommandHndlr::sendPayloadAndWaitResponse(const uint8_t* payload, size_t payload_len) {
    // Send binary payload
    for (size_t i = 0; i < payload_len; i++) {
        m_modem_uart.writeByte(payload[i]);
    }
    
    g_logger.info("Sent %zu bytes of payload\n", payload_len);
    
    // Small delay to ensure data is transmitted
    vTaskDelay(pdMS_TO_TICKS(100));

    // Wait for SEND OK or ERROR response
    ResponseState state = {};
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(10000);

    while (xTaskGetTickCount() < deadline) {
        uint8_t c;
        
        if (!m_modem_uart.readByte(c)) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        // Handle line termination
        if (c == '\n') {
            state.line_buffer[state.line_len] = '\0';
            if (state.line_len > 0 && state.line_buffer[state.line_len - 1] == '\r') {
                state.line_buffer[--state.line_len] = '\0';
            }

            if (state.line_len > 0) {
                g_logger.info("RX: %s\n", state.line_buffer);

                // Check for SEND OK (UDP/TCP socket send confirmation)
                if (strcmp(state.line_buffer, "SEND OK") == 0) {
                    g_logger.info("Payload sent successfully\n");
                    return true;
                }

                // Check for +QCOAPSEND response (CoAP-specific)
                if (strstr(state.line_buffer, "+QCOAPSEND:") != nullptr) {
                    // Check for success response codes (65 = 2.01 Created, 69 = 2.05 Content)
                    if (strstr(state.line_buffer, ",65") != nullptr || 
                        strstr(state.line_buffer, ",69") != nullptr) {
                        g_logger.info("CoAP server acknowledged packet\n");
                    }
                }

                // Check for OK (generic success)
                if (strcmp(state.line_buffer, "OK") == 0) {
                    g_logger.info("Payload send confirmed with OK\n");
                    return true;
                }

                // Check for SEND FAIL
                if (strcmp(state.line_buffer, "SEND FAIL") == 0) {
                    g_logger.error("Payload send failed\n");
                    return false;
                }

                // Check for ERROR
                if (strcmp(state.line_buffer, "ERROR") == 0) {
                    g_logger.error("Payload send error\n");
                    return false;
                }

                // Check for +QIURC: "closed" (connection closed)
                if (strstr(state.line_buffer, "+QIURC: \"closed\"") != nullptr) {
                    g_logger.warning("Socket closed during send\n");
                    return false;
                }
            }

            state.line_len = 0;
        } 
        else if (c != '\r') {
            if (state.line_len < sizeof(state.line_buffer) - 1) {
                state.line_buffer[state.line_len++] = c;
            } else {
                g_logger.warning("RX buffer overflow\n");
                state.line_len = 0;
            }
        }
    }

    g_logger.error("Timeout waiting for payload confirmation\n");
    return false;
}

bool ATCommandHndlr::processResponse(ResponseState& state, const ATCommand_t& atCmd) {
    uint8_t c;
    
    // Non-blocking read
    if (!m_modem_uart.readByte(c)) {
        return false;  // No data available
    }

    // Handle line termination
    if (c == '\n') {
        return handleCompleteLine(state, atCmd);
    }

    // Accumulate characters (ignore \r)
    if (c != '\r') {
        if (state.line_len < sizeof(state.line_buffer) - 1) {
            state.line_buffer[state.line_len++] = c;
        } else {
            g_logger.warning("RX buffer overflow, discarding line\n");
            state.line_len = 0;
        }
    }

    return false;  // Line not complete yet
}

bool ATCommandHndlr::handleCompleteLine(ResponseState& state, const ATCommand_t& atCmd) {
    // Null-terminate and remove trailing \r if present
    state.line_buffer[state.line_len] = '\0';
    if (state.line_len > 0 && state.line_buffer[state.line_len - 1] == '\r') {
        state.line_buffer[--state.line_len] = '\0';
    }

    // Skip empty lines
    if (state.line_len == 0) {
        return false;
    }

    g_logger.info("RX: %s\n", state.line_buffer);

    // Check for ERROR response (both "ERROR" and "+CME ERROR")
    if (strcmp(state.line_buffer, "ERROR") == 0 || strstr(state.line_buffer, "+CME ERROR") != nullptr) {
        g_logger.info("AT response error: %s (command: %s)\n", state.line_buffer, atCmd.cmd);
        state.success = false;
        state.line_len = 0;
        return true;  // Done processing
    }

    // Check for expected response
    if (strstr(state.line_buffer, atCmd.expect) != nullptr) {
        state.got_expected = true;
        g_logger.info("Got expected response: %s\n", atCmd.expect);
    }

    // Check for OK response
    if (strcmp(state.line_buffer, "OK") == 0) {
        if (state.got_expected || atCmd.msg_type == MsgType::SHUTDOWN) {
            g_logger.info("AT OK: %s\n", atCmd.cmd);
            state.success = true;
        } else {
            g_logger.warning("AT OK without expected response: %s (expected: %s)\n", 
                           atCmd.cmd, atCmd.expect);
            state.success = false;
        }
        state.line_len = 0;
        return true;  // Done processing
    }

    // Reset for next line
    state.line_len = 0;
    return false;  // Continue processing
}