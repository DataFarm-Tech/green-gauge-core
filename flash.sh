#!/bin/bash

# Function to display help text
function display_help {
    echo "Usage: $0 [flash | compile | clean] [port (optional for flash)]"
    echo
    echo "This script automates operations for building and flashing firmware on datafarm devices using PlatformIO."
    echo
    echo "Arguments:"
    echo "  flash    : Performs the flash operation to upload the firmware to the ESP32 device."
    echo "             Optionally provide a port name (e.g., /dev/ttyUSB0) as a second argument."
    echo "  compile  : Compiles the project without uploading to the device."
    echo "  clean    : Cleans the build environment (removes temporary and build files)."
    echo
    echo "Examples:"
    echo "  $0 flash             : Flash and monitor using default port (/dev/ttyUSB0)."
    echo "  $0 flash /dev/ttyUSB1: Flash and monitor using specified port."
    echo "  $0 compile           : Compile the project without uploading."
    echo "  $0 clean             : Clean the build environment."
    echo
    echo "Note: To configure the upload_port, baud_rate and libs, configure the platformio.ini file."
    exit 1
}

# Check if argument count is invalid
if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    display_help
fi

# Get the first argument
action=$1
port=${2:-/dev/ttyUSB0}  # Default port if none specified

# Perform actions
case "$action" in
    flash)
        echo "Performing flash operation..."
        sudo pio run -e flash --target upload
        echo "Opening serial monitor on port: $port"
        sudo pio device monitor -b 115200 -p "$port"
        ;;

    compile)
        echo "Performing compile operation..."
        sudo pio run -e compile
        ;;

    clean)
        echo "Cleaning build environment..."
        sudo pio run --target clean
        ;;
    esp32_test)
        sudo pio run -e esp32s3_test --target upload
        sudo pio device monitor -b 115200 -p /dev/ttyACM0
        ;;

    *)
        echo "Invalid action: $action"
        display_help
        ;;
esac
