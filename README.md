# ESP32 SWD Flasher for nRF52
This software makes it possibile to Read and Write the internal Flash of the Nordic nRF52 series with an ESP32 using the SWD interface.
A tool to exploit the APPROTECT vulnerability is included as well.

### You can support my work via PayPal: https://paypal.me/hoverboard1 this keeps projects like this coming.

To flash an nRF52 connect the following:
- nRF **SWDCLK** to ESP32 **GPIO D21**
- nRF **SWDIO** to ESP32 **GPIO D19**
- nRF **GND** to ESP32 **GND** to **NFet Mosfet GND**

 Then power the nRF52 as needed

To bypass the Readout protection (APPROTECT) of an nRF52 connect all of the above and the following:
- nRF 3.3V Power **VDD** to ESP32 **GPIO D22**
- **NFet Mosfet** to ESP32 **GPIO D5** (as shown)
- **NFet Mosfet Gate** to nRF **DEC1** (as shown)

 Then power the nRF52 as needed


This repo is explained and demonstrated in these videos (click to watch):


[![YoutubeVideo](https://img.youtube.com/vi/tMPD0kBG_So/0.jpg)](https://www.youtube.com/watch?v=tMPD0kBG_So)


[![YoutubeVideo](https://img.youtube.com/vi/Iu6RoXRZxOk/0.jpg)](https://www.youtube.com/watch?v=Iu6RoXRZxOk)

### Required Hardware

- ESP32 Development Board
- NFet Mosfet
- nRF52 Series Board
- Optional: Oscilloscope

### Required Software

- Arduino IDE https://www.arduino.cc/
- ESP32 core Library https://github.com/espressif/arduino-esp32
- This version of the WifiManager https://github.com/tzapu/WiFiManager/tree/feature_asyncwebserver
- AsyncTCP https://github.com/me-no-dev/AsyncTCP
- ESPAsyncWebServer https://github.com/me-no-dev/ESPAsyncWebServer

### HowTo:

Note: Use version 1.0.6 of the ESP32 core. Also use the source files vs the release packages.

#### Arduino:
(It is also possible to use PlatformIO)

- Copy or install the three downloaded libraries (AsyncTCP, ESPAsyncWebServer, WifiManager) into the Arduino > libraries directoy
- Arduino > libraries: Rename:
 - AsyncTCP-master to AsyncTCP
 - ESPAsyncWebServer-master to ESPAsyncWebServer
 - WiFiManager-master to WiFiManager
- Add the ESP32 Core to Arduino (File > Additional Boards Manager URLS > https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json)
- Install the ESP32 Core (Tools > Boards Manager > Search > esp32 > Select version > 1.0.6 > Install)
- Open the "ESP32_SWD_WIFI.ino" file with Arduino and select the "ESP32 Dev Module" (Tools > Board: > ESP32 Arduino > ESP32 Dev Module)
- Make sure to select "No OTA (1MB App / 3MB SPIFFS)" as the partition scheme for the ESP32 to have more space available. (Tools > Partition Scheme: > "No OTA (1MB App/3MB SPIFFS)")
- Select the ESP32 COM port (Tools > Port: > COM#)
- Click Verify
- Click Upload
- Long-press the BOOT button on ESP32 after clicking Upload, until it is showing "Uploading"
- Once it displays "Leaving... Hard resetting..." its done flashing and ready to setup
- The ESP32 will now create a Wifi Network with the WifiManager called: "AutoConnectAP" after connecting to the Wifi go to the ip: 192.168.4.1 in your Browser
- Configure the Wifi for your home network
- Once fully connected enter: "http://swd.local" in your internet browser and it should show a first page from the ESP32
- Go to: "http://swd.local/edit" login with admin:admin
- Click Choose File and browse for the "data/index.htm" file and click Upload
- Go to: "http://swd.local" again, the ESP32 SWD Flasher page should now be displayed
- If not already done, connect the nRF via SWD. Click the button "Init SWD" and wait for the response in the info page or look in the Arduino UART terminal if something doesn't work. The nRF chip should be detectedand it will display a notification about whether or not the nRF is locked
- To flash new firmware to an nRF you can erase the whole chip and then flash an uploaded file via the "Flash File" button, you need to enter the correct filename
- To dump the flash content of an nRF enter a filename, an offset if wanted, and a size of bytes in decimal then click the "Dump to File" button and wait for it to finish
- To Glitch the nRF use the Delay Input to find the right spot to glitch, it should be near the small voltage drop of the DEC1 line, best is to have an Oscilliscope connected to see what is happening, but you can also just blindly find the delay as the delay will automatically increase and the ESP32 will notify when it achieves a successfull glitch after clicking "Enable Glitcher" you can change the delay time on the fly



### ESP32 Glitcher schematic:

<img width="800" alt="" src="https://github.com/atc1441/ESP32_nRF52_SWD/blob/main/ESP32_nRF_glitcher_schematic.jpg">



Credits go to LimitedResults for finding the Power glitching Exploit: https://limitedresults.com/2020/06/nrf52-debug-resurrection-approtect-bypass-part-2/
