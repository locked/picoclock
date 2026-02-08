== building ==

Basic binary files are provided, but the code should be fairly easy to build with:

 arduino-cli compile -b esp32:esp32:esp32 --verbose

Once you have the 3 bin files (or just want to use the one from this repository), flash your module with esptool (change the `boot_app0.bin` path):

 esptool --chip esp32 --port "/dev/ttyUSB0" --baud 921600  --before default-reset --after hard-reset write-flash  -z --flash-mode keep --flash-freq keep --flash-size keep 0x1000 esp32.ino.bootloader.bin 0x8000 esp32.ino.partitions.bin 0xe000 path/to/esp32/tools/boot_app0.bin 0x10000 esp32.ino.bin

