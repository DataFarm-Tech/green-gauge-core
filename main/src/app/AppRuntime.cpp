#include "AppRuntime.hpp"

#include <cctype>
#include <stdio.h>
#include <string.h>

#include "ActivatePkt.hpp"
#include "CoapOTAUpdater.hpp"
#include "CoapPktAssm.hpp"
#include "Config.hpp"
#include "HwTypes.hpp"
#include "Key.hpp"
// #include "Logger.hpp"
#include "NPK.hpp"
#include "ReadingPkt.hpp"
#include "UARTDriver.hpp"
#include "GpsUpdatePkt.hpp"
#include "Utils.hpp"

extern "C" {
    #include "driver/uart.h"
    #include "esp_event.h"
    #include "esp_netif.h"
    #include "esp_log.h"
    #include "esp_sleep.h"
    #include "esp_wifi.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

DeviceConfig g_device_config;
uint32_t wakeup_causes = 0;
esp_reset_reason_t reset_reason;

std::unique_ptr<Communication> g_comm;
ConnectionType g_hw_var = ConnectionType::SIM;
GPS m_gps;

static const DeviceConfig k_default_device_config = {
    .has_activated = false,
    .gps_coord = "",
    .main_app_delay = 30,
    .session_count = 0,
    .secretKey = "",
    .manf_info = {
        .hw_ver = {.value = ""},
        .hw_var = {.value = ""},
        .fw_ver = {.value = ""},
        .nodeId = {.value = ""},
        .secretkey = {.value = ""},
        .p_code = {.value = ""},
        .sim_mod_sn = {.value = ""},
        .sim_card_sn = {.value = ""},
        .chassis_ver = {.value = ""}
    },
    .calib = {.calib_list = {{.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Nitrogen}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Phosphorus}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Potassium}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Moisture}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::PH}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Temperature}}, .last_cal_ts = 0}};

