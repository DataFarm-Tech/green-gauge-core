idf_component_register(
    SRCS 
        "src/main.cpp"
        "src/WiFiConnection.cpp"
        "src/Communication.cpp"
        "src/SimConnection.cpp"
        "src/Packet.cpp"
        "src/EthernetConnection.cpp"
    INCLUDE_DIRS "include"
    REQUIRES 
        nvs_flash
        esp_event
        esp_wifi
        cbor
        littlefs
)
