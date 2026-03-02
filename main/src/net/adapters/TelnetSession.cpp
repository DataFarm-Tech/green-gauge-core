#include "TelnetSession.hpp"

#include <cstring>
#include <cstdio>
#include "esp_system.h"
#include "esp_app_desc.h"

#ifndef PROJECT_VER
#define PROJECT_VER "unknown"
#endif

bool TelnetSession::start(ATCommandHndlr& at_handler)
{
    if (session_task_handle != nullptr)
    {
        return true;
    }

    hndlr = &at_handler;
    keep_running = true;

    BaseType_t created = xTaskCreate(
        sessionTaskEntry,
        "telnet_cli",
        6144,
        this,
        4,
        &session_task_handle);

    if (created != pdPASS)
    {
        session_task_handle = nullptr;
        keep_running = false;
        hndlr = nullptr;
        printf("Failed to create telnet CLI task\n");
        return false;
    }

    printf("Telnet CLI task started\n");
    return true;
}

void TelnetSession::stop()
{
    keep_running = false;

    if (session_task_handle != nullptr)
    {
        vTaskDelete(session_task_handle);
        session_task_handle = nullptr;
    }

    if (socket_open)
    {
        closeSocket();
    }

    hndlr = nullptr;
}

bool TelnetSession::isRunning() const
{
    return session_task_handle != nullptr;
}

void TelnetSession::sessionTaskEntry(void* arg)
{
    auto* self = static_cast<TelnetSession*>(arg);
    self->sessionLoop();
    vTaskDelete(nullptr);
}

void TelnetSession::sessionLoop()
{
    char rx_buf[256] = {0};

    while (keep_running)
    {
        if (!socket_open)
        {
            const TickType_t now = xTaskGetTickCount();
            if ((next_connect_tick != 0) && (static_cast<int32_t>(now - next_connect_tick) < 0))
            {
                vTaskDelay(SESSION_POLL_INTERVAL);
                continue;
            }

            if (!openSocket())
            {
                if (open_failures < 6)
                {
                    open_failures++;
                }

                TickType_t delay = RECONNECT_DELAY;
                for (uint8_t i = 1; i < open_failures; ++i)
                {
                    if (delay >= MAX_RECONNECT_DELAY / 2)
                    {
                        delay = MAX_RECONNECT_DELAY;
                        break;
                    }
                    delay *= 2;
                }

                if (delay > MAX_RECONNECT_DELAY)
                {
                    delay = MAX_RECONNECT_DELAY;
                }

                next_connect_tick = now + delay;
                printf("Telnet open failed, retrying in %lu ms (attempt %u)\n",
                       static_cast<unsigned long>(pdTICKS_TO_MS(delay)),
                       static_cast<unsigned>(open_failures));
                continue;
            }

            socket_open = true;
            open_failures = 0;
            next_connect_tick = 0;
            command_len = 0;
            last_activity_tick = xTaskGetTickCount();
            printf("Telnet CLI socket connected to %s:%u\n", TELNET_HOST, static_cast<unsigned>(TELNET_PORT));

            if (!sendText("\r\nGreen Gauge Telnet CLI\r\nType 'help' for commands\r\n") ||
                !sendText("> "))
            {
                printf("Telnet banner/prompt send failed, reconnecting socket\n");
                socket_open = false;
                next_connect_tick = xTaskGetTickCount() + RECONNECT_DELAY;
                continue;
            }
        }

        size_t bytes_read = 0;
        if (!readIncoming(rx_buf, sizeof(rx_buf), &bytes_read))
        {
            printf("Telnet read failed, reconnecting socket\n");
            socket_open = false;
            next_connect_tick = xTaskGetTickCount() + RECONNECT_DELAY;
            continue;
        }

        if (bytes_read > 0) {
            processIncoming(rx_buf, bytes_read);
            last_activity_tick = xTaskGetTickCount();
        } else {
            TickType_t now = xTaskGetTickCount();
            if ((now - last_activity_tick) >= KEEPALIVE_INTERVAL) {
                (void)sendText("\r\n");
                last_activity_tick = now;
            }
            vTaskDelay(SESSION_POLL_INTERVAL);
        }
    }

    if (socket_open)
    {
        closeSocket();
        socket_open = false;
    }

    session_task_handle = nullptr;
}

