## ESP32 Controller Documentation

# Looking for pinout, datasheets, or other information? Check Hardware Repository [HERE](https://github.com/DataFarm-Tech/cn001-sn001-hardware)


## LLM Message Protocol

| Byte Num | Purpose                   |
| ---------| --------------------------| 
| 0-5      | dest ID                   |
| 6-12     | src ID                    |
| 13-19    | DATA                      |
| 20-21    | CALC CRC16 CCITT FALSE    |

Note: if a data request is made from the controller, then the data field is set to 0.
#



To flash software on the esp32, run the following command:

```./flash flash ```

The following command compiles the firmware into firmware.elf, followed by uploading the firmware.elf file into the device. You can see this by reading through the script (the following scripts use platformio env). All env variables are set.
Note: Please make sure that the esp32 is connected to your computer before executing the script.


To only generate the firmware.elf file, execute this command:

```./flash.sh deploy ```

The following command compiles the firmware into a firmware.elf file.


The following command compiles and uploads the firmware with some env variables enabled:

```./flash.sh dev ```


## Workflows
deploy_build.yml

The following workflow does the following:
- Installs `python` and 'python3-pip'
- Installs 'pio' using 'python3-pip'
- Compiles the code into one firmware.elf binary
- Uploads the firmware.elf file via FTP
- Increments the version number in the FTP server by 0.1


test.yml

The following workflow does the following:
- Installs `python` and 'python3-pip'
- Installs 'pio' using 'python3-pip'
- Compiles the code into one firmware.elf binary (If the compilation is unsuccessful the workflow will fail.)
- This workflow executes for all developer branches

Notes:
//1. Read ssid and password from EEPROM mem
//2. Check that ssid and password did'nt return NULL
//3. Reading compile time variables from config.h and converting them into IPAddress object
//4. Configure wifi setup with those IPAddress objects (IPv4, GW, SN, DNS)
//5. Start wifi with ssid and password credentials found in EEPROM mem (if wifi does not connect then exit function and start BLE)
//6. Ping google.com to check that Network config is correct
//7. If OTA_ENABLED is set to true then start the OTA class

WiFi.begin("MyWifi6E", "ExpZZLN9U8V2");