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
)

# # Enable all warnings for GCC/Clang
# target_compile_options(${COMPONENT_LIB} PRIVATE
#     -Wall          # Enable all standard warnings
#     -Wextra        # Extra warnings
#     -Wpedantic     # Strict ISO compliance
#     -Wconversion   # Warn on implicit type conversions
#     -Wshadow       # Warn when local variables shadow others
#     -Wnon-virtual-dtor  # Warn if a class has virtual methods but no virtual destructor
# )

set(DEEP_SLEEP_EN 1)
target_compile_definitions(${COMPONENT_LIB} PRIVATE DEEP_SLEEP_EN=${DEEP_SLEEP_EN})

set(DHCP_EN 1)
target_compile_definitions(${COMPONENT_LIB} PRIVATE DHCP_EN=${DHCP_EN})

set(ACT_EN 1)
target_compile_definitions(${COMPONENT_LIB} PRIVATE ACT_EN=${ACT_EN})