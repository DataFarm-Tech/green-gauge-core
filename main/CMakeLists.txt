idf_component_register(
    SRCS 
        "src/main.cpp"
        "src/WiFiConnection.cpp"
        "src/Communication.cpp"
        "src/SimConnection.cpp"
        "src/EthernetConnection.cpp"
        "src/BatteryPacket.cpp"
        "src/IPacket.cpp"
        "src/ReadingPacket.cpp"
        "src/ActivatePacket.cpp"
        "src/EEPROMConfig.cpp"
        "src/I2CDriver.cpp"
        "src/Node.cpp"
        "src/CLI.cpp"
        "src/UARTConsole.cpp"
        "src/OTAUpdater.cpp"
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
    # -Wpedantic     # Strict ISO compliance
    # -Wconversion   # Warn on implicit type conversions
    # # -Wshadow       # Warn when local variables shadow others
    # -Wnon-virtual-dtor  # Warn if a class has virtual methods but no virtual destructor
)

set(CLI_EN 1)
set(OTA_EN 1)
set(DEEP_SLEEP_EN 1)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    CLI_EN=${CLI_EN}
    OTA_EN=${OTA_EN}
    DEEP_SLEEP_EN=${DEEP_SLEEP_EN}
)
