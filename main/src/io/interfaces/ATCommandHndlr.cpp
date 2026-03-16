#include "ATCommandHndlr.hpp"
#include "UARTDriver.hpp"
#include "Logger.hpp"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

SemaphoreHandle_t ATCommandHndlr::s_cmd_mutex = nullptr;

static void sanitizePrintable(const char* src, char* dst, size_t dst_len)
{
    if (dst == nullptr || dst_len == 0)
    {
        return;
    }

    if (src == nullptr)
    {
        dst[0] = '\0';
        return;
    }

    size_t i = 0;
    for (; src[i] != '\0' && i < (dst_len - 1); ++i)
    {
        const unsigned char ch = static_cast<unsigned char>(src[i]);
        dst[i] = (ch >= 32U && ch <= 126U) ? static_cast<char>(ch) : '.';
    }
    dst[i] = '\0';
}

static void printSanitizedRx(const char* line)
{
    char sanitized[256] = {0};
    sanitizePrintable(line, sanitized, sizeof(sanitized));
    printf("RX: %s\n", sanitized);
}

static void printSanitizedMsg(const char* prefix, const char* line)
{
    char sanitized[256] = {0};
    sanitizePrintable(line, sanitized, sizeof(sanitized));
    printf("%s%s\n", (prefix != nullptr) ? prefix : "", sanitized);
}

bool ATCommandHndlr::ensureCmdMutex() {
    if (s_cmd_mutex != nullptr) {
        return true;
    }

    s_cmd_mutex = xSemaphoreCreateRecursiveMutex();
    if (s_cmd_mutex == nullptr) {
        printf("Failed to create AT command mutex\n");
        return false;
    }

    return true;
}

bool ATCommandHndlr::lockCmd() {
    if (!ensureCmdMutex()) {
        return false;
    }

    if (xSemaphoreTakeRecursive(s_cmd_mutex, pdMS_TO_TICKS(15000)) != pdTRUE) {
        printf("Timeout waiting for AT command mutex\n");
        return false;
    }

    return true;
}

void ATCommandHndlr::unlockCmd() {
    if (s_cmd_mutex != nullptr) {
        (void)xSemaphoreGiveRecursive(s_cmd_mutex);
    }
}

