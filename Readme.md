Description from orginal repository after the changes paragraph.
Removed dead links and generalized description (replaced Bitcoin with Crypto)



# Changes introduced by this fork
* Added new exchange websocket
* Added option to select which pairs should be dispalyed
* Support for multiple pairs
* (Optional) Show asset part of name (maybe problematic on 8 digit display)
* Only display USD price



# Crypto price ticker with ESP8266 (realtime websockets)
* 7-segment 8-bit - or - Dot Matrix LED Display support with 32x8px
* bitstamp & bitfinex websocket interfacing for lightning fast, real time updates!
* solderless build possible (if you order the display with pre-soldered pin headers)
* low power (<0.5W), cheap to build (around 5 EUR)

## pictures in action
![animation](https://thumbs.gfycat.com/VainBeautifulAcornwoodpecker-size_restricted.gif)  
v0.2-bitstamp-websockets in action ([gfycat link](https://gfycat.com/gifs/detail/VainBeautifulAcornwoodpecker))

![coinboard case](docs/images/photo_coinboard_case.jpg)
3D printed coinboard case

![picture](docs/images/btc-ticker-esp8266-matrix32.jpg)  
v0.3 with dot matrix display

![picture](docs/images/btc-ticker-esp8266.jpg)  
original prototype

## components
* ESP8266s/NodeMCUs or Wemos D1 Mini
* 7 segment display with MAX7219
* dupont cables

## wiring 7-segment (software SPI, Wemos D1 Mini)

D1 Mini | Display
--- | ---
GND | GND
5V/VIN | VCC
D8  | CS
D7  | DIN
D6  | CLK

## wiring dot-matrix-display using hardware SPI

NodeMCU | Display
---     | ---
GND     | GND
5V/VIN  | VCC
D6      | CS
D7      | CLK
D8      | DIN

## how to install
- flash the board
  * upload source sketch with arduino IDE
  * or flash binary [(download)](https://github.com/nebman/btc-ticker-esp8266/releases) with [esptool (python)](https://github.com/espressif/esptool) or [flash download tools (WIN)](https://espressif.com/en/support/download/other-tools) @ address 0x0
- connect board to power 
- connect your smartphone/computer to ESPxxxxxx wifi
- enter your home wifi settings at the captive portal

## 3D printed case options

these are the 3rd party designs I have tested for this project

Case | Link | Remarks
-----|-----|-----
coinboard|https://www.thingiverse.com/thing:2785082 |Wemos D1 only, works okish with some hotglue ([picture](docs/images/photo_coinboard_case.jpg))
ESP8266 Matrix Display Case|https://www.thingiverse.com/thing:2885225| printed too small, pay attention to your matrix module size! mine was a little larger
IR Remote Tester|https://www.thingiverse.com/thing:1413083|works good (I closed the sensor hole above the display) but screw mechanism needs rework or maybe screw inserts

## known issues

- compilation error in LedControl.h:  
solution: comment out or delete pgmspace.h include
