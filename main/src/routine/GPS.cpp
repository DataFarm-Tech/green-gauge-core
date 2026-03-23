#include "GPS.hpp"
// #include "Logger.hpp"
#include "Utils.hpp"

#include <cstring>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

/**
 * @brief QGPS command to enable GPS functionality
 * This command activates the GPS receiver on the modem. The expected response is "OK" if
 * the command was successful. Enabling GPS is a prerequisite for acquiring location data, and this command should be sent before attempting to query for GPS coordinates.
 * Timeout: 2000ms
 * MsgType: STATUS (used for status and control commands)
 */
ATCommand_t cmd_enable = {
    "AT+QGPS=1",
    "OK",
    2000,
    MsgType::STATUS,
    nullptr, 
    0
};

/**
 * @brief QGPSLOC command to query GPS coordinates
 * This command requests the current GPS location from the modem. The expected response includes a line starting
 * with "+QGPSLOC:" followed by comma-separated values containing the GPS data (e.g., latitude, longitude, altitude, etc.). This command is used to retrieve the current GPS coordinates after the GPS receiver has been enabled and has acquired a fix.
 * Timeout: 5000ms (GPS queries may take longer due to satellite acquisition time)
 * MsgType: STATUS (used for status and control commands)
 */
ATCommand_t cmd = {
    "AT+QGPSLOC?",
    "+QGPSLOC:",
    5000,
    MsgType::STATUS,
    nullptr, 
    0
};

GPS::GPS()
{
}

bool GPS::getCoordinates(std::string &out)
{
    // First, ensure GPS is enabled before querying
    char resp[256] = {0};
    const int max_retries = 5;

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

    // Retry GPS query up to 5 times with 5-second delays between attempts
    // GPS module may take additional time to lock satellites
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
