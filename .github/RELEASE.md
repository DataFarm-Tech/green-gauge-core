# Internal Release Notes

Internal summary of the current Green Gauge Core firmware release.

## Supported Hardware Versions

Officially mapped hardware versions in this firmware build:

| Hardware Version | Status |
| --- | --- |
| 0.0.1 | ✅ Supported |
| 0.0.2 | ✅ Supported |

## Included In This Firmware

- Runtime: ESP32-S3 runtime with hardware-aware communication setup.
- Connectivity: Cellular modem support over UART with connection management, packet transport, and HTTPS streaming support.
- Connectivity: Wi-Fi support in the communication stack.
- Protocols: CoAP transport for activation, sensor reporting, and GPS updates.
- Protocols: CBOR packet encoding for compact telemetry payloads.
- Provisioning: Device activation flow with persisted activation state.
- Provisioning: Manufacturing and identity fields for hardware version, hardware variant, firmware version, node ID, product code, modem serial, SIM serial, chassis version, and secret material.
- Sensors: NPK support over RS485 / Modbus RTU.
- Sensors: Measurement support for nitrogen, phosphorus, potassium, moisture, pH, and temperature.
- Sensors: Per-measurement calibration with stored gain and offset values.
- Data Collection: Batched sensor reading collection with session tracking.
- Location: GPS acquisition through the modem with coordinate upload support.
- OTA: Semantic-version-based update checks.
- OTA: Firmware download, OTA write, and reboot flow after successful update.
- Storage: Persistent configuration storage in NVS.
- Storage: LittleFS-backed logs and queued telemetry storage.
- Storage: Rotating log files with system and error logging.
- Reliability: Persistent queue for unsent telemetry packets with replay when connectivity returns.
- Power: Deep-sleep operation with configurable wake interval and wakeup-driven runtime flow.
- Tooling: UART CLI with help, reset, version, log, history, EEPROM management, and manufacturing-field provisioning commands.
- Tooling: Remote telnet session support for modem command execution and diagnostics over the cellular path.

## Release Artifacts

| Artifact | Notes |
| --- | --- |
| Firmware binary | Build output for flashing or release distribution |
| Version metadata with SHA-256 | Version identifier plus firmware hash |
| Release notes | Internal summary for this release |

## Known Gaps and Follow-Ups

- Ethernet support is not complete; the adapter is still effectively a stub in the communication layer.
- OTA needs better failure diagnostics around firmware download, CBOR version parsing, and EEPROM persistence.
- Wi-Fi and cellular reconnect behavior works, but failure and retry visibility should be improved in logs.
- GPS handling still falls back broadly; modem failure, timeout, and invalid-fix cases should be separated more clearly.
- Activation and measurement send failures should be reviewed so every failure path is visible and recoverable.
- LittleFS queued-packet replay should be hardened further, especially for partial flush failures and backlog inspection.
- Remaining TODO and placeholder areas in the shell and network adapter code should be cleaned up.