bool TelnetSession::openSocket()
{
    if (hndlr == nullptr)
    {
        return false;
    }

    return hndlr->openTCPSocket(
        TELNET_HOST,
        TELNET_PORT,
        TELNET_CONTEXT_ID,
        TELNET_CONNECT_ID,
        20000);
}

void TelnetSession::closeSocket()
{
    if (hndlr == nullptr)
    {
        return;
    }

    ATCommand_t close_cmd = {
        "AT+QICLOSE=1",
        "OK",
        5000,
        MsgType::DATA,
        nullptr,
        0
    };

    (void)hndlr->send(close_cmd);
}

bool TelnetSession::sendText(const char* text)
{
    if (hndlr == nullptr || text == nullptr)
    {
        return false;
    }

    const size_t text_len = strlen(text);
    if (text_len == 0) {
        return true;
    }

    return hndlr->sendSocketData(
        TELNET_CONNECT_ID,
        reinterpret_cast<const uint8_t*>(text),
        text_len,
        5000);
}

bool TelnetSession::readIncoming(char* out_buf, size_t out_len, size_t* out_read)
{
    if (hndlr == nullptr || out_buf == nullptr || out_len == 0 || out_read == nullptr) {
        return false;
    }

    return hndlr->readSocketData(
        TELNET_CONNECT_ID,
        out_buf,
        out_len,
        out_read,
        3000,
        200);
}

void TelnetSession::processIncoming(const char* data, size_t len)
{
    if (data == nullptr || len == 0) {
        return;
    }

    for (size_t i = 0; i < len; ++i)
    {
        const char c = data[i];

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            (void)sendText("\r\n");
            command_buffer[command_len] = '\0';
            executeCommand(command_buffer);
            command_len = 0;
            printPrompt();
            continue;
        }

        if ((c == '\b' || c == 0x7F) && command_len > 0) {
            command_len--;
            (void)sendText("\b \b");
            continue;
        }

        if (c >= 32 && c < 127) {
            if (command_len < sizeof(command_buffer) - 1) {
                command_buffer[command_len++] = c;
                char echo[2] = {c, '\0'};
                (void)sendText(echo);
            }
        }
    }
}

void TelnetSession::executeCommand(const char* command_line)
{
    if (command_line == nullptr || command_line[0] == '\0') {
        return;
    }

    if (strcmp(command_line, "help") == 0) {
        (void)sendText(
            "Commands:\r\n"
            "  help      Show commands\r\n"
            "  status    Show telnet session status\r\n"
            "  version   Show firmware version\r\n"
            "  reset     Reboot device\r\n");
        return;
    }

    if (strcmp(command_line, "status") == 0) {
        (void)sendText("Telnet CLI connected\r\n");
        return;
    }

    if (strcmp(command_line, "version") == 0) {
        const esp_app_desc_t* app_desc = esp_app_get_description();
        char msg[256] = {0};
        snprintf(msg,
                 sizeof(msg),
                 "Project: %s\r\nVersion: %s\r\nBuild: %s %s\r\nIDF: %s\r\n",
                 app_desc->project_name,
                 app_desc->version,
                 app_desc->date,
                 app_desc->time,
                 app_desc->idf_ver);
        (void)sendText(msg);
        return;
    }

    if (strcmp(command_line, "reset") == 0) {
        (void)sendText("Rebooting...\r\n");
        vTaskDelay(pdMS_TO_TICKS(300));
        esp_restart();
        return;
    }

    (void)sendText("Unknown command. Type 'help'.\r\n");
}

void TelnetSession::printPrompt()
{
    (void)sendText("> ");
}
