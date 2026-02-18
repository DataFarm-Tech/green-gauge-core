idf_component_register(
    SRCS 
        "src/main.cpp"
        "src/io/drivers/UARTDriver.cpp"
        "src/io/drivers/EEPROMConfig.cpp"
        "src/io/interfaces/ATCommandHndlr.cpp"
        "src/net/adapters/Communication.cpp"
        "src/net/adapters/EthernetConnection.cpp"
        "src/net/adapters/SimConnection.cpp"
        "src/net/adapters/WiFiConnection.cpp"
        "src/net/cbor_pkt_build/ActivatePkt.cpp"
        "src/net/cbor_pkt_build/ReadingPkt.cpp"
        "src/net/cbor_pkt_build/GpsUpdatePkt.cpp"
        "src/net/cbor_pkt_build/Key.cpp"
        "src/net/coap_pkt_build/CoapPktAssm.cpp"
        "src/routine/NPK.cpp"
        "src/routine/GPS.cpp"
        "src/sys/OTAUpdater.cpp"
        "src/sys/Logger.cpp"
        "src/other/utils.cpp"
    INCLUDE_DIRS "include"
    REQUIRES 
        nvs_flash
        esp_event
        esp_wifi
        cbor
        esp_driver_usb_serial_jtag
        littlefs
        esp_https_ota
        app_update
        esp_http_client
        driver
        esp_driver_uart
        esp_driver_gpio
        esp_netif 
        lwip
        datafarm__cli
)

target_include_directories(${COMPONENT_LIB}
    SYSTEM PRIVATE
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wconversion
    -Wdouble-promotion
    -Wformat=2
    -Wformat-truncation
    -Wformat-overflow
    -Wlogical-op
    -Wnull-dereference
    -Wduplicated-cond
    -Wduplicated-branches
    -Wimplicit-fallthrough
    -Wstack-usage=4096
    -Wvla
)

# Default values (can be overridden from command line)
set(OTA_EN 1 CACHE STRING "Enable OTA")
set(DEEP_SLEEP_EN 1 CACHE STRING "Enable Deep Sleep")
set(PROJECT_VER "unknown" CACHE STRING "Firmware version")

# Apply compile definitions
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    OTA_EN=${OTA_EN}
    DEEP_SLEEP_EN=${DEEP_SLEEP_EN}
    PROJECT_VER="${PROJECT_VER}"
)