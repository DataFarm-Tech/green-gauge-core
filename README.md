# Green Gauge Core

ESP32-S3 firmware for the Green Gauge soil monitoring device. Reads NPK, moisture, pH, and temperature via an RS485 sensor, transmits data over CoAP/CBOR on a cellular connection, and supports OTA updates and deep-sleep power cycling.

## Table of Contents

**Getting Started**
- [Requirements](#requirements)
- [Building](#building)
- [Flashing](#flashing)

**Project Details**
- [Partition Layout](#partition-layout)
- [Manufacturing NVS (manf-info)](#manufacturing-nvs-manf-info)
- [OTA Update Flow](#ota-update-flow)
- [Project Structure](#project-structure)
- [CLI](#cli)
- [Deployment](#deployment)

---

## Requirements

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/) — tested with the `espressif/idf:latest` Docker image
- Python 3 + `esptool.py` (for flashing)
- A supported device with a provisioned NVS manufacturing binary

---

## Building

```bash
./build.sh
```

The build script reads the latest git tag as the firmware version, compiles with OTA enabled, and outputs `build/df-firmware.bin`.

To build manually with custom flags:

```bash
. $IDF_PATH/export.sh
idf.py build -DOTA_EN=1 -DDEEP_SLEEP_EN=1 -DTELNET_CLI_EN=1 -DPROJECT_VER="0.0.1"
```

| Build flag | Default | Description |
| --- | --- | --- |
| `OTA_EN` | `1` | Enable OTA update support |
| `DEEP_SLEEP_EN` | `1` | Enable deep-sleep power cycling |
| `TELNET_CLI_EN` | `1` | Enable remote telnet CLI session |
| `PROJECT_VER` | `unknown` | Firmware version string embedded in the binary |

---

## Flashing

```bash
./flash.sh [OPTIONS]
```

| Flag | Default | Description |
| --- | --- | --- |
| `-p PORT` | `/dev/ttyACM0` | Serial port |
| `-b BAUD` | `115200` | Baud rate |
| `-d BUILD_DIR` | `build` | Path to the build output directory |
| `-n NVS_BIN` | `manf-info/NT0052/NT0052.bin` | Path to the manufacturing NVS binary |

**Example:**

```bash
./flash.sh -p /dev/ttyUSB0 -b 460800 -d build -n manf-info/NT0097/NT0097.bin
```

The script erases the entire flash, then writes the bootloader, partition table, OTA data, manufacturing NVS, and the application binary into both OTA slots.

### Manufacturing NVS binaries

Pre-built NVS binaries for known hardware variants live under `manf-info/`:

```
manf-info/
  NT0052/
  NT0067/
  NT0097/
```

---

## Partition Layout

| Name | Type | Offset | Size |
| --- | --- | --- | --- |
| `nvs` | NVS | 0x9000 | 24 KB |
| `phy_init` | PHY | 0xF000 | 4 KB |
| `ota_data` | OTA data | 0x10000 | 8 KB |
| `ota_0` | App (OTA slot 0) | 0x20000 | 2 MB |
| `ota_1` | App (OTA slot 1) | 0x220000 | 2 MB |
| `littlefs` | LittleFS | 0x420000 | ~4 MB |

---

## Manufacturing NVS (`manf-info`)

Each device is provisioned with a manufacturing NVS binary that is flashed separately from the application. The source for each SKU lives in `manf-info/<NODE_ID>/` as a CSV and a compiled `.bin`.

To regenerate a binary from its CSV:

```bash
cd manf-info
./build.sh NT0052
```

### NVS fields

| Key | Type | Description |
| --- | --- | --- |
| `hw_ver` | string | Hardware version (e.g. `0.0.1`) |
| `hw_var` | string | Hardware variant (e.g. `A`) |
| `fw_ver` | string | Firmware version at provisioning time |
| `node_id` | string | Unique node identifier (e.g. `NT0052`) |
| `sim_mod_sn` | string | SIM module serial number |
| `sim_card_sn` | string | SIM card serial number |
| `chassis_ver` | string | Chassis version |
| `secret_key` | string | Device secret key used during activation |
| `p_code` | string | Product code (e.g. `CN001-SN001`) |
| `has_activated` | u8 | `0` = not yet activated, `1` = activated |
| `main_app_delay` | u32 | Deep-sleep wake interval in seconds |
| `session_count` | u64 | Incremented each measurement cycle |
| `hmac_key` | hex2bin | 32-byte HMAC key |
| `cal_<x>_offset` | string | Calibration offset per sensor channel (`n`, `p`, `k`, `m`, `ph`, `t`) |
| `cal_<x>_gain` | string | Calibration gain per sensor channel |
| `last_cal_ts` | u32 | Unix timestamp of last calibration |

---

## OTA Update Flow

On every boot (when `OTA_EN=1`), the device checks for a firmware update immediately after connecting to the network and before taking any sensor readings.

```
Boot
 └─ Connect to network
     └─ GET /firmware-version  (CoAP)
         ├─ version <= current  →  skip, continue to sensor loop
         └─ version > current   →  GET /firmware-download  (CoAP)
             └─ HTTPS stream binary → OTA partition
                 ├─ success  →  set boot partition, persist fw_ver to NVS, reboot
                 └─ failure  →  abort OTA, continue to sensor loop
```

### Steps in detail

1. **Version check** — sends a CoAP GET to `/firmware-version`. The server responds with a CBOR-encoded version string. The device compares it segment-by-segment against `fw_ver` stored in NVS using semantic versioning (`MAJOR.MINOR.PATCH`).
2. **Download URL** — if a newer version is available, sends a CoAP GET to `/firmware-download`. The server responds with a CBOR-encoded HTTPS URL pointing to the binary.
3. **Streaming write** — the binary is streamed in chunks directly from HTTPS into the inactive OTA partition using `esp_ota_write`. On SIM connections the modem's native HTTPS transport is used; Wi-Fi falls back to `esp_http_client`.
4. **Finalise** — calls `esp_ota_end` and `esp_ota_set_boot_partition` to mark the new partition as active, persists the new `fw_ver` to NVS, and reboots.
5. **Rollback** — if the stream fails at any point, `esp_ota_abort` is called and the device continues with the current firmware.

### OTA CoAP endpoints

| Endpoint | Method | Description |
| --- | --- | --- |
| `/firmware-version` | GET | Returns CBOR-encoded latest available version string |
| `/firmware-download` | GET | Returns CBOR-encoded HTTPS URL for the firmware binary |

### Disabling OTA

OTA can be disabled at build time:

```bash
idf.py build -DOTA_EN=0
```

---

## Project Structure

```
main/
  include/       Header files
  src/
    app/         Application runtime and entry point
    io/          UART and EEPROM drivers
    net/         Network adapters (Wi-Fi, SIM, Ethernet) and CoAP/CBOR packet builders
    routine/     Sensor routines (NPK, GPS)
    sys/         OTA updater, logger, CBOR decoder
    shell/       UART CLI commands
    other/       Utilities
components/      Vendored components
manf-info/       Per-SKU manufacturing NVS binaries
```

---

## CLI

Connect over USB serial to access the interactive CLI:

| Command | Description |
| --- | --- |
| `help` | List available commands |
| `version` | Show firmware version |
| `reset` | Reboot the device |
| `log` | Print system log |
| `eeprom get` | Display current NVS configuration |
| `eeprom clean` | Erase NVS configuration |
| `manf-set <field> <value>` | Set a manufacturing field (`hwver`, `nodeId`, `secretkey`, `p_code`, `hw_var`) |
| `install <ip> <file>` | Trigger an OTA update from a local HTTP server |

---

## Deployment

The GitHub Actions pipeline is split into CI and CD:

1. **CI**: [CI ESP-IDF PR Build](.github/workflows/CI-PR.yml)
2. **CD**: [CD ESP-IDF Release Deploy](.github/workflows/CN001-SN001-BUILD.yml)

CI runs on pull requests targeting `main` or `release` and performs:

1. **Builds** the firmware inside the `espressif/idf:latest` Docker container targeting `esp32s3`.
2. **Uploads build artifacts** (`build/` + `RELEASE.md`) as a GitHub Actions artifact retained for 14 days.

CD runs on version tags (for example, `v1.2.3`) and performs:

1. **Builds** the firmware inside the `espressif/idf:latest` Docker container targeting `esp32s3`.
2. **Uploads build artifacts** (`build/` + `RELEASE.md`) as a GitHub Actions artifact retained for 30 days.
3. **Publishes a release to Backblaze B2**, uploading three files to the configured bucket:

| File | Description |
| --- | --- |
| `firmware.bin` | Application binary |
| `version.txt` | Firmware version string + SHA-256 hash |
| `RELEASE.md` | Internal release notes |

Any existing files in the bucket are automatically archived under `archive/<UTC timestamp>/` before the new versions are written.

### Required secrets

The following repository secrets must be configured in GitHub for the Backblaze upload step to succeed:

| Secret | Description |
| --- | --- |
| `B2_KEY_ID` | Backblaze B2 application key ID |
| `B2_APPLICATION_KEY` | Backblaze B2 application key |
| `B2_BUCKET` | Target bucket name |
