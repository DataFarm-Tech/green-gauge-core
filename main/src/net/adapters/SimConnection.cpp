#include "SimConnection.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include "Types.hpp"
#include "ATCommandHndlr.hpp"
#include "Logger.hpp"
#include "CoapPktAssm.hpp"
#include "EEPROMConfig.hpp"

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
            // ADD: Small delay between INIT commands
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        if (init_success)
        {
            retry_counter = 0;
            // ADD: Allow modem to settle after INIT before checking status
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
                for (auto &cmd : hndlr.at_command_table)
                {
                    if (cmd.msg_type != MsgType::NETREG)
                        continue;
                    if (hndlr.send(cmd))
                    {
                        registered = true;
                        break;
                    }
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
                    if (retry_counter >= RETRIES)
                    {
                        sim_stat = SimStatus::ERROR;
                    }
                    else
                    {
                        vTaskDelay(pdMS_TO_TICKS(2000));
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

    // Step 2: Activate PDP context
    ATCommand_t activate_pdp = {
        "AT+QIACT=1",
        "OK",
        10000,
        MsgType::DATA,
        nullptr,
        0};

    if (!hndlr.send(activate_pdp))
    {
        printf("Failed to activate PDP context\n");
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

    for (auto &cmd : hndlr.at_command_table)
    {
        if (cmd.msg_type != MsgType::STATUS)
            continue;
        return hndlr.send(cmd);  // remove the sim_stat = ERROR side effect
    }
    return false;
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
    uint8_t coap_buffer[512];
    size_t coap_buffer_len = 0;
    char send_cmd[64];

    if (!cbor_buffer || cbor_buffer_len == 0)
    {
        printf("Invalid packet parameters\n");
        return false;
    }

    if (sim_stat != SimStatus::CONNECTED)
    {
        printf("Cannot send packet: not connected\n");
        return false;
    }

    printf("Building CoAP packet from CBOR payload (%zu bytes)\n", cbor_buffer_len);

    coap_buffer_len = CoapPktAssm::buildCoapBuffer(coap_buffer, cbor_buffer, cbor_buffer_len, pkt_config);

    if (coap_buffer_len == 0) {
        return false;
    }
    

    printf("CoAP packet built: %zu bytes total\n", coap_buffer_len);

    // Step 4: Send CoAP packet as UDP data
    snprintf(send_cmd, sizeof(send_cmd), "AT+QISEND=0,%zu", coap_buffer_len);

    ATCommand_t udp_send = {
        send_cmd,
        ">",
        5000,
        MsgType::DATA,
        coap_buffer,
        coap_buffer_len};

    if (!hndlr.send(udp_send))
    {
        printf("Failed to send UDP packet\n");
        closeUDPSocket();
        deactivatePDP();
        return false;
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

void SimConnection::closeCoAPSession()
{
    // Keep for compatibility, but use closeUDPSocket instead
    closeUDPSocket();
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