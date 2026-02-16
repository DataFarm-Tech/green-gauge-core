#!/usr/bin/env bash

set -e

# Get the latest git tag
PROJECT_VER=$(git describe --tags --abbrev=0)
echo "Current Git tag: $PROJECT_VER"

echo "=== Building firmware ==="
idf.py build -D CMAKE_VERBOSE_MAKEFILE=ON -DOTA_EN=0 -DDEEP_SLEEP_EN=0 -DPROJECT_VER="$PROJECT_VER"

# Path to the built firmware
BIN_FILE="build/firmware-idf.bin"
RENAMED_FILE="cn1.bin"

if [[ ! -f "$BIN_FILE" ]]; then
    echo "ERROR: $BIN_FILE not found!"
    exit 1
fi

# Display firmware binary details
echo ""
echo "=== Firmware Binary Details ==="
FW_SIZE=$(stat -c%s "$BIN_FILE" 2>/dev/null || stat -f%z "$BIN_FILE")
FW_SIZE_MB=$(awk "BEGIN {printf \"%.2f\", $FW_SIZE / 1048576}")

echo "Size:      $FW_SIZE_MB MB"

# Output directory
DEST_DIR="/firmware/dev/$USER/"

if [[ ! -d "$DEST_DIR" ]]; then
    echo "Creating $DEST_DIR ..."
    sudo mkdir -p "$DEST_DIR"
fi

##echo "Renaming firmware to $RENAMED_FILE ..."
##cp "$BIN_FILE" "$RENAMED_FILE"

##echo "Copying firmware to $DEST_DIR ..."
##sudo mv "$RENAMED_FILE" "$DEST_DIR/"

##echo "=== Done ==="
##echo "Firmware copied to $DEST_DIR/$RENAMED_FILE"
