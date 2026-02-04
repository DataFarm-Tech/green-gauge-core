#include "SimConnection.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include "Types.hpp"
#include "ATCommandHndlr.hpp"
#include "Logger.hpp"
#include "CoapPktAssm.hpp"

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

bool SimConnection::connect()
{
    g_logger.info("Starting Quectel Connection\n");
    uint8_t retry_counter = 0;

    while (sim_stat != SimStatus::CONNECTED)
    {
        switch (sim_stat)
        {
        case SimStatus::DISCONNECTED:
            g_logger.info("DISCONNECTED SIM STAT\n");
            sim_stat = SimStatus::INIT;
            break;

        case SimStatus::INIT:
            g_logger.info("INIT SIM STAT\n");
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
                }

                if (init_success)
                {
                    retry_counter = 0;
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
                        vTaskDelay(pdMS_TO_TICKS(2000));
                    }
                }
            }
            break;

        case SimStatus::NOTREADY:
            // printf("NOTREADY SIM STAT\n");
            g_logger.info("NOTREADY SIM STAT\n");

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
                        vTaskDelay(pdMS_TO_TICKS(2000));
                    }
                }
            }
            break;

        case SimStatus::REGISTERING:
            g_logger.info("REGISTERING SIM STAT\n");
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
                    g_logger.info("Device connected to network\n");
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
            g_logger.error("Failed to connect to network after %d retries\n", RETRIES);
            return false;

        case SimStatus::CONNECTED:
            // Should not reach here due to while condition, but included for completeness
            g_logger.info("CONNECTED SIM STAT\n");
            break;
        }
    }

    return true;
}

bool SimConnection::isConnected()
{

    if (sim_stat != SimStatus::CONNECTED)
    {
        return false;
    }

    for (auto &cmd : hndlr.at_command_table)
    {
        if (cmd.msg_type != MsgType::STATUS)
            continue;

        if (hndlr.send(cmd))
        {
            return true;
        }

        sim_stat = SimStatus::ERROR;
        return false;
    }

    return false;
}

void SimConnection::disconnect()
{

    g_logger.info("Disconnecting from network...\n");

    // Send shutdown commands
    for (auto &cmd : hndlr.at_command_table)
    {
        if (cmd.msg_type != MsgType::SHUTDOWN)
            continue;

        if (hndlr.send(cmd))
        {
            g_logger.info("Shutdown command successful\n");
        }
        else
        {
            g_logger.warning("Shutdown command failed\n");
        }
    }

    // Update state
    sim_stat = SimStatus::DISCONNECTED;

    g_logger.info("SIM disconnected\n");
}

bool SimConnection::sendPacket(const uint8_t * cbor_buffer, const size_t cbor_buffer_len, const PktType pkt_type)
{
    /**
     * Storing the complete COAP packet to be sent.
     * By the AT CMD Handlr.
     * 
     * ReadingArray -> CBOR data
     * CBOR placed into COAP pkt
     */
    uint8_t coap_buffer[512];

    if (!cbor_buffer || cbor_buffer_len == 0)
    {
        g_logger.error("Invalid packet parameters\n");
        return false;
    }

    if (sim_stat != SimStatus::CONNECTED)
    {
        g_logger.error("Cannot send packet: not connected\n");
        return false;
    }

    g_logger.info("Building CoAP packet from CBOR payload (%zu bytes)\n", cbor_buffer_len);

    size_t coap_buffer_len = CoapPktAssm::buildCoapBuffer(coap_buffer, pkt_type, cbor_buffer, cbor_buffer_len);

    g_logger.info("CoAP packet built: %zu bytes total\n", coap_buffer_len);

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
        g_logger.error("Failed to define PDP context\n");
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
        g_logger.error("Failed to activate PDP context\n");
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
        g_logger.error("Failed to open UDP socket\n");
        deactivatePDP();
        return false;
    }

    // Wait for QIOPEN URC
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Step 4: Send CoAP packet as UDP data
    char send_cmd[64];
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
        g_logger.error("Failed to send UDP packet\n");
        closeUDPSocket();
        deactivatePDP();
        return false;
    }

    // Wait for SEND OK
    vTaskDelay(pdMS_TO_TICKS(1000));

    g_logger.info("CoAP packet sent successfully via UDP\n");

    // Cleanup
    closeUDPSocket();
    deactivatePDP();

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
        g_logger.warning("Failed to close UDP socket\n");
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
        g_logger.warning("Failed to deactivate PDP context\n");
    }
}