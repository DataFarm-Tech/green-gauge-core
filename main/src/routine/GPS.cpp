#include "GPS.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

#include <cstring>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

GPS::GPS()
{
}

bool GPS::getCoordinates(std::string &out)
{
    // First, ensure GPS is enabled before querying
    ATCommand_t cmd_enable = {"AT+QGPS=1",
                              "OK",
                              2000,
                              MsgType::STATUS,
                              nullptr, 0};

    printf("GPS: Enabling GNSS receiver...\n");
    if (!m_hndlr.send(cmd_enable))
    {
        printf("Failed to enable GPS\n");
        // Don't return - still try to query in case GPS is already on
    }

    // GPS Cold Start acquisition can take 30-60+ seconds depending on signal and location
    // Wait substantial time before querying for fix
    printf("GPS: Waiting for satellite acquisition (this may take 30-60 seconds)...\n");
    vTaskDelay(pdMS_TO_TICKS(30000)); // 30 second initial wait for satellite search

    char resp[256] = {0};
    ATCommand_t cmd = {"AT+QGPSLOC?",
                       "+QGPSLOC:",
                       5000,
                       MsgType::STATUS,
                       nullptr, 0};

    // Retry GPS query up to 5 times with 5-second delays between attempts
    // GPS module may take additional time to lock satellites
    const int max_retries = 5;
    for (int attempt = 1; attempt <= max_retries; attempt++)
    {
        memset(resp, 0, sizeof(resp));

        if (m_hndlr.sendAndCapture(cmd, resp, sizeof(resp)))
        {
            std::string parsed = Utils::parseGPSLine(std::string(resp));
            if (!parsed.empty())
            {
                printf("GPS: fix acquired on attempt %d: %s\n", attempt, parsed.c_str());
                out = parsed;
                return true;
            }
            else
            {
                printf("GPS: parse failed on attempt %d for response: %s\n", attempt, resp);
            }
        }
        else
        {
            printf("GPS: no response on attempt %d\n", attempt);
        }

        if (attempt < max_retries)
        {
            vTaskDelay(pdMS_TO_TICKS(45000)); // 5-second delay between retries
        }
    }

    printf("GPS: failed to acquire fix after %d attempts (total wait: ~55 seconds)\n", max_retries);
    return false;
}
