# Diesel Filter Particle viewer

Only tested in a Volkswagen 2.0 TDI CAGA engine.

The code is mostly based on https://github.com/fkoca5055/obdESP with some changes to check DFP status, provide a WiFi Access Point to change Bluetooth device, PIN, debug viewer...

## Needed parts

 - ESP32 NodeMCU ESP-32s
 - TFT Screen based on ILI9341
 - ELM327

## Box and schema

 - [PCB](schema/dpf1) designed with EAGLE.
 - [Box to 3d print](schema/3dmodel) designed with FreeCAD.
 
 
## Pinout


|  TFT |   ESP32 GPIO |
|---|---|
| TFT_CS | 15 |
| TFT_DC | 4 |
| TFT_MOSI | 23 |
| TFT_SLK | 18 |
| TFT_RST | 2 |
| TFT_LED | -1 |
| TFT_MISO | 19 |

Please change `TFT_RST` to another GPIO since this the integrated led in `NodeMCU ESP-32s`.

# Next steps

Remove the screen and make use of the FIS to display DFP information

# Useful links

- https://stackoverflow.com/questions/23877761/sniffing-logging-your-own-android-bluetooth-traffic
- https://forums.tdiclub.com/index.php?threads/reading-soot-level-with-torque.464119/page-4
- https://github.com/fkoca5055/obdESP/blob/master/2.4_LCD.ino
- https://www.cardumps.net/account/my-profile
- https://stackoverflow.com/questions/23877761/sniffing-logging-your-own-android-bluetooth-traffic
- http://jakubdziworski.github.io/obd/bluetooth/arduino/car/2020/07/13/arduino-obd-dpf-monitor-insignia.html
- http://www.areavag.com/foro/showthread.php?42159-Regeneraci%F3n-filtro-part%EDculas-diagnosis-con-iPhone/page3
- https://gist.github.com/0/c73e2557d875446b9603
- https://stackoverrun.com/es/q/12573350
- http://jakubdziworski.github.io/
- https://scribles.net/setting-up-bluetooth-serial-port-profile-on-raspberry-pi/
- https://forum.fairphone.com/t/location-of-btsnoop-hci-log-file/60320
- https://www.fte.com/WebHelpII/Sodera/Content/Documentation/WhitePapers/BPA600/Encryption/GettingAndroidLinkKey/RetrievingHCIlog.htm
- https://stackoverflow.com/questions/23877761/sniffing-logging-your-own-android-bluetooth-traffic
- https://github.com/fkoca5055/obdESP
- https://github.com/ibanezgomez/FISBlocks
