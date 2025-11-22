#!/usr/bin/env bash

set -e

echo "=== Building firmware ==="
idf.py build

# Path to the built firmware
BIN_FILE="build/firmware-idf.bin"
RENAMED_FILE="cn1.bin"

if [[ ! -f "$BIN_FILE" ]]; then
    echo "ERROR: $BIN_FILE not found!"
    exit 1
fi

# Output directory
DEST_DIR="/firmware/dev/$USER/"

if [[ ! -d "$DEST_DIR" ]]; then
    echo "Creating $DEST_DIR ..."
    sudo mkdir -p "$DEST_DIR"
fi

echo "Renaming firmware to $RENAMED_FILE ..."
cp "$BIN_FILE" "$RENAMED_FILE"

echo "Copying firmware to $DEST_DIR ..."
sudo mv "$RENAMED_FILE" "$DEST_DIR/"

echo "=== Done ==="
echo "Firmware copied to $DEST_DIR/$RENAMED_FILE"
