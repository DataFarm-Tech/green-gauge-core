esptool --chip esp32s3 --port /dev/ttyACM0 erase_flash

esptool.py --chip esp32s3 --port /dev/ttyACM0 write_flash 0x9000 manf-info/NT0052/NT0052.bin

cd build
esptool.py --chip esp32s3 -b 460800 --before default-reset --after hard-reset write-flash "@flash_args"
