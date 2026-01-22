#include "SimConnection.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include "Types.hpp"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

static const char* TAG = "SIM";

bool SimConnection::sendAT(const ATCommand_t& atCmd) {
    char line[256];   // larger buffer for long responses
    size_t len = 0;
    bool got_expect = false;

    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(atCmd.timeout_ms);

    m_modem_uart.writef("%s\r\n", atCmd.cmd);

    while (xTaskGetTickCount() < deadline) {
        uint8_t c;

        // Fully non-blocking read
        if (!m_modem_uart.readByte(c)) {
            // No data available; yield a bit
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        // Accumulate line
        if (c == '\n') {
            line[len] = '\0';
            if (len > 0 && line[len - 1] == '\r') line[len - 1] = '\0';
            ESP_LOGD(TAG, "RX: %s", line);

            if (strcmp(line, "ERROR") == 0) {
                ESP_LOGE(TAG, "AT ERROR (%s)", atCmd.cmd);
                return false;
            }

            if (strstr(line, atCmd.expect)) got_expect = true;

            if (strcmp(line, "OK") == 0) {
                if (got_expect || atCmd.msg_type == MsgType::SHUTDOWN) {
                    ESP_LOGI(TAG, "AT OK: %s", atCmd.cmd);
                    return true;
                } else {
                    ESP_LOGW(TAG, "AT OK without expected response (%s)", atCmd.cmd);
                    return false;
                }
            }

            len = 0;
            continue;
        }

        // Normal character
        if (c != '\r') {
            if (len < sizeof(line) - 1) {
                line[len++] = c;
            } else {
                // discard until next newline
                ESP_LOGW(TAG, "RX line overflow, discarding");
                len = 0;
            }
        }
    }

    ESP_LOGE(TAG, "AT TIMEOUT (%s)", atCmd.cmd);
    return false;
}



void SimConnection::tick(void* arg) {
    auto* self = static_cast<SimConnection*>(arg);

    while (true) {
        switch (self->m_status) {

        case SimStatus::DISCONNECTED:
            break;

        case SimStatus::INIT:
            ESP_LOGI(TAG, "FSM: INIT");

            self->m_modem_uart.init(
                115200,
                GPIO_NUM_17,   // TX → Quectel RX
                GPIO_NUM_18,   // RX → Quectel TX
                GPIO_NUM_19,   // RTS
                GPIO_NUM_20    // CTS
            );

            vTaskDelay(pdMS_TO_TICKS(300));

            // Run all INIT commands from class table
            for (auto& cmd : at_command_table) {
                if (cmd.msg_type != MsgType::INIT) continue;
                if (!self->sendAT(cmd)) {
                    self->m_status = SimStatus::ERROR;
                    break;
                }
            }

            if (self->m_status != SimStatus::ERROR) {
                self->m_status = SimStatus::NOTREADY;
            }
            break;

        case SimStatus::NOTREADY:
            ESP_LOGI(TAG, "FSM: CHECK SIM");

            for (auto& cmd : at_command_table) {
                if (cmd.msg_type != MsgType::STATUS) continue;
                if (self->sendAT(cmd)) {
                    self->m_status = SimStatus::REGISTERING;
                    break;
                }
            }
            break;

        case SimStatus::REGISTERING:
            ESP_LOGI(TAG, "FSM: REGISTERING");

            for (auto& cmd : at_command_table) {
                if (cmd.msg_type != MsgType::NETREG) continue;
                if (self->sendAT(cmd)) {
                    ESP_LOGI(TAG, "Network registered");
                    self->m_retry_count = 0;
                    self->m_status = SimStatus::CONNECTED;
                    break;
                }
            }
            break;

        case SimStatus::CONNECTED:
            break;

        case SimStatus::ERROR:
            self->m_retry_count++;
            ESP_LOGE(TAG, "FSM: ERROR (%lu/%lu)", self->m_retry_count, MAX_RETRIES);

            if (self->m_retry_count >= MAX_RETRIES) {
                ESP_LOGE(TAG, "FSM: MAX RETRIES EXCEEDED");
                self->m_status = SimStatus::DISCONNECTED;
                break;
            }

            vTaskDelay(pdMS_TO_TICKS(5000));
            self->m_status = SimStatus::INIT;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(
            self->m_status == SimStatus::CONNECTED ? 1000 : 500));
    }

    self->m_task = nullptr;
    vTaskDelete(nullptr);
}


bool SimConnection::connect() {
    if (m_retry_count >= MAX_RETRIES) {
        ESP_LOGE(TAG, "SIM permanently failed");
        return false;
    }

    if (m_task) {
        ESP_LOGW(TAG, "SIM FSM already running");
        return true;
    }

    ESP_LOGI(TAG, "Starting SIM FSM task");
    m_status = SimStatus::INIT;

    BaseType_t res = xTaskCreate(
        &SimConnection::tick,
        "sim_fsm",
        4096,
        this,
        5,
        &m_task
    );

    if (res != pdPASS) {
        ESP_LOGE(TAG, "Failed to create SIM FSM task");
        m_task = nullptr;
        return false;
    }

    return true;
}

bool SimConnection::isConnected() {
    return m_status == SimStatus::CONNECTED;
}

void SimConnection::disconnect() {
    ESP_LOGI(TAG, "Disconnecting SIM");

    // Run shutdown command from class table
    for (auto& cmd : at_command_table) {
        if (cmd.msg_type == MsgType::SHUTDOWN) {
            sendAT(cmd);
            break;
        }
    }

    m_status = SimStatus::DISCONNECTED;

    if (m_task) {
        vTaskDelete(m_task);
        m_task = nullptr;
    }
}
