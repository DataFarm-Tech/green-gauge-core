#include "SimConnection.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include "Types.hpp"
#include "ATCommandHndlr.hpp"
#include "Logger.hpp"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

bool SimConnection::connect() {
    uint8_t retry_counter = 0;

    while (sim_stat != SimStatus::CONNECTED) {
        switch (sim_stat)
        {
            case SimStatus::DISCONNECTED:
                g_logger.info("Starting Quectel EC25 Wakeup\n");    
                sim_stat = SimStatus::INIT;
                break;
            case SimStatus::INIT:
                g_logger.info("Initialising Quectel EC25 Module\n");    
                for (auto& cmd : hndlr.at_command_table) {
                    if (cmd.msg_type != MsgType::INIT) continue;
                    if (!hndlr.send(cmd)) {
                        sim_stat = SimStatus::ERROR;
                        break;
                    }
                }

                if (sim_stat != SimStatus::ERROR) {
                    sim_stat = SimStatus::NOTREADY;
                }
                break;
            
            case SimStatus::NOTREADY:
                for (auto& cmd : hndlr.at_command_table) {
                    if (cmd.msg_type != MsgType::STATUS) continue;
                    if (hndlr.send(cmd)) {
                        sim_stat = SimStatus::REGISTERING;
                        break;
                    }
                }
                break;
            case SimStatus::REGISTERING:
                for (auto& cmd : hndlr.at_command_table) {
                    if (cmd.msg_type != MsgType::NETREG) continue;
                    if (hndlr.send(cmd)) {
                        retry_counter = 0;
                        sim_stat = SimStatus::CONNECTED;
                        break;
                    }
                }
                break;
            case SimStatus::ERROR:
                if (retry_counter >= RETRIES) {
                    return false;
                }
                
                retry_counter++;

                sim_stat = SimStatus::INIT;

                vTaskDelay(pdMS_TO_TICKS(5000));
                break;
            case SimStatus::CONNECTED:
                break;
        }
    }
    

    return true;
}

bool SimConnection::isConnected() {
    
    if (sim_stat != SimStatus::CONNECTED) {
        return false;
    }

    for (auto& cmd : hndlr.at_command_table) {
        if (cmd.msg_type != MsgType::STATUS) continue;
        
        if (hndlr.send(cmd)) {
            return true;
        }
        
        sim_stat = SimStatus::ERROR;
        return false;
    }
    
    return false;
}

void SimConnection::disconnect() {
    
    g_logger.info("Disconnecting from network...\n");
    
    // Send shutdown commands
    for (auto& cmd : hndlr.at_command_table) {
        if (cmd.msg_type != MsgType::SHUTDOWN) continue;
        
        if (hndlr.send(cmd)) {
            g_logger.info("Shutdown command successful\n");
        } else {
            g_logger.warning("Shutdown command failed\n");
        }
    }
    
    // Update state
    sim_stat = SimStatus::DISCONNECTED;
    
    g_logger.info("SIM disconnected\n");
}

bool SimConnection::sendPacket(const uint8_t * pkt, const size_t pkt_len) {
    if (!pkt || pkt_len == 0) {
        g_logger.error("Invalid packet parameters\n");
        return false;
    }

    if (sim_stat != SimStatus::CONNECTED) {
        g_logger.error("Cannot send packet: not connected\n");
        return false;
    }

    g_logger.info("Sending CoAP packet (%zu bytes)\n", pkt_len);

    // Step 1: Configure CoAP context
    ATCommand_t config_cmd = {
        "AT+QCOAPCFG=\"contextid\",0",
        "OK",
        2000,
        MsgType::DATA,
        nullptr,
        0
    };

    if (!hndlr.send(config_cmd)) {
        g_logger.error("Failed to configure CoAP context\n");
        return false;
    }

    // Step 2: Open CoAP session
    ATCommand_t coap_open = {
        "AT+QCOAPOPEN=0,\"45.79.239.100\",5683",
        "OK",
        5000,
        MsgType::DATA,
        nullptr,
        0
    };

    if (!hndlr.send(coap_open)) {
        g_logger.error("Failed to open CoAP connection\n");
        return false;
    }

    // Step 3: Add Content-Format header for CBOR
    ATCommand_t coap_addheader = {
        "AT+QCOAPADDHEADER=0,12,\"60\"",
        "OK",
        2000,
        MsgType::DATA,
        nullptr,
        0
    };

    if (!hndlr.send(coap_addheader)) {
        g_logger.warning("Failed to add CoAP header\n");
    }

    // Step 4: Send QCOAPSEND command with payload
    char send_cmd[128];
    snprintf(send_cmd, sizeof(send_cmd), 
             "AT+QCOAPSEND=0,1,0,2,\"/api/sensor\",%zu", pkt_len);

    ATCommand_t coap_send = {
        send_cmd,
        ">",
        10000,
        MsgType::DATA,
        pkt,         // Payload pointer
        pkt_len      // Payload length
    };

    if (!hndlr.send(coap_send)) {
        g_logger.error("Failed to send CoAP packet\n");
        closeCoAPSession();
        return false;
    }

    g_logger.info("CoAP packet sent successfully\n");
    closeCoAPSession();
    return true;
}

void SimConnection::closeCoAPSession() {
    ATCommand_t close_cmd = {
        "AT+QCOAPCLOSE=0",
        "OK",
        5000,
        MsgType::SHUTDOWN,
        nullptr,
        0
    };
    
    if (!hndlr.send(close_cmd)) {
        g_logger.warning("Failed to close CoAP session\n");
    }
}