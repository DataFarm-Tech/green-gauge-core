#!/bin/bash

# Function to display help text
function display_help {
    echo "Usage: $0 [flash | compile | clean]"
    echo
    echo "This script automates operations for building and flashing firmware on datafarm devices using PlatformIO."
    echo
    echo "Arguments:"
    echo "  flash    : Performs the flash operation to upload the firmware to the ESP32 device."
    echo "  compile  : Compiles the project without uploading to the device."
    echo "  clean    : Cleans the build environment (removes temporary and build files)."
    echo
    echo "Examples:"
    echo "  $0 flash   : Flash the firmware to the ESP32."
    echo "  $0 compile : Compile the project without uploading."
    echo "  $0 clean   : Clean the build environment."
    echo
    echo "If no argument is provided, or more than one, the script will exit with a usage message."
    echo "Note: To configure the upload_port, baud_rate and libs. Configure the platformio.ini file."
    exit 1
}

# Check if the number of arguments is exactly 2
if [ $# -ne 1 ]; then
    display_help
fi

# Get the first argument
action=$1

# Check if the action is "flash", "compile", or "clean"
if [ "$action" == "flash" ]; then
    echo "Performing flash operation..."
    sudo pio run -e flash --target upload
    sudo pio device monitor -b 115200 -p /dev/ttyUSB0

elif [ "$action" == "compile" ]; then
    echo "Performing compile operation..."
    sudo pio run -e compile

elif [ "$action" == "clean" ]; then
    echo "Cleaning build environment..."
    sudo pio run -e compile --target clean

else
    echo "Invalid action: $action"
    display_help
fi