bool ATCommandHndlr::send(const ATCommand_t& atCmd) {
    if (!lockCmd()) {
        return false;
    }

    if (!atCmd.cmd || !atCmd.expect) {
        printf("Invalid AT command parameters\n");
        unlockCmd();
        return false;
    }

    // g_logger.info("TX: %s\n", atCmd.cmd);
    m_modem_uart.writef("%s\r\n", atCmd.cmd);

    // Check if this command includes a payload
    if (atCmd.payload != nullptr && atCmd.payload_len > 0) {
        // Some commands (e.g., QHTTPURL) use "CONNECT" readiness instead of '>' prompt.
        auto waitForPayloadReady = [this](const char* expect, int timeout_ms) -> bool {
            const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
            char line_buf[128] = {0};
            size_t line_len = 0;

            while (xTaskGetTickCount() < deadline) {
                uint8_t c = 0;
                if (!m_modem_uart.readByte(c)) {
                    vTaskDelay(pdMS_TO_TICKS(1));
                    continue;
                }

                if (c == '>') {
                    return true;
                }

                if (c == '\n') {
                    line_buf[line_len] = '\0';
                    if (line_len > 0 && line_buf[line_len - 1] == '\r') {
                        line_buf[--line_len] = '\0';
                    }

                    if (line_len > 0) {
                        printSanitizedRx(line_buf);

                        if (strcmp(line_buf, "ERROR") == 0 || strstr(line_buf, "+CME ERROR") != nullptr) {
                            printSanitizedMsg("Payload readiness failed due to modem error: ", line_buf);
                            return false;
                        }

                        if (expect != nullptr && expect[0] != '\0' && strstr(line_buf, expect) != nullptr) {
                            return true;
                        }
                    }

                    line_len = 0;
                } else if (c != '\r') {
                    if (line_len < sizeof(line_buf) - 1) {
                        line_buf[line_len++] = static_cast<char>(c);
                    } else {
                        line_len = 0;
                    }
                }
            }

            return false;
        };

        if (!waitForPayloadReady(atCmd.expect, atCmd.timeout_ms)) {
            printf("Failed waiting for payload readiness after command: %s\n", atCmd.cmd);
            unlockCmd();
            return false;
        }

        // Send payload and wait for response
        const bool success = sendPayloadAndWaitResponse(atCmd.payload, atCmd.payload_len);
        unlockCmd();
        return success;
    } else {
        // Standard command - wait for expected response and OK
        ResponseState state = {};
        const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(atCmd.timeout_ms);

        while (xTaskGetTickCount() < deadline) {
            if (processResponse(state, atCmd)) {
                unlockCmd();
                return state.success;
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        printf("AT TIMEOUT: %s (after %dms)\n", atCmd.cmd, atCmd.timeout_ms);
        unlockCmd();
        return false;
    }
}

bool ATCommandHndlr::sendAndCapture(const ATCommand_t& atCmd, char* out_buf, size_t out_len) {
    if (!lockCmd()) {
        return false;
    }

    if (!atCmd.cmd || !atCmd.expect || out_buf == nullptr || out_len == 0) {
        printf("Invalid parameters to sendAndCapture\n");
        unlockCmd();
        return false;
    }

    // g_logger.info("TX(capture): %s\n", atCmd.cmd);
    m_modem_uart.writef("%s\r\n", atCmd.cmd);

    ResponseState state = {};
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(atCmd.timeout_ms);

    while (xTaskGetTickCount() < deadline) {
        uint8_t c;
        if (!m_modem_uart.readByte(c)) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        if (c == '\n') {
            state.line_buffer[state.line_len] = '\0';
            // strip trailing CR
            if (state.line_len > 0 && state.line_buffer[state.line_len - 1] == '\r') {
                state.line_buffer[--state.line_len] = '\0';
            }

            if (state.line_len > 0) {
                printSanitizedRx(state.line_buffer);

                // Check for CME or generic ERROR
                if (strcmp(state.line_buffer, "ERROR") == 0 || strstr(state.line_buffer, "+CME ERROR") != nullptr) {
                    printSanitizedMsg("AT response error (capture): ", state.line_buffer);
                    unlockCmd();
                    return false;
                }

                // If the expected substring is present, capture and return success
                if (strstr(state.line_buffer, atCmd.expect) != nullptr) {
                    // Copy into out_buf
                    strncpy(out_buf, state.line_buffer, out_len - 1);
                    out_buf[out_len - 1] = '\0';
                    unlockCmd();
                    return true;
                }
            }

            state.line_len = 0;
        } else if (c != '\r') {
            if (state.line_len < sizeof(state.line_buffer) - 1) {
                state.line_buffer[state.line_len++] = c;
            } else {
                printf("RX buffer overflow in sendAndCapture, discarding line\n");
                state.line_len = 0;
            }
        }
    }

    printf("AT TIMEOUT (capture): %s (after %dms)\n", atCmd.cmd, atCmd.timeout_ms);
    unlockCmd();
    return false;
}

bool ATCommandHndlr::openIPSocket(const char* protocol,
                                  const char* host,
                                  uint16_t port,
                                  uint8_t context_id,
                                  uint8_t connect_id,
                                  int timeout_ms) {
    if (protocol == nullptr || protocol[0] == '\0' ||
        host == nullptr || host[0] == '\0' ||
        port == 0) {
        printf("Invalid parameters to openIPSocket\n");
        return false;
    }

    char cmd[160] = {0};
    const int written = snprintf(cmd,
                                 sizeof(cmd),
                                 "AT+QIOPEN=%u,%u,\"%s\",\"%s\",%u,0,0",
                                 static_cast<unsigned>(context_id),
                                 static_cast<unsigned>(connect_id),
                                 protocol,
                                 host,
                                 static_cast<unsigned>(port));

    if (written <= 0 || written >= static_cast<int>(sizeof(cmd))) {
        printf("AT+QIOPEN command formatting failed\n");
        return false;
    }

    if (!lockCmd()) {
        return false;
    }

    m_modem_uart.writef("%s\r\n", cmd);

    char line_buf[256] = {0};
    size_t line_len = 0;
    bool got_qiopen_for_target = false;
    int target_err_code = -1;

    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    while (xTaskGetTickCount() < deadline) {
        uint8_t c = 0;
        if (!m_modem_uart.readByte(c)) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        if (c == '\n') {
            line_buf[line_len] = '\0';
            if (line_len > 0 && line_buf[line_len - 1] == '\r') {
                line_buf[--line_len] = '\0';
            }

            if (line_len > 0) {
                printSanitizedRx(line_buf);

                if (strcmp(line_buf, "ERROR") == 0 || strstr(line_buf, "+CME ERROR") != nullptr) {
                    printSanitizedMsg("AT response error while opening socket: ", line_buf);
                    unlockCmd();
                    return false;
                }

                if (strncmp(line_buf, "+QIOPEN:", 8) == 0) {
                    int resp_connect_id = -1;
                    int err_code = -1;
                    if (sscanf(line_buf, "+QIOPEN: %d,%d", &resp_connect_id, &err_code) == 2) {
                        if (resp_connect_id == static_cast<int>(connect_id)) {
                            got_qiopen_for_target = true;
                            target_err_code = err_code;
                            break;
                        }

                        printf("Ignoring QIOPEN for other socket id: %d\n", resp_connect_id);
                    }
                }
            }

            line_len = 0;
        } else if (c != '\r') {
            if (line_len < sizeof(line_buf) - 1) {
                line_buf[line_len++] = static_cast<char>(c);
            } else {
                line_len = 0;
            }
        }
    }

    unlockCmd();

    if (!got_qiopen_for_target) {
        printf("QIOPEN timeout waiting for socket id %u\n", static_cast<unsigned>(connect_id));
        return false;
    }

    if (target_err_code != 0) {
        printf("QIOPEN failed with modem error code: %d\n", target_err_code);
        return false;
    }

    printf("%s socket opened successfully (id=%u)\n",
           protocol,
           static_cast<unsigned>(connect_id));
    return true;
}

bool ATCommandHndlr::openTCPSocket(const char* host,
                                   uint16_t port,
                                   uint8_t context_id,
                                   uint8_t connect_id,
                                   int timeout_ms) {
    return openIPSocket("TCP", host, port, context_id, connect_id, timeout_ms);
}

bool ATCommandHndlr::openSSLSocket(const char* host,
                                   uint16_t port,
                                   uint8_t ssl_context_id,
                                   uint8_t connect_id,
                                   int timeout_ms) {
    if (host == nullptr || host[0] == '\0' || port == 0) {
        printf("Invalid parameters to openSSLSocket\n");
        return false;
    }

    char cfg_cmd[96] = {0};
    ATCommand_t cfg = {
        cfg_cmd,
        "OK",
        5000,
        MsgType::DATA,
        nullptr,
        0
    };

    snprintf(cfg_cmd, sizeof(cfg_cmd), "AT+QSSLCFG=\"sslversion\",%u,4", static_cast<unsigned>(ssl_context_id));
    if (!send(cfg)) {
        printf("QSSLCFG sslversion failed\n");
        return false;
    }

    snprintf(cfg_cmd, sizeof(cfg_cmd), "AT+QSSLCFG=\"seclevel\",%u,0", static_cast<unsigned>(ssl_context_id));
    if (!send(cfg)) {
        printf("QSSLCFG seclevel failed\n");
        return false;
    }

    snprintf(cfg_cmd, sizeof(cfg_cmd), "AT+QSSLCFG=\"sni\",%u,1", static_cast<unsigned>(ssl_context_id));
    if (!send(cfg)) {
        printf("QSSLCFG sni failed\n");
        return false;
    }

    // Bind SSL context to PDP context 1 for modems that require explicit context mapping.
    snprintf(cfg_cmd, sizeof(cfg_cmd), "AT+QSSLCFG=\"contextid\",%u,1", static_cast<unsigned>(ssl_context_id));
    if (!send(cfg)) {
        // Not all Quectel firmware builds support this key; continue without it.
        printf("QSSLCFG contextid unsupported/failed, continuing without explicit context binding\n");
    }

    static constexpr size_t kOpenFormatCount = 7;
    for (size_t fmt_idx = 0; fmt_idx < kOpenFormatCount; ++fmt_idx) {
        char open_cmd[160] = {0};
        int written = 0;

        if (fmt_idx == 0) {
            written = snprintf(open_cmd,
                               sizeof(open_cmd),
                               "AT+QSSLOPEN=%u,%u,\"%s\",%u,0",
                               static_cast<unsigned>(ssl_context_id),
                               static_cast<unsigned>(connect_id),
                               host,
                               static_cast<unsigned>(port));
        } else if (fmt_idx == 1) {
            written = snprintf(open_cmd,
                               sizeof(open_cmd),
                               "AT+QSSLOPEN=%u,%u,\"%s\",%u",
                               static_cast<unsigned>(ssl_context_id),
                               static_cast<unsigned>(connect_id),
                               host,
                               static_cast<unsigned>(port));
        } else if (fmt_idx == 2) {
            written = snprintf(open_cmd,
                               sizeof(open_cmd),
                               "AT+QSSLOPEN=%u,\"%s\",%u,%u",
                               static_cast<unsigned>(connect_id),
                               host,
                               static_cast<unsigned>(port),
                               static_cast<unsigned>(ssl_context_id));
        } else if (fmt_idx == 3) {
            written = snprintf(open_cmd,
                               sizeof(open_cmd),
                               "AT+QSSLOPEN=%u,\"%s\",%u,0",
                               static_cast<unsigned>(ssl_context_id),
                               host,
                               static_cast<unsigned>(port));
        } else if (fmt_idx == 4) {
            written = snprintf(open_cmd,
                               sizeof(open_cmd),
                               "AT+QSSLOPEN=%u,\"%s\",%u",
                               static_cast<unsigned>(ssl_context_id),
                               host,
                               static_cast<unsigned>(port));
        } else if (fmt_idx == 5) {
            written = snprintf(open_cmd,
                               sizeof(open_cmd),
                               "AT+QSSLOPEN=%u,\"%s\",%u",
                               static_cast<unsigned>(connect_id),
                               host,
                               static_cast<unsigned>(port));
        } else {
            written = snprintf(open_cmd,
                               sizeof(open_cmd),
                               "AT+QSSLOPEN=\"%s\",%u",
                               host,
                               static_cast<unsigned>(port));
        }

        if (written <= 0 || written >= static_cast<int>(sizeof(open_cmd))) {
            continue;
        }

        if (!lockCmd()) {
            return false;
        }

        m_modem_uart.writef("%s\r\n", open_cmd);

        char line_buf[256] = {0};
        size_t line_len = 0;
        bool got_open_result = false;
        bool syntax_error = false;
        int target_err_code = -1;

        const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
        while (xTaskGetTickCount() < deadline) {
            uint8_t c = 0;
            if (!m_modem_uart.readByte(c)) {
                vTaskDelay(pdMS_TO_TICKS(1));
                continue;
            }

            if (c == '\n') {
                line_buf[line_len] = '\0';
                if (line_len > 0 && line_buf[line_len - 1] == '\r') {
                    line_buf[--line_len] = '\0';
                }

                if (line_len > 0) {
                    printSanitizedRx(line_buf);

                    if (strcmp(line_buf, "ERROR") == 0 || strstr(line_buf, "+CME ERROR") != nullptr) {
                        syntax_error = true;
                        break;
                    }

                    if (strncmp(line_buf, "+QSSLOPEN:", 10) == 0) {
                        int resp_connect_id = -1;
                        int err_code = -1;
                        if (sscanf(line_buf, "+QSSLOPEN: %d,%d", &resp_connect_id, &err_code) == 2) {
                            if (resp_connect_id == static_cast<int>(connect_id)) {
                                got_open_result = true;
                                target_err_code = err_code;
                                break;
                            }
                        } else if (sscanf(line_buf, "+QSSLOPEN: %d", &err_code) == 1) {
                            // Some modem variants return only an error code.
                            got_open_result = true;
                            target_err_code = err_code;
                            break;
                        }
                    }
                }

                line_len = 0;
            } else if (c != '\r') {
                if (line_len < sizeof(line_buf) - 1) {
                    line_buf[line_len++] = static_cast<char>(c);
                } else {
                    line_len = 0;
                }
            }
        }

        unlockCmd();

        if (got_open_result) {
            if (target_err_code != 0) {
                printf("QSSLOPEN failed with modem error code: %d\n", target_err_code);
                return false;
            }

            printf("SSL socket opened successfully (id=%u)\n", static_cast<unsigned>(connect_id));
            return true;
        }

        if (syntax_error) {
            printf("QSSLOPEN format variant %u rejected, trying next\n", static_cast<unsigned>(fmt_idx + 1));
            continue;
        }

        printf("QSSLOPEN timeout with format variant %u\n", static_cast<unsigned>(fmt_idx + 1));
    }

    printf("AT response error while opening SSL socket: all QSSLOPEN variants failed\n");
    return false;
}

bool ATCommandHndlr::sendSocketData(uint8_t connect_id,
                                    const uint8_t* payload,
                                    size_t payload_len,
                                    int timeout_ms) {
    if (payload == nullptr || payload_len == 0) {
        return false;
    }

    char cmd[48] = {0};
    const int written = snprintf(cmd,
                                 sizeof(cmd),
                                 "AT+QISEND=%u,%u",
                                 static_cast<unsigned>(connect_id),
                                 static_cast<unsigned>(payload_len));
    if (written <= 0 || written >= static_cast<int>(sizeof(cmd))) {
        return false;
    }

    ATCommand_t send_cmd = {
        cmd,
        ">",
        timeout_ms,
        MsgType::DATA,
        payload,
        payload_len
    };

    return send(send_cmd);
}

bool ATCommandHndlr::readSocketData(uint8_t connect_id,
                                    char* out_buf,
                                    size_t out_len,
                                    size_t* out_read,
                                    int timeout_ms,
                                    size_t max_read) {
    if (out_buf == nullptr || out_len == 0 || out_read == nullptr || max_read == 0) {
        return false;
    }

    if (!lockCmd()) {
        return false;
    }

    *out_read = 0;
    out_buf[0] = '\0';

    char cmd[48] = {0};
    const unsigned request_len = static_cast<unsigned>((max_read > 1024U) ? 1024U : max_read);
    const int written = snprintf(cmd,
                                 sizeof(cmd),
                                 "AT+QIRD=%u,%u",
                                 static_cast<unsigned>(connect_id),
                                 request_len);
    if (written <= 0 || written >= static_cast<int>(sizeof(cmd))) {
        unlockCmd();
        return false;
    }

    m_modem_uart.writef("%s\r\n", cmd);

    char line_buf[128] = {0};
    size_t line_len = 0;
    int expected_data_len = -1;
    size_t copied = 0;
    int remaining_data_bytes = 0;
    bool got_ok = false;

    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    while (xTaskGetTickCount() < deadline) {
        uint8_t c = 0;
        if (!m_modem_uart.readByte(c)) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        if (expected_data_len >= 0 && remaining_data_bytes > 0) {
            if (copied < (out_len - 1)) {
                out_buf[copied++] = static_cast<char>(c);
            }

            remaining_data_bytes--;
            if (remaining_data_bytes == 0) {
                out_buf[copied] = '\0';
                *out_read = copied;
            }
            continue;
        }

        if (c == '\n') {
            line_buf[line_len] = '\0';
            if (line_len > 0 && line_buf[line_len - 1] == '\r') {
                line_buf[--line_len] = '\0';
            }

            if (line_len > 0) {
                if (strcmp(line_buf, "OK") == 0) {
                    got_ok = true;
                    break;
                }

                if (strcmp(line_buf, "ERROR") == 0 || strstr(line_buf, "+CME ERROR") != nullptr) {
                    unlockCmd();
                    return false;
                }

                if (strstr(line_buf, "+QIURC: \"closed\"") != nullptr) {
                    int closed_id = -1;
                    if (sscanf(line_buf, "+QIURC: \"closed\",%d", &closed_id) == 1) {
                        if (closed_id == static_cast<int>(connect_id)) {
                            unlockCmd();
                            return false;
                        }
                    } else {
                        unlockCmd();
                        return false;
                    }
                }

                if (strncmp(line_buf, "+QIRD:", 6) == 0) {
                    int parsed_len = -1;
                    if (sscanf(line_buf, "+QIRD: %d", &parsed_len) == 1) {
                        if (parsed_len <= 0) {
                            *out_read = 0;
                        } else {
                            expected_data_len = parsed_len;
                            remaining_data_bytes = parsed_len;
                        }
                    }
                }
            }

            line_len = 0;
        } else if (c != '\r') {
            if (line_len < sizeof(line_buf) - 1) {
                line_buf[line_len++] = static_cast<char>(c);
            } else {
                line_len = 0;
            }
        }
    }

    unlockCmd();
    return got_ok;
}

bool ATCommandHndlr::httpGetStream(const std::string& url,
                                   const ModemChunkCallback& onChunk,
                                   int timeout_seconds) {
    if (url.empty() || !onChunk) {
        return false;
    }

    char cmd_buf[96] = {0};
    ATCommand_t cmd = {
        cmd_buf,
        "OK",
        8000,
        MsgType::DATA,
        nullptr,
        0
    };

    snprintf(cmd_buf, sizeof(cmd_buf), "AT+QHTTPCFG=\"contextid\",1");
    if (!send(cmd)) {
        printf("QHTTPCFG contextid failed\n");
        return false;
    }

    snprintf(cmd_buf, sizeof(cmd_buf), "AT+QHTTPCFG=\"sslctxid\",1");
    if (!send(cmd)) {
        // Not all firmware supports sslctxid here.
        printf("QHTTPCFG sslctxid unsupported, continuing\n");
    }

    char url_cmd[64] = {0};
    snprintf(url_cmd, sizeof(url_cmd), "AT+QHTTPURL=%u,30", static_cast<unsigned>(url.size()));
    ATCommand_t url_load = {
        url_cmd,
        "CONNECT",
        10000,
        MsgType::DATA,
        reinterpret_cast<const uint8_t*>(url.data()),
        url.size()
    };

    if (!send(url_load)) {
        printf("QHTTPURL failed\n");
        return false;
    }

    char get_resp[128] = {0};
    ATCommand_t http_get = {
        "AT+QHTTPGET=80",
        "+QHTTPGET:",
        timeout_seconds * 1000,
        MsgType::DATA,
        nullptr,
        0
    };

    if (!sendAndCapture(http_get, get_resp, sizeof(get_resp))) {
        printf("QHTTPGET failed\n");
        return false;
    }

    int get_result = -1;
    int status_code = -1;
    int content_len = -1;
    if (sscanf(get_resp, "+QHTTPGET: %d,%d,%d", &get_result, &status_code, &content_len) < 2) {
        printf("Unable to parse QHTTPGET response: %s\n", get_resp);
        return false;
    }

    if (get_result != 0 || (status_code != 200 && status_code != 206)) {
        printf("QHTTPGET returned result=%d status=%d\n", get_result, status_code);
        return false;
    }

    printf("QHTTPGET success: status=%d, content_len=%d\n", status_code, content_len);
    printf("Starting QHTTPREAD stream...\n");

    if (!lockCmd()) {
        return false;
    }

    m_modem_uart.writef("AT+QHTTPREAD=120\r\n");

    char line_buf[128] = {0};
    size_t line_len = 0;
    int expected_len = -1;
    int remaining = 0;
    bool got_ok = false;
    bool in_connect_data_phase = false;
    uint8_t chunk_buf[512];
    size_t chunk_len = 0;
    size_t total_streamed = 0;

    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_seconds * 1000);
    while (xTaskGetTickCount() < deadline) {
        uint8_t c = 0;
        if (!m_modem_uart.readByte(c)) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        if ((expected_len >= 0 || in_connect_data_phase) && remaining > 0) {
            chunk_buf[chunk_len++] = c;
            remaining--;

            if (chunk_len == sizeof(chunk_buf) || remaining == 0) {
                if (!onChunk(chunk_buf, chunk_len)) {
                    unlockCmd();
                    return false;
                }
                total_streamed += chunk_len;
                if ((total_streamed % (32 * 1024)) < chunk_len) {
                    printf("QHTTPREAD progress: %u bytes\n", static_cast<unsigned>(total_streamed));
                }
                chunk_len = 0;
            }

            if (remaining == 0 && in_connect_data_phase) {
                in_connect_data_phase = false;
            }

            continue;
        }

        if (c == '\n') {
            line_buf[line_len] = '\0';
            if (line_len > 0 && line_buf[line_len - 1] == '\r') {
                line_buf[--line_len] = '\0';
            }

            if (line_len > 0) {
                if (strcmp(line_buf, "ERROR") == 0 || strstr(line_buf, "+CME ERROR") != nullptr) {
                    unlockCmd();
                    return false;
                }

                if (strncmp(line_buf, "+QHTTPREAD:", 11) == 0) {
                    if (sscanf(line_buf, "+QHTTPREAD: %d", &expected_len) == 1 && expected_len >= 0) {
                        remaining = expected_len;
                        printf("QHTTPREAD payload announced: %d bytes\n", expected_len);
                    }
                }

                if (strcmp(line_buf, "CONNECT") == 0) {
                    if (content_len > 0) {
                        expected_len = content_len;
                        remaining = content_len;
                        in_connect_data_phase = true;
                        printf("QHTTPREAD CONNECT mode: expecting %d bytes\n", content_len);
                    } else {
                        printf("QHTTPREAD CONNECT received without known content length\n");
                    }
                }

                if (strcmp(line_buf, "OK") == 0) {
                    got_ok = true;
                    break;
                }
            }

            line_len = 0;
        } else if (c != '\r') {
            if (line_len < sizeof(line_buf) - 1) {
                line_buf[line_len++] = static_cast<char>(c);
            } else {
                line_len = 0;
            }
        }
    }

    unlockCmd();

    if (!got_ok) {
        printf("QHTTPREAD timeout or incomplete transfer\n");
        return false;
    }

    if (total_streamed == 0) {
        printf("QHTTPREAD completed but no payload bytes were streamed\n");
        return false;
    }

    printf("QHTTPREAD complete: streamed %u bytes\n", static_cast<unsigned>(total_streamed));

    return true;
}

bool ATCommandHndlr::waitForPrompt(int timeout_ms) {
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    char line_buf[128] = {0};
    size_t line_len = 0;
    
    while (xTaskGetTickCount() < deadline) {
        uint8_t c;
        if (m_modem_uart.readByte(c)) {
            if (c == '>') {
                printf("Got '>' prompt\n");
                return true;
            }

            if (c == '\n') {
                line_buf[line_len] = '\0';
                if (line_len > 0 && line_buf[line_len - 1] == '\r') {
                    line_buf[--line_len] = '\0';
                }

                if (line_len > 0) {
                    if (strcmp(line_buf, "ERROR") == 0 || strstr(line_buf, "+CME ERROR") != nullptr) {
                        printSanitizedMsg("Prompt wait failed due to modem error: ", line_buf);
                        return false;
                    }

                    if (strstr(line_buf, "+QIURC: \"closed\"") != nullptr) {
                        printSanitizedMsg("Prompt wait aborted: ", line_buf);
                        return false;
                    }
                }

                line_len = 0;
            } else if (c != '\r') {
                if (line_len < sizeof(line_buf) - 1) {
                    line_buf[line_len++] = static_cast<char>(c);
                } else {
                    line_len = 0;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    return false;
}
bool ATCommandHndlr::sendPayloadAndWaitResponse(const uint8_t* payload, size_t payload_len) {
    // Send binary payload
    for (size_t i = 0; i < payload_len; i++) {
        m_modem_uart.writeByte(payload[i]);
    }
    
    printf("Sent %zu bytes of payload\n", payload_len);
    
    // Small delay to ensure data is transmitted
    vTaskDelay(pdMS_TO_TICKS(100));

    // Wait for SEND OK or ERROR response
    ResponseState state = {};
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(10000);

    while (xTaskGetTickCount() < deadline) {
        uint8_t c;
        
        if (!m_modem_uart.readByte(c)) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        // Handle line termination
        if (c == '\n') {
            state.line_buffer[state.line_len] = '\0';
            if (state.line_len > 0 && state.line_buffer[state.line_len - 1] == '\r') {
                state.line_buffer[--state.line_len] = '\0';
            }

            if (state.line_len > 0) {
                printSanitizedRx(state.line_buffer);

                // Check for SEND OK (UDP/TCP socket send confirmation)
                if (strcmp(state.line_buffer, "SEND OK") == 0) {
                    printf("Payload sent successfully\n");
                    return true;
                }

                // Check for +QCOAPSEND response (CoAP-specific)
                if (strstr(state.line_buffer, "+QCOAPSEND:") != nullptr) {
                    // Check for success response codes (65 = 2.01 Created, 69 = 2.05 Content)
                    if (strstr(state.line_buffer, ",65") != nullptr || 
                        strstr(state.line_buffer, ",69") != nullptr) {
                        printf("CoAP server acknowledged packet\n");
                    }
                }

                // Check for OK (generic success)
                if (strcmp(state.line_buffer, "OK") == 0) {
                    printf("Payload send confirmed with OK\n");
                    return true;
                }

                // Check for SEND FAIL
                if (strcmp(state.line_buffer, "SEND FAIL") == 0) {
                    printf("Payload send failed\n");
                    return false;
                }

                // Check for ERROR
                if (strcmp(state.line_buffer, "ERROR") == 0) {
                    printf("Payload send error\n");
                    return false;
                }

                // Check for +QIURC: "closed" (connection closed)
                if (strstr(state.line_buffer, "+QIURC: \"closed\"") != nullptr) {
                    printf("Socket closed during send\n");
                    return false;
                }
            }

            state.line_len = 0;
        } 
        else if (c != '\r') {
            if (state.line_len < sizeof(state.line_buffer) - 1) {
                state.line_buffer[state.line_len++] = c;
            } else {
                printf("RX buffer overflow\n");
                state.line_len = 0;
            }
        }
    }

    printf("Timeout waiting for payload confirmation\n");
    return false;
}

bool ATCommandHndlr::processResponse(ResponseState& state, const ATCommand_t& atCmd) {
    uint8_t c;
    
    // Non-blocking read
    if (!m_modem_uart.readByte(c)) {
        return false;  // No data available
    }

    // Handle line termination
    if (c == '\n') {
        return handleCompleteLine(state, atCmd);
    }

    // Accumulate characters (ignore \r)
    if (c != '\r') {
        if (state.line_len < sizeof(state.line_buffer) - 1) {
            state.line_buffer[state.line_len++] = c;
        } else {
            printf("RX buffer overflow, discarding line\n");
            state.line_len = 0;
        }
    }

    return false;  // Line not complete yet
}

bool ATCommandHndlr::handleCompleteLine(ResponseState& state, const ATCommand_t& atCmd) {
    // Null-terminate and remove trailing \r if present
    state.line_buffer[state.line_len] = '\0';
    if (state.line_len > 0 && state.line_buffer[state.line_len - 1] == '\r') {
        state.line_buffer[--state.line_len] = '\0';
    }

    // Skip empty lines
    if (state.line_len == 0) {
        return false;
    }

    // g_logger.info("RX: %s\n", state.line_buffer);

    // Check for ERROR response (both "ERROR" and "+CME ERROR")
    if (strcmp(state.line_buffer, "ERROR") == 0 || strstr(state.line_buffer, "+CME ERROR") != nullptr) {
        printSanitizedMsg("AT response error: ", state.line_buffer);
        printf("AT response error command: %s\n", atCmd.cmd);
        state.success = false;
        state.line_len = 0;
        return true;  // Done processing
    }

    // Check for expected response
    if (strstr(state.line_buffer, atCmd.expect) != nullptr) {
        state.got_expected = true;
        printf("Got expected response: %s\n", atCmd.expect);
    }

    // Check for OK response
    if (strcmp(state.line_buffer, "OK") == 0) {
        if (state.got_expected || atCmd.msg_type == MsgType::SHUTDOWN) {
            printf("AT OK: %s\n", atCmd.cmd);
            state.success = true;
        } else {
            printf("AT OK without expected response: %s (expected: %s)\n", 
                           atCmd.cmd, atCmd.expect);
            state.success = false;
        }
        state.line_len = 0;
        return true;  // Done processing
    }

    // Reset for next line
    state.line_len = 0;
    return false;  // Continue processing
}