idf_component_register(
    SRCS 
        "src/uspace/main.cpp"
        "src/net/WiFiConnection.cpp"
        "src/net/Communication.cpp"
        "src/net/SimConnection.cpp"
        "src/net/EthernetConnection.cpp"
        "src/uspace/BatteryPacket.cpp"
        "src/net/IPacket.cpp"
        "src/uspace/ReadingPacket.cpp"
        "src/uspace/ActivatePacket.cpp"
        "src/mem/EEPROMConfig.cpp"
        "src/uspace/Node.cpp"
        "src/uspace/CLI.cpp"
        "src/sys/UARTConsole.cpp"
        "src/sys/I2CDriver.cpp"
        "src/sys/OTAUpdater.cpp"
        "src/sys/Logger.cpp"
    INCLUDE_DIRS "include"
    REQUIRES 
        nvs_flash
        esp_event
        esp_wifi
        cbor
        littlefs
        esp_https_ota
        app_update
        esp_http_client
        driver
        esp_driver_uart
        esp_driver_gpio
    driver
)

# Enable all warnings for GCC/Clang
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall          # Enable all standard warnings
    -Wextra        # Extra warnings
)

set(CLI_EN 1)
set(OTA_EN 1)
set(DEEP_SLEEP_EN 1)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    CLI_EN=${CLI_EN}
    OTA_EN=${OTA_EN}
    DEEP_SLEEP_EN=${DEEP_SLEEP_EN}
)
