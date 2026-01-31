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
    g_logger.info("Starting Quectel Connection\n");
    uint8_t retry_counter = 0;

    while (sim_stat != SimStatus::CONNECTED) {
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
                    for (auto& cmd : hndlr.at_command_table) {
                        if (cmd.msg_type != MsgType::INIT) continue;
                        if (!hndlr.send(cmd)) {
                            init_success = false;
                            break;
                        }
                    }

                    if (init_success) {
                        retry_counter = 0;
                        sim_stat = SimStatus::NOTREADY;
                    } else {
                        retry_counter++;
                        if (retry_counter >= RETRIES) {
                            sim_stat = SimStatus::ERROR;
                        } else {
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
                    for (auto& cmd : hndlr.at_command_table) {
                        if (cmd.msg_type != MsgType::STATUS) continue;
                        if (hndlr.send(cmd)) {
                            status_ok = true;
                            break;
                        }
                    }
                    
                    if (status_ok) {
                        retry_counter = 0;
                        sim_stat = SimStatus::REGISTERING;
                    } else {
                        retry_counter++;
                        if (retry_counter >= RETRIES) {
                            sim_stat = SimStatus::ERROR;
                        } else {
                            vTaskDelay(pdMS_TO_TICKS(2000));
                        }
                    }
                }
                break;
                
            case SimStatus::REGISTERING:
                g_logger.info("REGISTERING SIM STAT\n");
                {
                    bool registered = false;
                    for (auto& cmd : hndlr.at_command_table) {
                        if (cmd.msg_type != MsgType::NETREG) continue;
                        if (hndlr.send(cmd)) {
                            registered = true;
                            break;
                        }
                    }
                    
                    if (registered) {
                        retry_counter = 0;
                        sim_stat = SimStatus::CONNECTED;
                        // printf("CONNECTED SIM STAT\n");
                        // g_logger.info("CONNECTED SIM STAT\n");
                        g_logger.info("Device connected to network\n");
                    } else {
                        retry_counter++;
                        if (retry_counter >= RETRIES) {
                            sim_stat = SimStatus::ERROR;
                        } else {
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

    g_logger.info("Building CoAP packet from CBOR payload (%zu bytes)\n", pkt_len);

    // Build CoAP packet with URI path
    uint8_t coap_buffer[512];
    size_t coap_len = 0;
    
    // CoAP Header (4 bytes minimum)
    // Byte 0: Ver(2) | Type(2) | TKL(4)
    // Version 1, CON type (0), Token length 4
    coap_buffer[coap_len++] = 0x44;  // 01000100 = Ver:1, Type:CON, TKL:4
    
    // Byte 1: Code (0.02 = POST)
    coap_buffer[coap_len++] = 0x02;
    
    // Bytes 2-3: Message ID (random)
    static uint16_t msg_id = 0x1234;
    coap_buffer[coap_len++] = static_cast<uint8_t>((msg_id >> 8) & 0xFF);
    coap_buffer[coap_len++] = static_cast<uint8_t>(msg_id & 0xFF);
    msg_id++;
    
    // Token (4 bytes)
    coap_buffer[coap_len++] = 0x01;
    coap_buffer[coap_len++] = 0x02;
    coap_buffer[coap_len++] = 0x03;
    coap_buffer[coap_len++] = 0x04;
    
    // Option 11: Uri-Path = "activate" (delta=11, length=8)
    coap_buffer[coap_len++] = 0xB8;  // 10111000 = delta:11, length:8
    coap_buffer[coap_len++] = 'a';
    coap_buffer[coap_len++] = 'c';
    coap_buffer[coap_len++] = 't';
    coap_buffer[coap_len++] = 'i';
    coap_buffer[coap_len++] = 'v';
    coap_buffer[coap_len++] = 'a';
    coap_buffer[coap_len++] = 't';
    coap_buffer[coap_len++] = 'e';
    
    // Option 12: Content-Format = 60 (CBOR) (delta=1, length=1)
    coap_buffer[coap_len++] = 0x11;  // 00010001 = delta:1, length:1
    coap_buffer[coap_len++] = 60;
    
    // Payload marker
    coap_buffer[coap_len++] = 0xFF;
    
    // Copy CBOR payload
    if (coap_len + pkt_len > sizeof(coap_buffer)) {
        g_logger.error("CoAP packet too large\n");
        return false;
    }
    memcpy(&coap_buffer[coap_len], pkt, pkt_len);
    coap_len += pkt_len;
    
    g_logger.info("CoAP packet built: %zu bytes total\n", coap_len);

    // Step 1: Define PDP context
    ATCommand_t define_pdp = {
        "AT+QICSGP=1,1,\"telstra.internet\",\"\",\"\",1",
        "OK",
        2000,
        MsgType::DATA,
        nullptr,
        0
    };

    if (!hndlr.send(define_pdp)) {
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
        0
    };

    if (!hndlr.send(activate_pdp)) {
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
        0
    };

    if (!hndlr.send(open_socket)) {
        g_logger.error("Failed to open UDP socket\n");
        deactivatePDP();
        return false;
    }

    // Wait for QIOPEN URC
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Step 4: Send CoAP packet as UDP data
    char send_cmd[64];
    snprintf(send_cmd, sizeof(send_cmd), "AT+QISEND=0,%zu", coap_len);

    ATCommand_t udp_send = {
        send_cmd,
        ">",
        5000,
        MsgType::DATA,
        coap_buffer,
        coap_len
    };

    if (!hndlr.send(udp_send)) {
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

void SimConnection::closeUDPSocket() {
    ATCommand_t close_cmd = {
        "AT+QICLOSE=0",
        "OK",
        5000,
        MsgType::SHUTDOWN,
        nullptr,
        0
    };
    
    if (!hndlr.send(close_cmd)) {
        g_logger.warning("Failed to close UDP socket\n");
    }
}

void SimConnection::closeCoAPSession() {
    // Keep for compatibility, but use closeUDPSocket instead
    closeUDPSocket();
}

void SimConnection::deactivatePDP() {
    ATCommand_t deactivate_pdp = {
        "AT+QIDEACT=1",
        "OK",
        5000,
        MsgType::SHUTDOWN,
        nullptr,
        0
    };
    
    if (!hndlr.send(deactivate_pdp)) {
        g_logger.warning("Failed to deactivate PDP context\n");
    }
}