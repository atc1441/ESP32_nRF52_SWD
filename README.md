# ESP32_nRF52_SWD
This software brings you the possibility to Read and Write the internal Flash of the Nordic nRF52 series with an ESP32

To flash an nRF52 connect the SWD pins: CLK to GPIO 21 and DIO to GPIO 19 of the ESP32. Also connect the nRF52 GND to the ESP32 and power the nRF52 as needed.

To bypass the Readout protection of an nRF52 connect the following:

3.3V Power of the nRF to GPIO 22 of the ESP32
NFet Mosfet like shown to GPIO 5 of the ESP32 - the Gate from the Mosfet to the DEC1 pin of the nRF
SWD pins as mentioned above




### Needed Software

- Arduino IDE https://www.arduino.cc/
- ESP32 core Library https://github.com/espressif/arduino-esp32
- This version of the WifiManager https://github.com/tzapu/WiFiManager/tree/feature_asyncwebserver
- AsyncTCP https://github.com/me-no-dev/AsyncTCP
- ESPAsyncWebServer https://github.com/me-no-dev/ESPAsyncWebServer
