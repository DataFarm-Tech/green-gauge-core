#include "SimConnection.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include "Types.hpp"
#include "ATCommandHndlr.hpp"
#include "Logger.hpp"
#include "CoapPktAssm.hpp"
#include "EEPROMConfig.hpp"
#include "Utils.hpp"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

/**
 * @brief CFUNCMD reset command
 * This command performs a full modem reset, which is necessary to recover from certain error states and ensure a clean start. The modem will reboot and reinitialize after this command, so it should be used with caution.
 * Expected response: "OK" after the modem restarts and is ready to accept commands again.
 * Timeout: 5000ms (to allow for modem reboot time)
 * MsgType: INIT (used during initial connection setup)
 */
ATCommand_t cfun_reset = {
    "AT+CFUN=1,1",
    "OK",
    5000,
    MsgType::INIT,
    nullptr,
    0
};

/**
 * @brief CEREG command to check network registration status
 * This command queries the modem for its current network registration status. The expected response includes "+C
 * EREG: <n>,<stat>" where <stat> indicates the registration status (e.g., 1 for registered, 5 for registered with roaming). This command is used to determine if the modem has successfully connected to the cellular network.
 * Timeout: 3000ms
 * MsgType: NETREG (used during network registration phase)
 */
ATCommand_t cereg_cmd = {
    "AT+CEREG?",
    "+CEREG:",
    3000,
    MsgType::NETREG,
    nullptr,
    0
};

/**
 * @brief QICSGP command to define PDP context
 * This command sets up the PDP context for the cellular connection, specifying the APN and other parameters. The expected response is "OK" if the context was defined successfully. This is a critical
 * step in preparing the modem for network communication, as it configures how the modem will connect to the cellular data network.
 * Timeout: 2000ms
 * MsgType: DATA (used during data connection setup)
 */
ATCommand_t define_pdp = {
    "AT+QICSGP=1,1,\"telstra.internet\",\"\",\"\",1",
    "OK",
    2000,
    MsgType::DATA,
    nullptr,
    0
};

/**
 * @brief QIMUX command to enable multi-socket mode
 * This command configures the modem to allow multiple simultaneous socket connections. The expected response is "OK" if the command was successful. Enabling multi-socket mode is important for applications that require
 * multiple concurrent network connections (e.g., one for telemetry data and another for remote management). This command should be sent before attempting to open any sockets.
 * Timeout: 3000ms
 * MsgType: DATA (used during data connection setup)
 */
ATCommand_t enable_multi_socket = {
    "AT+QIMUX=1",
    "OK",
    3000,
    MsgType::DATA,
    nullptr,
    0
};

/**
 * @brief QIACT command to activate PDP context
 * This command activates the previously defined PDP context, allowing the modem to establish a data connection with the cellular network. The expected response is "OK" if the PDP context was activated successfully. Activ
 * ating the PDP context is a crucial step in the connection process, as it enables the modem to obtain an IP address and start sending/receiving data over the cellular network. This command may need to be retried if the modem is not yet ready to activate the context.
 * Timeout: 10000ms (activation can take some time, especially on initial attempts)
 * MsgType: DATA (used during data connection setup)
 */
ATCommand_t activate_pdp = {
    "AT+QIACT=1",
    "OK",
    10000,
    MsgType::DATA,
    nullptr,
    0
};

/**
 * @brief QIACT command to check PDP context status
 * This command queries the modem for the status of the PDP context, including whether it is active and the assigned IP address. The expected response includes "+QIACT: 1,1"
 * indicating that the context is active. This command is used to verify that the PDP context was successfully activated and that the modem is ready for network communication.
 * Timeout: 3000ms
 * MsgType: DATA (used during data connection setup)
 */
ATCommand_t check_ip = {
    "AT+QIACT?",
    "+QIACT:",
    3000,
    MsgType::DATA,
    nullptr,
    0
};

