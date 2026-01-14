#include "SimConnection.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include "Types.hpp"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

static const char* TAG = "SIM";


bool SimConnection::sendAT(const char* cmd, const char* expect, int timeout_ms)
{
    char rx_buf[GEN_BUFFER_SIZE] = {};
    int idx = 0;
    TickType_t start = xTaskGetTickCount();

    m_modem_uart.writef("%s\r\n", cmd);

    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms)) {
        uint8_t c;
        if (m_modem_uart.readByte(c) == 1) {
            if (idx < (int)sizeof(rx_buf) - 1) {
                rx_buf[idx++] = c;
                rx_buf[idx] = '\0';
            }

            if (strstr(rx_buf, expect)) {
                ESP_LOGI(TAG, "AT OK: %s", cmd);
                return true;
            }

            if (strstr(rx_buf, "ERROR")) {
                ESP_LOGE(TAG, "AT ERROR: %s", rx_buf);
                return false;
            }
        }
    }

    ESP_LOGE(TAG, "AT TIMEOUT: %s", cmd);
    return false;
}

void SimConnection::tick(void* arg)
{
    auto* self = static_cast<SimConnection*>(arg);

    while (true) {
        switch (self->m_status) {

        case SimStatus::DISCONNECTED:
            /* Idle */
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

            if (!self->sendAT("AT", "OK", 1000)) {
                self->m_status = SimStatus::ERROR;
                break;
            }

            self->sendAT("ATE0", "OK", 1000);
            self->m_status = SimStatus::NOTREADY;
            break;

        case SimStatus::NOTREADY:
            ESP_LOGI(TAG, "FSM: CHECK SIM");

            if (self->sendAT("AT+CPIN?", "READY", 2000)) {
                self->m_status = SimStatus::REGISTERING;
            }
            break;

        case SimStatus::REGISTERING:
            ESP_LOGI(TAG, "FSM: REGISTERING");

            if (self->sendAT("AT+CEREG?", ",1", 3000) ||
                self->sendAT("AT+CEREG?", ",5", 3000)) {

                ESP_LOGI(TAG, "Network registered");
                self->m_retry_count = 0;   // ✅ success resets retries
                self->m_status = SimStatus::CONNECTED;
            }
            break;

        case SimStatus::CONNECTED:
            /* Link monitoring / URCs later */
            break;

        case SimStatus::ERROR:
            self->m_retry_count++;

            ESP_LOGE(
                TAG,
                "FSM: ERROR (%lu/%lu)",
                self->m_retry_count,
                MAX_RETRIES
            );

            if (self->m_retry_count >= MAX_RETRIES) {
                ESP_LOGE(TAG, "FSM: MAX RETRIES EXCEEDED");
                self->m_status = SimStatus::DISCONNECTED;
                self->m_task = nullptr;
                vTaskDelete(nullptr);   // Kill task permanently
                return;
            }

            vTaskDelay(pdMS_TO_TICKS(5000));
            self->m_status = SimStatus::INIT;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

bool SimConnection::connect()
{
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

    xTaskCreate(
        &SimConnection::tick,
        "sim_fsm",
        4096,
        this,
        5,
        &m_task
    );
    

    return true;
}

bool SimConnection::isConnected()
{
    return m_status == SimStatus::CONNECTED;
}

void SimConnection::disconnect()
{
    ESP_LOGI(TAG, "Disconnecting SIM");

    sendAT("AT+CFUN=0", "OK", 3000);
    m_status = SimStatus::DISCONNECTED;

    if (m_task) {
        vTaskDelete(m_task);
        m_task = nullptr;
    }
}
