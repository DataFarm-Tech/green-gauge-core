#!/usr/bin/env bash

set -euo pipefail

PORT="${PORT:-/dev/ttyACM0}"
BAUD="${BAUD:-115200}"
NVS_BIN="${NVS_BIN:-manf-info/NT0052/NT0052.bin}"

if [ ! -f "build/df-firmware.bin" ]; then
		echo "Missing build/df-firmware.bin. Run idf.py build first."
		exit 1
fi

if [ ! -f "$NVS_BIN" ]; then
		echo "Missing NVS binary: $NVS_BIN"
		exit 1
fi

echo "Erasing flash on $PORT ..."
esptool.py --chip esp32s3 --port "$PORT" erase_flash

echo "Flashing bootloader, partition table, OTA data, and BOTH app slots ..."
esptool.py --chip esp32s3 --port "$PORT" -b "$BAUD" --before default-reset --after hard-reset write_flash \
	--flash-mode dio --flash-freq 80m --flash-size 8MB \
	0x0 build/bootloader/bootloader.bin \
	0x8000 build/partition_table/partition-table.bin \
	0x10000 build/ota_data_initial.bin \
	0x20000 build/df-firmware.bin \
	0x220000 build/df-firmware.bin

echo "Writing provisioning NVS blob ..."
esptool.py --chip esp32s3 --port "$PORT" -b "$BAUD" --before default-reset --after hard-reset write_flash \
	0x9000 "$NVS_BIN"

echo "Flash complete."