/**
 * @brief QIOPEN command to open a UDP socket
 * This command instructs the modem to open a UDP socket connection to a specified remote host and port. The expected response is "OK" followed by an asynchronous "+QIOPEN: <
 * id>,<err>" response indicating whether the socket was opened successfully (err=0) or if there was an error. This command is essential for establishing a communication channel over which data packets can be sent and received.
 * Timeout: 5000ms (to allow for socket opening process)
 * MsgType: DATA (used during data connection setup)
 */
ATCommand_t open_socket = {
    "AT+QIOPEN=1,0,\"UDP\",\"45.79.118.187\",5683,0,0",
    "OK",
    5000,
    MsgType::DATA,
    nullptr,
    0
};

/**
 * @brief QIDEACT command to deactivate PDP context
 * This command deactivates the active PDP context, effectively disconnecting the modem from the cellular network. The expected response is "OK" if the context was deactivated successfully. This command is
 * used for cleanup and error recovery purposes, allowing the modem to reset its network state before retrying activation or shutting down.
 * Timeout: 5000ms
 * MsgType: SHUTDOWN (used during connection shutdown and error recovery)
 */
ATCommand_t deactivate_pdp = {
    "AT+QIDEACT=1",
    "OK",
    5000,
    MsgType::SHUTDOWN,
    nullptr,
    0
};

/**
 * @brief QICLOSE command to close the UDP socket
 * This command instructs the modem to close the previously opened UDP socket. The expected response is "OK" if the socket was closed successfully. This command is used during cleanup and shutdown procedures to ensure that all network resources are properly released.
 * Timeout: 5000ms
 * MsgType: SHUTDOWN (used during connection shutdown and error recovery)
 */
ATCommand_t close_cmd = {
    "AT+QICLOSE=0",
    "OK",
    5000,
    MsgType::SHUTDOWN,
    nullptr,
    0
};

/**
 * @brief QSSLCLOSE command to close the SSL socket
 * This command instructs the modem to close an SSL/TLS socket that was previously opened with QSSLOPEN. The expected response is "OK" if the socket was closed successfully.
 * This command is used during cleanup and shutdown procedures to ensure that all network resources are properly released, especially in cases where an SSL/TLS connection was established for secure communication.
 * Timeout: 7000ms (SSL/TLS sockets may require additional time to close gracefully
 * MsgType: SHUTDOWN (used during connection shutdown and error recovery)
 */
ATCommand_t close_ssl_socket = {
        "AT+QSSLCLOSE=1",
        "OK",
        7000,
        MsgType::DATA,
        nullptr,
        0
};

/**
 * @brief QICLOSE command to close the UDP socket (alternative for SSL connections)
 * This command is used as an alternative to close_cmd for cases where the connection was established using
 * QSSLOPEN, which may require using QICLOSE with the SSL socket ID instead of QSSLCLOSE. The expected response is "OK" if the socket was closed successfully. This command provides flexibility in handling different types of connections during shutdown procedures.
 * Timeout: 5000ms
 * MsgType: SHUTDOWN (used during connection shutdown and error recovery)
 */
ATCommand_t close_tls_socket = {
    "AT+QICLOSE=1",
    "OK",
    5000,
    MsgType::DATA,
    nullptr,
    0
};



