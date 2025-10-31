#include <iostream>
#include "Communication.hpp"
#include "esp_log.h"
#include "Config.hpp"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "nvs_flash.h"
    #include "esp_sleep.h"
}

#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <string.h>
#include <vector>
#include "Packet.hpp"
#include "esp_littlefs.h"


constexpr int sleep_time_sec = 6 * 60 * 60;
static const char *TAG = "LITTLEFS_DEMO";

// esp_vfs_littlefs_conf_t conf = {
//         .base_path = "/littlefs",
//         .partition_label = "littlefs",
//         .format_if_mount_failed = true,
//         .dont_mount = false,
// };


// // Unmount the LittleFS partition
// void unmount_fs(void) {
//     esp_vfs_littlefs_unregister(conf.partition_label);
//     ESP_LOGI(TAG, "LittleFS unmounted");
// }

// // Mount the LittleFS partition
// void mount_fs(void) {
//     esp_err_t ret = esp_vfs_littlefs_register(&conf);

//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to mount LittleFS (%s)", esp_err_to_name(ret));
//         return;
//     }

//     size_t total = 0, used = 0;
//     ret = esp_littlefs_info(conf.partition_label, &total, &used);
//     if (ret == ESP_OK) {
//         ESP_LOGI(TAG, "LittleFS mounted: total=%u, used=%u", total, used);
//     } else {
//         ESP_LOGW(TAG, "Failed to get LittleFS partition info (%s)", esp_err_to_name(ret));
//     }
// }

// // Write text to a file
// esp_err_t write_file(const char *file_path, const char *message) {
//     if (!file_path || !message) {
//         ESP_LOGE(TAG, "Invalid arguments");
//         return ESP_ERR_INVALID_ARG;
//     }

//     FILE *f = fopen(file_path, "w");
//     if (!f) {
//         ESP_LOGE(TAG, "Failed to open file for writing: %s", file_path);
//         return ESP_FAIL;
//     }

//     size_t msg_len = strlen(message);
//     size_t bytes_written = fwrite(message, 1, msg_len, f);
//     fflush(f);  // ensure data is committed
//     fclose(f);

//     if (bytes_written != msg_len) {
//         ESP_LOGE(TAG, "Write error: wrote %zu of %zu bytes", bytes_written, msg_len);
//         return ESP_FAIL;
//     }

//     ESP_LOGI(TAG, "File written successfully: %s (%zu bytes)", file_path, bytes_written);
//     return ESP_OK;
// }

// // Read text from a file
// esp_err_t read_file(const char *file_path) {
//     if (!file_path) {
//         ESP_LOGE(TAG, "Invalid argument: file_path is NULL");
//         return ESP_ERR_INVALID_ARG;
//     }

//     FILE *f = fopen(file_path, "r");
//     if (!f) {
//         ESP_LOGE(TAG, "Failed to open file for reading: %s", file_path);
//         return ESP_FAIL;
//     }

//     char buffer[128];
//     ESP_LOGI(TAG, "Contents of %s:", file_path);

//     while (fgets(buffer, sizeof(buffer), f)) {
//         ESP_LOGI(TAG, "%s", buffer);
//     }

//     fclose(f);
//     return ESP_OK;
// }

void init_nvs()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(ret);
}

extern "C" void app_main(void) 
{
    // // File path
    // const char *file_path = "/littlefs/hello.txt";

    // mount_fs();

    // write_file("/littlefs/hello.txt", "HelloWorld");
    // read_file("/littlefs/hello.txt");

    // unmount_fs();


    init_nvs();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    Communication comm(ConnectionType::WIFI);
    
    if (!comm.connect()) 
    {
        ESP_LOGE("MAIN", "Failed to connect.");
    }
    else
    {
        ESP_LOGI("MAIN", "Connected.");

        Packet pkt;
        if (pkt.createPayload(23, 55)) 
        {
            pkt.sendCoAP();
        } 

        if (comm.isConnected()) 
        {
            ESP_LOGI("MAIN", "Still connected. Disconnecting...");
            comm.disconnect();
        }
    }

    ESP_LOGI("MAIN", "Going into deep sleep for %d seconds...", sleep_time_sec);
    esp_sleep_enable_timer_wakeup(sleep_time_sec * 1000000ULL);  // Convert to microseconds
    esp_deep_sleep_start();
}
