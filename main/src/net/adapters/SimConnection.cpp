#include "SimConnection.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include "Types.hpp"
#include "ATCommandHndlr.hpp"
#include "Logger.hpp"
#include "CoapPktAssm.hpp"
#include "EEPROMConfig.hpp"
#include <cstdio>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

bool SimConnection::connect()
{
    printf("Starting Quectel Connection\n");

    printf("%s\n", g_device_config.manf_info.sim_mod_sn.value);

    printf("%s\n", g_device_config.manf_info.sim_card_sn.value);

    ATCommand_t cfun_reset = {
        "AT+CFUN=1,1",
        "OK",
        5000,
        MsgType::INIT,
        nullptr,
        0
    };

    hndlr.send(cfun_reset);
    vTaskDelay(pdMS_TO_TICKS(8000));  // EC25 reboot time

    

    uint8_t retry_counter = 0;

    while (sim_stat != SimStatus::CONNECTED)
    {
        switch (sim_stat)
        {
        case SimStatus::DISCONNECTED:
            printf("DISCONNECTED SIM STAT\n");
            

            sim_stat = SimStatus::INIT;
            break;

        case SimStatus::INIT:
    printf("INIT SIM STAT\n");
    {
        bool init_success = true;
        for (auto &cmd : hndlr.at_command_table)
        {
            if (cmd.msg_type != MsgType::INIT)
                continue;
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
            if (retry_counter >= RETRIES)
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
        printf("NOTREADY SIM STAT\n");
        {
            bool status_ok = false;
            for (auto &cmd : hndlr.at_command_table)
            {
                if (cmd.msg_type != MsgType::STATUS)
                    continue;
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
                if (retry_counter >= RETRIES)
                {
                    sim_stat = SimStatus::ERROR;
                }
                else
                {
                    // Increase delay for STATUS retries (SIM might still be initializing)
                    printf("STATUS check failed, retry %d/%d\n", retry_counter, RETRIES);
                    vTaskDelay(pdMS_TO_TICKS(5000)); // Increased from 2000
                }
            }
        }
    break;

        case SimStatus::REGISTERING:
            printf("REGISTERING SIM STAT\n");
            {
                bool registered = false;

                ATCommand_t cereg_cmd = {
                    "AT+CEREG?",
                    "+CEREG:",
                    3000,
                    MsgType::NETREG,
                    nullptr,
                    0};

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
                    // printf("CONNECTED SIM STAT\n");
                    // g_logger.info("CONNECTED SIM STAT\n");
                    printf("Device connected to network\n");
                }
                else
                {
                    retry_counter++;
                    constexpr uint8_t REG_RETRIES = 20;
                    if (retry_counter >= REG_RETRIES)
                    {
                        sim_stat = SimStatus::ERROR;
                    }
                    else
                    {
                        vTaskDelay(pdMS_TO_TICKS(3000));
                    }
                }
            }
            break;

        case SimStatus::ERROR:
            printf("ERROR SIM STAT - Max retries exceeded\n");
            printf("Failed to connect to network after %d retries\n", RETRIES);
            return false;

        case SimStatus::CONNECTED:
            // Should not reach here due to while condition, but included for completeness
            printf("CONNECTED SIM STAT\n");
            break;
        }
    }

    // Step 1: Define PDP context
    ATCommand_t define_pdp = {
        "AT+QICSGP=1,1,\"telstra.internet\",\"\",\"\",1",
        "OK",
        2000,
        MsgType::DATA,
        nullptr,
        0};

    if (!hndlr.send(define_pdp))
    {
        printf("Failed to define PDP context\n");
        return false;
    }

    // Step 2: Activate PDP context (can fail transiently right after registration)
    ATCommand_t activate_pdp = {
        "AT+QIACT=1",
        "OK",
        10000,
        MsgType::DATA,
        nullptr,
        0};

    bool pdp_active = false;
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

    // Step 3: Open UDP socket for CoAP
    ATCommand_t open_socket = {
        "AT+QIOPEN=1,0,\"UDP\",\"45.79.239.100\",5683,0,0",
        "OK",
        5000,
        MsgType::DATA,
        nullptr,
        0};

    if (!hndlr.send(open_socket))
    {
        printf("Failed to open UDP socket\n");
        deactivatePDP();
        return false;
    }

    // Wait for QIOPEN URC
    vTaskDelay(pdMS_TO_TICKS(2000));


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

bool SimConnection::sendPacket(const uint8_t * cbor_buffer, const size_t cbor_buffer_len, const PktEntry_t pkt_config)
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

    if (!cbor_buffer || cbor_buffer_len == 0)
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

    // Wait for SEND OK
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("CoAP packet sent successfully via UDP\n");

    return true;
}

void SimConnection::closeUDPSocket()
{
    ATCommand_t close_cmd = {
        "AT+QICLOSE=0",
        "OK",
        5000,
        MsgType::SHUTDOWN,
        nullptr,
        0};

    if (!hndlr.send(close_cmd))
    {
        printf("Failed to close UDP socket\n");
    }
}

void SimConnection::deactivatePDP()
{
    ATCommand_t deactivate_pdp = {
        "AT+QIDEACT=1",
        "OK",
        5000,
        MsgType::SHUTDOWN,
        nullptr,
        0};

    if (!hndlr.send(deactivate_pdp))
    {
        printf("Failed to deactivate PDP context\n");
    }
}