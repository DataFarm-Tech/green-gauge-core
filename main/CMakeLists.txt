idf_component_register(
    SRCS 
        "src/main.cpp"
        "src/WiFiConnection.cpp"
        "src/Communication.cpp"
        "src/SimConnection.cpp"
        "src/EthernetConnection.cpp"
        "src/BatteryPacket.cpp"
        "src/IPacket.cpp"
    INCLUDE_DIRS "include"
    REQUIRES 
        nvs_flash
        esp_event
        esp_wifi
        cbor
        littlefs
)

set(DEEP_SLEEP_EN 0)

target_compile_definitions(${COMPONENT_LIB} PRIVATE DEEP_SLEEP_EN=${DEEP_SLEEP_EN})