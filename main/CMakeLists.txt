idf_component_register(
    SRCS 
        "src/main.cpp"
        "src/net/WiFiConnection.cpp"
        "src/net/Communication.cpp"
        "src/net/SimConnection.cpp"
        "src/net/EthernetConnection.cpp"
        "src/mem/EEPROMConfig.cpp"
        # "src/shell/CLI.cpp"
        # "src/shell/Commands.cpp"
        "src/sys/UARTDriver.cpp"
        "src/sys/OTAUpdater.cpp"
        "src/sys/Logger.cpp"
        "src/io/NPK.cpp"
        "src/net/ActivatePkt.cpp"
        "src/sys/ATCommandHndlr.cpp"
        "src/net/ReadingPkt.cpp"
        "src/io/CoapPktAssm.cpp"
        "src/net/GPS.cpp"
        "src/other/Utils.cpp"
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
    # -Wconversion
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