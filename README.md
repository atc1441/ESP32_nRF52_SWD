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


HowTo:

- Open the "ESP32_SWD_WIFI.ino" file with Arduino and select the "ESP32 Dev Module", Make sure to select "No OTA (1MB App / 3MB SPIFFS)" as config for the ESP32 to have more space available.
- Select the ESP32 COM port
- Click on Upload and wait for a succesfull upload
- The ESP32 will now create a Wifi Network with the WifiManage called "AutoConnectAP" with your Browser go to the ip 192.168.4.1 after connecting to the Wifi
- Configure the Wifi to your Home network
- If fully connected enter "http://swd.local" in your internet browser and it should show a first page from the ESP32
- Go to "http://swd.local/edit" log in with admin:admin browse for the "index.htm" file and upload it via the supplied webpage
- Got to "http://swd.local" again, the SWD Flasher paged should be shown now.
- Connect the nRF via SWD if not already done. Click the button "Init SWD" and wait for the response in the info page or look in the Arduino UART terminal if something doesnt work, The nRF chip should be detected, if the nRF is locked it will notify about that.

. To flash a new firmware to an nRF you can erase the whole chip to then flash an uploaded file via the "Flash file" button, you need to enter the correct filename.
- To dump the flash content of an nRF enter a filename an offset if wanted and a size of bytes in dezimal then click the "Dump to file" button and wait for it.
- To Glitch the nRF use the Delay input to find the right spot on where to glitch, it should be near the small voltage drop of the DEC1 line, best is to have an Osci connected to see what is happening but you can also just blindly find the Delay as the delay will increase and the ESP32 will notify about a suffesfull glitch after clicking "Enable Glitcher" you can change the delay time on the fly.

