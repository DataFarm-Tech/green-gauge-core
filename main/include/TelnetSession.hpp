#pragma once

#include "ATCommandHndlr.hpp"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

class TelnetSession {
public:
    bool start(ATCommandHndlr& at_handler);
    void stop();
    bool isRunning() const;

private:
    static void sessionTaskEntry(void* arg);
    void sessionLoop();

    bool openSocket();
    void closeSocket();
    bool sendText(const char* text);
    bool readIncoming(char* out_buf, size_t out_len, size_t* out_read);
    void processIncoming(const char* data, size_t len);
    void executeCommand(const char* command_line);
    void printPrompt();

    ATCommandHndlr* hndlr = nullptr;
    TaskHandle_t session_task_handle = nullptr;
    volatile bool keep_running = false;
    volatile bool socket_open = false;
    char command_buffer[192] = {0};
    size_t command_len = 0;
    TickType_t last_activity_tick = 0;
    TickType_t next_connect_tick = 0;
    uint8_t open_failures = 0;

    static constexpr const char* TELNET_HOST = "45.79.239.100";
    static constexpr uint16_t TELNET_PORT = 5000;
    static constexpr uint8_t TELNET_CONTEXT_ID = 1;
    static constexpr uint8_t TELNET_CONNECT_ID = 1;
    static constexpr TickType_t SESSION_POLL_INTERVAL = pdMS_TO_TICKS(100);
    static constexpr TickType_t KEEPALIVE_INTERVAL = pdMS_TO_TICKS(30000);
    static constexpr TickType_t RECONNECT_DELAY = pdMS_TO_TICKS(5000);
    static constexpr TickType_t MAX_RECONNECT_DELAY = pdMS_TO_TICKS(120000);
};
