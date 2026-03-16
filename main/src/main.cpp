#include "AppRuntime.hpp"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
}

#define APP_RUNTIME_STACK_SIZE 8192

/**
 * @brief Main application entry point.
 */
extern "C" void app_main(void)
{
    reset_reason = esp_reset_reason();
    wakeup_causes = esp_sleep_get_wakeup_causes();

    // Step 2: Load or create configuration
    if (!load_create_config())
    {
        printf("Config load failed, using deep sleep fallback\n");
        enter_deep_sleep();
        return;
    }

    // Step 3: Select communication features based on loaded config
    hw_features();

    // Step 4: Initialize hardware for selected communication mode
    init_hw();

    // Step 5: Launch provisioning + operational tasks in background
    xTaskCreate(start_app, "start_app", APP_RUNTIME_STACK_SIZE, nullptr, 5, nullptr);

    return;
}