bool SimConnection::estDataSession() 
{
    bool pdp_active = false;

    if (!hndlr.send(define_pdp))
    {
        printf("Failed to define PDP context\n");
        return false;
    }

    if (!hndlr.send(enable_multi_socket))
    {
        printf("Failed to enable multi-socket mode (QIMUX=1)\n");
        return false;
    }

    for (int attempt = 1; attempt <= 3; ++attempt)
    {
        if (hndlr.send(activate_pdp))
        {
            pdp_active = true;
            break;
        }

        printf("PDP activate failed (attempt %d/3)\n", attempt);

        // Reset context and retry after a short backoff.
        deactivatePDP();
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    if (!pdp_active)
    {
        printf("Failed to activate PDP context after retries\n");
        return false;
    }

    // After PDP activation success
    vTaskDelay(pdMS_TO_TICKS(4000));   // REQUIRED for EC25 stability

    if (!hndlr.send(check_ip)) {
        printf("PDP context not fully active\n");
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(5000));   // REQUIRED for EC25 stability

    if (!hndlr.send(open_socket))
    {
        printf("Failed to open UDP socket\n");
        deactivatePDP();
        return false;
    }

    return true;
}

bool SimConnection::connect()
{
    uint8_t retry_counter = 0;
    uint8_t retry_limit = static_cast<uint8_t>(RETRIES);
    const char *retry_phase = "initialization";
    
    #if 0
    printf("Starting Quectel Connection\n");
    printf("%s\n", g_device_config.manf_info.sim_mod_sn.value);
    printf("%s\n", g_device_config.manf_info.sim_card_sn.value);
    #endif

    hndlr.send(cfun_reset);
    vTaskDelay(pdMS_TO_TICKS(8000));  // EC25 reboot time

    while (sim_stat != SimStatus::CONNECTED)
    {
        switch (sim_stat)
        {
        case SimStatus::DISCONNECTED:
            sim_stat = SimStatus::INIT;
            break;

        case SimStatus::INIT:
        {
            retry_limit = static_cast<uint8_t>(RETRIES);
            retry_phase = "modem initialization";
            bool init_success = true;
            for (auto &cmd : hndlr.at_command_table)
            {
                if (cmd.msg_type != MsgType::INIT) continue;
                if (!hndlr.send(cmd))
                {
                    init_success = false;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            if (init_success)
            {
                retry_counter = 0;
                printf("INIT complete, waiting for modem to settle...\n");
                vTaskDelay(pdMS_TO_TICKS(3000)); // 3 second settling time
                sim_stat = SimStatus::NOTREADY;
            }
            else
            {
                retry_counter++;
                if (retry_counter >= retry_limit)
                {
                    sim_stat = SimStatus::ERROR;
                }
                else
                {
                    vTaskDelay(pdMS_TO_TICKS(5000)); // Increase retry delay for INIT
                }
            }
        }
        break;

        case SimStatus::NOTREADY:
        {
            retry_limit = static_cast<uint8_t>(RETRIES);
            retry_phase = "SIM ready check";
            bool status_ok = false;
            for (auto &cmd : hndlr.at_command_table)
            {
                if (cmd.msg_type != MsgType::STATUS) continue;
                if (hndlr.send(cmd))
                {
                    status_ok = true;
                    break;
                }
            }

            if (status_ok)
            {
                retry_counter = 0;
                sim_stat = SimStatus::REGISTERING;
            }
            else
            {
                retry_counter++;
                if (retry_counter >= retry_limit)
                {
                    sim_stat = SimStatus::ERROR;
                }
                else
                {
                    printf("STATUS check failed, retry %d/%d\n", retry_counter, retry_limit);
                    vTaskDelay(pdMS_TO_TICKS(5000));
                }
            }
        }
        break;

        case SimStatus::REGISTERING:
        {
            retry_limit = static_cast<uint8_t>(REG_RETRIES);
            retry_phase = "network registration";
            bool registered = false;

            char cereg_resp[128] = {0};
            if (hndlr.sendAndCapture(cereg_cmd, cereg_resp, sizeof(cereg_resp)))
            {
                int n = -1;
                int stat = -1;

                if (sscanf(cereg_resp, "+CEREG: %d,%d", &n, &stat) == 2)
                {
                    if (stat == 1 || stat == 5)
                    {
                        registered = true;
                    }
                    else
                    {
                        printf("CEREG not registered yet (stat=%d)\n", stat);
                    }
                }
                else
                {
                    printf("Unable to parse CEREG response: %s\n", cereg_resp);
                }
            }
            else
            {
                printf("Failed to query CEREG state\n");
            }

            if (registered)
            {
                retry_counter = 0;
                sim_stat = SimStatus::CONNECTED;
                printf("Device connected to network\n");
            }
            else
            {
                retry_counter++;
                if (retry_counter >= retry_limit)
                {
                    sim_stat = SimStatus::ERROR;
                }
                else
                {
                    printf("CEREG not ready, retry %d/%d\n", retry_counter, retry_limit);
                    vTaskDelay(pdMS_TO_TICKS(3000));
                }
            }
        }
        break;

        case SimStatus::ERROR:
            printf("ERROR SIM STAT - Max retries exceeded\n");
            printf("Failed during %s after %d/%d retries\n", retry_phase, retry_counter, retry_limit);
            return false;

        case SimStatus::CONNECTED:
            // Should not reach here due to while condition, but included for completeness
            printf("CONNECTED SIM STAT\n");
            break;
        }
    }

    if (!estDataSession())
    {
        return false;
    }

    return true;
}

bool SimConnection::isConnected()
{
    if (sim_stat != SimStatus::CONNECTED)
        return false;

    bool ran_hard_status_check = false;
    for (auto &cmd : hndlr.at_command_table)
    {
        if (cmd.msg_type != MsgType::STATUS)
            continue;

        const bool is_ping_check = (cmd.cmd != nullptr) && (strstr(cmd.cmd, "AT+QPING") != nullptr);

        if (!hndlr.send(cmd))
        {
            if (is_ping_check)
            {
                // ICMP may be blocked by carrier/APN; do not mark link down solely on ping failure.
                printf("STATUS warning: ping check failed, keeping session as connected\n");
                continue;
            }

            return false;
        }

        if (!is_ping_check)
        {
            ran_hard_status_check = true;
        }
    }

    return ran_hard_status_check;
}

void SimConnection::disconnect()
{
    printf("Disconnecting SIM connection\n");

    telnet_session.stop();

    // Close UDP socket first
    closeUDPSocket();
    
    // Deactivate PDP context
    deactivatePDP();

    // Send any remaining shutdown commands
    for (auto &cmd : hndlr.at_command_table)
    {
        if (cmd.msg_type != MsgType::SHUTDOWN)
            continue;

        if (hndlr.send(cmd))
        {
            printf("Shutdown command successful\n");
        }
        else
        {
            printf("Shutdown command failed\n");
        }
    }

    // Update state
    sim_stat = SimStatus::DISCONNECTED;

    printf("SIM disconnected\n");
}

bool SimConnection::sendPacket(const uint8_t * cbor_buffer,
                               const size_t cbor_buffer_len,
                               const PktEntry_t pkt_config,
                               std::string* response)
{
    /**
     * Storing the complete COAP packet to be sent.
     * By the AT CMD Handlr.
     * 
     * ReadingArray -> CBOR data
     * CBOR placed into COAP pkt
     */
    uint8_t coap_buffer[GEN_BUFFER_SIZE + 64];
    size_t coap_buffer_len = 0;
    char send_cmd[64];

    if (cbor_buffer_len > 0 && !cbor_buffer)
    {
        printf("Invalid packet parameters\n");
        return false;
    }

    if (response)
    {
        response->clear();
        return sendPacketStream(
            cbor_buffer,
            cbor_buffer_len,
            pkt_config,
            [response](const uint8_t* chunk, size_t chunk_len) -> bool {
                if (!chunk || chunk_len == 0)
                {
                    return true;
                }

                response->append(reinterpret_cast<const char*>(chunk), chunk_len);
                return true;
            });
    }

    auto ensure_connected = [this]() -> bool {
        if (sim_stat == SimStatus::CONNECTED) {
            return true;
        }

        printf("SIM not connected, attempting reconnect before send\n");
        return connect();
    };

    if (!ensure_connected())
    {
        printf("Cannot send packet: reconnect failed\n");
        return false;
    }

    coap_buffer_len = CoapPktAssm::buildCoapBuffer(coap_buffer, cbor_buffer, cbor_buffer_len, pkt_config);

    if (coap_buffer_len == 0) {
        return false;
    }
    

    auto send_udp_packet = [&]() -> bool {
        snprintf(send_cmd, sizeof(send_cmd), "AT+QISEND=0,%zu", coap_buffer_len);

        ATCommand_t udp_send = {
            send_cmd,
            ">",
            5000,
            MsgType::DATA,
            coap_buffer,
            coap_buffer_len};

        return hndlr.send(udp_send);
    };

    if (!send_udp_packet())
    {
        printf("Failed to send UDP packet, resetting SIM data link\n");
        disconnect();

        if (!ensure_connected())
        {
            printf("SIM reconnect failed after send failure\n");
            return false;
        }

        if (!send_udp_packet())
        {
            printf("Failed to send UDP packet after reconnect\n");
            disconnect();
            return false;
        }
    }

    // Wait for SEND OK
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("CoAP packet sent successfully via UDP\n");
    return true;
}

bool SimConnection::sendPacketStream(const uint8_t * cbor_buffer,
                                     const size_t cbor_buffer_len,
                                     const PktEntry_t pkt_config,
                                     const PacketChunkCallback& onChunk)
{
    uint8_t coap_buffer[GEN_BUFFER_SIZE + 64];
    size_t coap_buffer_len = 0;
    char send_cmd[64];

    if (cbor_buffer_len > 0 && !cbor_buffer)
    {
        printf("Invalid packet parameters\n");
        return false;
    }

    auto ensure_connected = [this]() -> bool {
        if (sim_stat == SimStatus::CONNECTED) {
            return true;
        }

        printf("SIM not connected, attempting reconnect before send\n");
        return connect();
    };

    if (!ensure_connected())
    {
        printf("Cannot send packet: reconnect failed\n");
        return false;
    }

    coap_buffer_len = CoapPktAssm::buildCoapBuffer(coap_buffer, cbor_buffer, cbor_buffer_len, pkt_config);

    if (coap_buffer_len == 0) {
        return false;
    }

    auto send_udp_packet = [&]() -> bool {
        snprintf(send_cmd, sizeof(send_cmd), "AT+QISEND=0,%zu", coap_buffer_len);

        ATCommand_t udp_send = {
            send_cmd,
            ">",
            5000,
            MsgType::DATA,
            coap_buffer,
            coap_buffer_len};

        return hndlr.send(udp_send);
    };

    if (!send_udp_packet())
    {
        printf("Failed to send UDP packet, resetting SIM data link\n");
        disconnect();

        if (!ensure_connected())
        {
            printf("SIM reconnect failed after send failure\n");
            return false;
        }

        if (!send_udp_packet())
        {
            printf("Failed to send UDP packet after reconnect\n");
            disconnect();
            return false;
        }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));

    if (!onChunk)
    {
        printf("CoAP packet sent successfully via UDP\n");
        return true;
    }

    char rx_buffer[GEN_BUFFER_SIZE + 64] = {0};
    size_t rx_read = 0;
    int empty_reads = 0;
    bool got_any_payload = false;
    static constexpr int EMPTY_READS_TO_FINISH = 5;
    const int response_window_ms = (pkt_config.response_win > 0)
                                       ? pkt_config.response_win
                                       : PKT_RESPONSE_WIN_DEFAULT_MS;
    const int read_timeout_ms = (pkt_config.socket_read_timeout > 0)
                                    ? pkt_config.socket_read_timeout
                                    : PKT_SOCKET_READ_TIMEOUT_DEFAULT_MS;
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(response_window_ms);

    auto has_time_remaining = [deadline]() -> bool {
        return static_cast<int32_t>(deadline - xTaskGetTickCount()) > 0;
    };

    while (has_time_remaining())
    {
        if (!hndlr.readSocketData(0, rx_buffer, sizeof(rx_buffer), &rx_read, read_timeout_ms, sizeof(rx_buffer) - 1))
        {
            vTaskDelay(pdMS_TO_TICKS(150));
            continue;
        }

        if (rx_read == 0)
        {
            empty_reads++;
            if (got_any_payload && empty_reads >= EMPTY_READS_TO_FINISH)
            {
                printf("CoAP packet sent successfully via UDP\n");
                return true;
            }

            vTaskDelay(pdMS_TO_TICKS(120));
            continue;
        }

        empty_reads = 0;

        std::string payload_chunk;
        if (Utils::extractCoapPayloadChunk(rx_buffer, rx_read, payload_chunk) > 0)
        {
            if (!onChunk(reinterpret_cast<const uint8_t*>(payload_chunk.data()), payload_chunk.size()))
            {
                printf("Streaming callback aborted firmware receive\n");
                return false;
            }

            got_any_payload = true;
        }

        vTaskDelay(pdMS_TO_TICKS(80));
    }

    if (got_any_payload)
    {
        printf("CoAP packet sent successfully via UDP\n");
        return true;
    }

    printf("Timed out waiting for response payload (%d ms)\n", response_window_ms);
    return false;
}

bool SimConnection::streamHttpsGet(const std::string& url, const PacketChunkCallback& onChunk)
{
    if (!onChunk)
    {
        return false;
    }

    auto ensure_connected = [this]() -> bool {
        if (sim_stat == SimStatus::CONNECTED)
        {
            return true;
        }

        printf("SIM not connected, attempting reconnect before HTTPS stream\n");
        return connect();
    };

    if (!ensure_connected())
    {
        return false;
    }

    // Prefer modem HTTP stack for HTTPS downloads on this modem/firmware family.
    if (hndlr.httpGetStream(url, onChunk, 180))
    {
        return true;
    }

    printf("Primary modem HTTP stack download failed, trying legacy SSL socket path\n");

    std::string host;
    std::string path;
    uint16_t port = 443;
    if (!Utils::parseHttpsUrl(url, host, port, path))
    {
        printf("Invalid HTTPS URL for SIM stream: %s\n", url.c_str());
        return false;
    }

    auto close_https_socket = [this]() {
        (void)hndlr.send(close_ssl_socket);
        (void)hndlr.send(close_tls_socket);
    };

    // Best effort cleanup in case socket id 1 was left open by a previous attempt.
    close_https_socket();

    if (!hndlr.openSSLSocket(host.c_str(), port, 1, 1, 45000))
    {
        printf("Failed to open SSL socket to %s:%u\n", host.c_str(), static_cast<unsigned>(port));
        return false;
    }

    std::string request = Utils::buildHttpsGetRequest(host, path);

    if (!hndlr.sendSocketData(1,
                              reinterpret_cast<const uint8_t*>(request.data()),
                              request.size(),
                              10000))
    {
        printf("Failed to send HTTPS GET request\n");
        close_https_socket();
        return false;
    }

    bool headers_done = false;
    bool chunked = false;
    bool got_body = false;
    bool chunked_done = false;
    size_t expected_chunk_bytes = 0;
    int64_t content_length = -1;
    int64_t body_received = 0;
    std::string pending;
    int idle_reads = 0;
    const int max_idle_reads = 20;

    auto emit_payload = [&onChunk, &got_body, &body_received](const char* data, size_t len) -> bool {
        if (len == 0)
        {
            return true;
        }

        if (!onChunk(reinterpret_cast<const uint8_t*>(data), len))
        {
            return false;
        }

        got_body = true;
        body_received += static_cast<int64_t>(len);
        return true;
    };

    auto process_chunked_payload = [&]() -> bool {
        while (true)
        {
            if (chunked_done)
            {
                return true;
            }

            if (expected_chunk_bytes == 0)
            {
                const size_t line_end = pending.find("\r\n");
                if (line_end == std::string::npos)
                {
                    return true;
                }

                std::string size_text = pending.substr(0, line_end);
                const size_t ext_pos = size_text.find(';');
                if (ext_pos != std::string::npos)
                {
                    size_text = size_text.substr(0, ext_pos);
                }

                char* end_ptr = nullptr;
                const unsigned long parsed = std::strtoul(size_text.c_str(), &end_ptr, 16);
                if (end_ptr == nullptr || *end_ptr != '\0')
                {
                    printf("Invalid chunk size in HTTPS response\n");
                    return false;
                }

                pending.erase(0, line_end + 2);
                expected_chunk_bytes = static_cast<size_t>(parsed);

                if (expected_chunk_bytes == 0)
                {
                    chunked_done = true;
                    return true;
                }
            }

            if (pending.size() < expected_chunk_bytes + 2)
            {
                return true;
            }

            if (!emit_payload(pending.data(), expected_chunk_bytes))
            {
                return false;
            }

            pending.erase(0, expected_chunk_bytes + 2);
            expected_chunk_bytes = 0;
        }
    };

    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(300000);
    while (static_cast<int32_t>(deadline - xTaskGetTickCount()) > 0)
    {
        char rx_buffer[1024] = {0};
        size_t rx_read = 0;

        if (!hndlr.readSocketData(1, rx_buffer, sizeof(rx_buffer), &rx_read, 2000, sizeof(rx_buffer) - 1))
        {
            idle_reads++;
            if (got_body && idle_reads >= max_idle_reads)
            {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (rx_read == 0)
        {
            idle_reads++;
            if ((chunked_done || (content_length >= 0 && body_received >= content_length)) ||
                (got_body && idle_reads >= max_idle_reads))
            {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        idle_reads = 0;
        pending.append(rx_buffer, rx_read);

        if (!headers_done)
        {
            const size_t header_end = pending.find("\r\n\r\n");
            if (header_end == std::string::npos)
            {
                continue;
            }

            const std::string headers = pending.substr(0, header_end);
            headers_done = true;

            const size_t status_line_end = headers.find("\r\n");
            const std::string status_line = (status_line_end == std::string::npos)
                                                ? headers
                                                : headers.substr(0, status_line_end);
            if (status_line.find(" 200 ") == std::string::npos &&
                status_line.find(" 206 ") == std::string::npos)
            {
                printf("HTTPS GET failed, status line: %s\n", status_line.c_str());
                close_https_socket();
                return false;
            }

            const std::string headers_lower = Utils::toLowerAscii(headers);
            const std::string chunked_key = "transfer-encoding: chunked";
            chunked = headers_lower.find(chunked_key) != std::string::npos;

            const std::string length_key = "content-length:";
            const size_t length_pos = headers_lower.find(length_key);
            if (length_pos != std::string::npos)
            {
                size_t value_start = length_pos + length_key.size();
                while (value_start < headers_lower.size() &&
                       std::isspace(static_cast<unsigned char>(headers_lower[value_start])))
                {
                    value_start++;
                }

                size_t value_end = value_start;
                while (value_end < headers_lower.size() && std::isdigit(static_cast<unsigned char>(headers_lower[value_end])))
                {
                    value_end++;
                }

                if (value_end > value_start)
                {
                    content_length = static_cast<int64_t>(std::strtoll(headers_lower.substr(value_start, value_end - value_start).c_str(), nullptr, 10));
                }
            }

            pending.erase(0, header_end + 4);
        }

        if (chunked)
        {
            if (!process_chunked_payload())
            {
                close_https_socket();
                return false;
            }

            if (chunked_done)
            {
                break;
            }
        }
        else
        {
            if (!pending.empty())
            {
                if (!emit_payload(pending.data(), pending.size()))
                {
                    close_https_socket();
                    return false;
                }
                pending.clear();
            }

            if (content_length >= 0 && body_received >= content_length)
            {
                break;
            }
        }
    }

    close_https_socket();

    if (chunked && !chunked_done)
    {
        printf("HTTPS chunked transfer did not complete\n");
        return false;
    }

    if (content_length >= 0 && body_received < content_length)
    {
        printf("HTTPS body truncated: received %lld of %lld bytes\n",
               static_cast<long long>(body_received),
               static_cast<long long>(content_length));
        return false;
    }

    return got_body;
}

bool SimConnection::startTelnetSession()
{
    if (sim_stat != SimStatus::CONNECTED)
    {
        return false;
    }

    if (telnet_session.isRunning())
    {
        return true;
    }

    return telnet_session.start(hndlr);
}

void SimConnection::closeUDPSocket()
{
    if (!hndlr.send(close_cmd))
    {
        printf("Failed to close UDP socket\n");
    }
}

void SimConnection::deactivatePDP()
{
    if (!hndlr.send(deactivate_pdp))
    {
        printf("Failed to deactivate PDP context\n");
    }
}