idf_component_register(
    SRCS 
        "src/main.cpp"
        "src/WiFiConnection.cpp"
        "src/Communication.cpp"
        "src/SimConnection.cpp"
        "src/EthernetConnection.cpp"
        "src/BatteryPacket.cpp"
        "src/IPacket.cpp"
        # "src/OTAUpdater.cpp"
        "src/ReadingPacket.cpp"
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
)

set(DEEP_SLEEP_EN 1)
target_compile_definitions(${COMPONENT_LIB} PRIVATE DEEP_SLEEP_EN=${DEEP_SLEEP_EN})

set(DHCP_EN 1)
target_compile_definitions(${COMPONENT_LIB} PRIVATE DHCP_EN=${DHCP_EN})