# ESP32-BLE-File-Transfer

To setup this project:
-have a linux device with bluetooth capabilities (this has only been tested on debian)
-have an ESP32 (only tested with Arduino Nano ESP32) wired to an SD card reader and plugged 
 into another PC (different than linux/debian system)

ESP32 Wiring Setup:

ESP32 -----> SD Card Reader
 GND            GND
 D13            CLK
 3.3v           3v3
 D12            MISO
 D11            MOSI
 D10             CS
 
Arduino Code:
-download/open arduino IDE
-install the Espressif Systems esp32 board and use it
-install libraries: ESP32 BLE Arduino by Neil Kolban, SD, SPI and FS. The latter 3 are usually pre-installed or included.
-open BLExxxxx.ino in the IDE and upload it to board

Debian Setup:
-install bleak: pip install bleak
-make sure bluetooth service is running: sudo systemctl start bluetooth
-run the given BLExxx.py file: sudo python3 blething.py

From here, everything should be functional, and the devices should communicate flawlessly :)