void enter_deep_sleep()
{
    const uint64_t sleep_seconds = static_cast<uint64_t>(g_device_config.main_app_delay);
    const uint64_t safe_sleep_seconds = (sleep_seconds > 0ULL) ? sleep_seconds : 60ULL;

    printf("Entering deep sleep for %llu seconds\n",
           static_cast<unsigned long long>(safe_sleep_seconds));

    if (g_comm)
    {
        g_comm->disconnect();
    }

    // g_logger.deinit();
    esp_sleep_enable_timer_wakeup(safe_sleep_seconds * 1000000ULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_deep_sleep_start();
}

void init_hw()
{
    // g_logger.init();

    // Required for components that depend on lwIP/tcpip task (e.g. esp_http_client).
    // Re-initialization returns ESP_ERR_INVALID_STATE, which is safe to ignore.
    esp_err_t netif_err = esp_netif_init();
    if (netif_err != ESP_OK && netif_err != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(netif_err);
    }

    esp_err_t event_loop_err = esp_event_loop_create_default();
    if (event_loop_err != ESP_OK && event_loop_err != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(event_loop_err);
    }

    rs485_uart.init(
        BAUD_4800,
        GPIO_NUM_38,
        GPIO_NUM_37,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE,
        GEN_BUFFER_SIZE,
        GEN_BUFFER_SIZE
    );

    switch (g_hw_var)
    {
    case ConnectionType::WIFI:
        break;
    case ConnectionType::SIM:
        m_modem_uart.init(
            BAUD_115200,
            GPIO_NUM_17,
            GPIO_NUM_18,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE,
            2048,
            0
        );
        break;

    default:
        break;
    }
}

bool load_create_config()
{
    if (!eeprom.begin())
    {
        printf("Failed to initialize EEPROMConfig\n");
        return false;
    }

    g_device_config = k_default_device_config;

    if (eeprom.loadConfig(g_device_config))
    {
        Utils::printDeviceConfig(g_device_config, "loaded from NVS");
        return true;
    }

    printf("Failed to load valid provisioning from NVS - using runtime defaults (not persisted)\n");
    g_device_config = k_default_device_config;

    Utils::printDeviceConfig(g_device_config, "defaults");

    return true;
}

void net_select(HwVer_e hw_ver)
{
    switch (hw_ver)
    {
    case HW_VER_0_0_1_E:
        g_hw_var = ConnectionType::SIM;
        break;
    case HW_VER_0_0_2_E:
        g_hw_var = ConnectionType::SIM;
        break;

    default:
        g_hw_var = ConnectionType::SIM;
        break;
    }

    g_comm = std::make_unique<Communication>(g_hw_var);
}

void hw_features(void)
{
    HwVer_e hw_ver = HW_VER_UNKNOWN_E;

    for (const auto& entry : ver)
    {
        if (entry.name == nullptr)
        {
            break;
        }

        if (strcmp(entry.name, g_device_config.manf_info.hw_ver.value) == 0)
        {
            hw_ver = entry.hw_ver;
            break;
        }
    }

    net_select(hw_ver);
}

static void read_gps()
{
    const int gps_cold_start_delay_ms = 30000;
    printf("Waiting for GPS fix (Cold Start may take 30-60 seconds)...\n");
    vTaskDelay(pdMS_TO_TICKS(gps_cold_start_delay_ms));

    if (m_gps.getCoordinates(g_device_config.gps_coord))
    {
        printf("GPS location retrieved: %s\n", g_device_config.gps_coord.c_str());
    }
    else
    {
        g_device_config.gps_coord = "0.0,0.0";
        printf("Failed to retrieve GPS location, using default coordinates: %s\n", g_device_config.gps_coord.c_str());
    }
}

static void handle_gps_update()
{
    GpsUpdatePkt gpsupdatePkt(PktType::GpsUpdate, std::string(g_device_config.manf_info.nodeId.value), std::string(GPS_URI), g_device_config.gps_coord);

    const uint8_t* pkt_1 = gpsupdatePkt.toBuffer();
    const size_t buffer_len = gpsupdatePkt.getBufferLength();

    if (!pkt_1 || buffer_len == 0)
    {
        printf("Failed to build GPS update packet for node: %s\n", g_device_config.manf_info.nodeId.value);
        return;
    }

    if (!g_comm->sendPacket(pkt_1, buffer_len, gpsupdate_entry))
    {
        printf("Sending activation packet failed for node: %s\n", g_device_config.manf_info.nodeId.value);
        return;
    }
}

static void handle_activation()
{
    Key::computeKey(g_device_config.secretKey, Key::HMAC_SIZE);
    
    ActivatePkt activatePkt(PktType::Activate, std::string(g_device_config.manf_info.nodeId.value),
                            std::string(ACT_URI), std::string(g_device_config.manf_info.secretkey.value), g_device_config.gps_coord, g_device_config.manf_info.hw_ver.value,
                            g_device_config.manf_info.sim_mod_sn.value, g_device_config.manf_info.sim_card_sn.value,
                            g_device_config.manf_info.chassis_ver.value);

    const uint8_t* pkt_1 = activatePkt.toBuffer();
    const size_t buffer_len = activatePkt.getBufferLength();

    if (g_device_config.has_activated)
    {
        printf("Device already activated (node: %s)\n", g_device_config.manf_info.nodeId.value);
        return;
    }

    printf("Sending activation packet for node: %s\n", g_device_config.manf_info.nodeId.value);


    if (!pkt_1 || buffer_len == 0)
    {
        printf("Failed to build activation packet for node: %s\n", g_device_config.manf_info.nodeId.value);
        return;
    }

    if (!g_comm->sendPacket(pkt_1, buffer_len, activate_entry))
    {
        printf("Sending activation packet failed for node: %s\n", g_device_config.manf_info.nodeId.value);
        return;
    }

    g_device_config.has_activated = true;

    if (eeprom.saveConfig(g_device_config))
    {
        printf("Activation complete, config saved to EEPROM\n");
    }
    else
    {
        printf("Failed to save activation status to EEPROM\n");
    }
}

static void collect_reading()
{
    uint16_t reading[NPK_COLLECT_SIZE];

    for (const NPK::MeasurementEntry& m_entry : NPK::MEASUREMENT_TABLE)
    {
        memset(reading, 0, sizeof(reading));

        if (!NPK::npk_collect(m_entry, reading))
        {
            printf("Failed to collect measurement type %d\n",
                   static_cast<int>(m_entry.type));
            continue;
        }

        if (!(g_device_config.session_count < UINT64_MAX))
        {
            printf("Error: session_count would overflow!\n");
            continue;
        }

        ReadingPkt readingPkt(PktType::Reading,
                              std::string(g_device_config.manf_info.nodeId.value),
                              std::string(DATA_URI),
                              reading,
                              m_entry.type,
                              g_device_config.session_count);

        const uint8_t* cbor_buffer = readingPkt.toBuffer();
        const size_t cbor_buffer_len = readingPkt.getBufferLength();

        if (!cbor_buffer || cbor_buffer_len == 0)
        {
            printf("Failed to build measurement packet type %d\n", static_cast<int>(m_entry.type));
            continue;
        }

        if (g_comm->sendPacket(cbor_buffer, cbor_buffer_len, reading_entry))
        {
            printf("Sent measurement type %d successfully\n", static_cast<int>(m_entry.type));
        }
        else
        {
            printf("Failed to send measurement type %d, queuing to file\n", static_cast<int>(m_entry.type));
        }
    }

    g_device_config.session_count++;

    if (eeprom.saveConfig(g_device_config))
    {
        printf("Readings complete, session saved to EEPROM\n");
    }
    else
    {
        printf("Failed to save session to EEPROM\n");
    }

    printf("Collection complete\n");
}

static bool has_required_identity_fields()
{
    return (g_device_config.manf_info.nodeId.value[0] != '\0') &&
           (g_device_config.manf_info.secretkey.value[0] != '\0') &&
           (g_device_config.manf_info.hw_ver.value[0] != '\0');
}

static bool is_valid_node_id(const char* node_id)
{
    if (node_id == nullptr)
    {
        return false;
    }

    const size_t len = strlen(node_id);
    if (len != 6)
    {
        return false;
    }

    for (size_t i = 0; i < len; ++i)
    {
        if (!std::isalnum(static_cast<unsigned char>(node_id[i])))
        {
            return false;
        }
    }

    return true;
}

// static bool activation_payload_ready()
// {
//     return is_valid_node_id(g_device_config.manf_info.nodeId.value) &&
//            (g_device_config.manf_info.secretkey.value[0] != '\0') &&
//            (g_device_config.manf_info.hw_ver.value[0] != '\0') &&
//            (g_device_config.manf_info.fw_ver.value[0] != '\0') &&
//            (g_device_config.manf_info.sim_mod_sn.value[0] != '\0') &&
//            (g_device_config.manf_info.sim_card_sn.value[0] != '\0') &&
//            (g_device_config.manf_info.chassis_ver.value[0] != '\0');
// }

void start_app(void* arg)
{
    bool connected_for_collection = false;
    bool identity_ready = false;

    if (!g_comm)
    {
        printf("Communication not initialized\n");
        goto cleanup;
    }

    if (!g_comm->connect()) //If connection fails, enter deep sleep.
    {
        printf("Unable to connect to network\n");
        enter_deep_sleep();
        return;
    }

    printf("Device connected to network\n");

#if OTA_EN == 1
    if (g_comm->isConnected())
    {
        CoapOTAUpdater ota(*g_comm, g_device_config.manf_info.fw_ver.value);

        if (ota.isFirmwareAvailable()) {
            printf("Firmware update detected, starting OTA process\n");

            if (ota.executeUpdate())
            {
                printf("OTA image ready, rebooting into updated firmware\n");
                vTaskDelay(pdMS_TO_TICKS(1500));
                esp_restart();
                    // if (!activation_payload_ready())
                    // {
                    //     printf("Activation skipped: manufacturing identity not fully provisioned or node_id invalid\n");
                    //     printf("nodeId='%s' (must be 6 alnum), hw_ver='%s', fw_ver='%s', sim_mod_sn='%s', sim_card_sn='%s', chassis_ver='%s'\n",
                    //            g_device_config.manf_info.nodeId.value,
                    //            g_device_config.manf_info.hw_ver.value,
                    //            g_device_config.manf_info.fw_ver.value,
                    //            g_device_config.manf_info.sim_mod_sn.value,
                    //            g_device_config.manf_info.sim_card_sn.value,
                    //            g_device_config.manf_info.chassis_ver.value);
                    //     return;
                    // }

            }
            else
            {
                printf("OTA download/write failed\n");
            }
        }
        else
        {
            printf("OTA check skipped: no update available or timeout/no response\n");
        }
    }
#endif

    read_gps();

    if (g_device_config.gps_coord.empty()) {
        printf("No GPS coordinates available to send in update packet\n");
        goto cleanup;
    }

    identity_ready = has_required_identity_fields();
    if (g_device_config.has_activated && !identity_ready)
    {
        // Recover from inconsistent persisted state (e.g. stale/partial NVS data).
        printf("Activation state inconsistent (has_activated=true but identity fields missing), forcing re-activation\n");
        g_device_config.has_activated = false;
        if (!eeprom.saveConfig(g_device_config))
        {
            printf("Warning: failed to persist corrected activation state\n");
        }
    }

    printf("Activation gate: has_activated=%s, nodeId='%s', hw_ver='%s'\n",
           g_device_config.has_activated ? "true" : "false",
           g_device_config.manf_info.nodeId.value,
           g_device_config.manf_info.hw_ver.value);

    if (!g_device_config.has_activated) {
        handle_activation();
    }
    else {
        handle_gps_update();
    }

#if TELNET_CLI_EN == 1
    if (!g_comm->startTelnetSession())
    {
        printf("Warning: failed to start telnet session\n");
    }
#endif

    connected_for_collection = g_comm->isConnected();
    if (connected_for_collection)
    {
        printf("Connection check passed, starting collect_reading()\n");
        collect_reading();
        goto cleanup;
    }
    else
    {
        printf("Connection check failed before collect_reading(), skipping collection this cycle\n");
        goto cleanup;
    }

cleanup:
    enter_deep_sleep();
    vTaskDelete(nullptr);
}
