# Changes introduced by this fork
* Added new exchange okex
* Added option to select which pairs should be displayed
* Support for multiple pairs
* (Optional) Show asset part of name 
  * (maybe problematic high currency coins like bitcoin)
  * to combat this, added option to shorten values
* Only displays USD price
* Removed support for Matrix display (I didn't need it)

# Crypto price ticker with ESP8266 (realtime websockets)
* 7-segment 8-bit display
* bitstamp, bitfinex, okex websocket interfacing for lightning fast, real time updates!
* solderless build possible (if you order the display with pre-soldered pin headers)
* low power (<0.5W), cheap to build

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
* NodeMCUs
* 7 segment display with MAX7219
* dupont cables

## wiring 7-segment

NodeMCUs | Display
--- | ---
GND | GND
5V/VIN | VCC
D8  | CS
D7  | DIN
D6  | CLK

## how to install
- flash the board (upload source sketch with arduino IDE)
- connect board to power
- connect your smartphone/computer to ESPxxxxxx wifi
- enter your home wifi settings at the captive portal

## My 3D printed case



## known issues

- compilation error in LedControl.h:  
solution: comment out or delete pgmspace.h include