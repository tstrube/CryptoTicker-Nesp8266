# Changes introduced by this fork
- Most code changed, original used as reference only
- Added new exchange okex
- Added option to select which pairs should be displayed
- Support for multiple pairs
- (Optional) Show asset part of name 
  - (maybe problematic high currency coins like bitcoin)
  - to combat this, added option to shorten values
- Since the original idea was for BTC, only values greater 1$ will work
- Only displays USD price
- Removed support for Matrix display (I didn't need it)
- Removed old websocket settings and untested exchanges

# Crypto price ticker (websockets) with ESP8266 
- 7-segment 8 digit display
- okex websocket interfacing for lightning fast, real time updates!
- solderless build possible (if you order the display with pre-soldered pin headers)
- low power (<0.5W), cheap to build

## pictures in action
![animation](https://thumbs.gfycat.com/VainBeautifulAcornwoodpecker-size_restricted.gif)  
Websockets in action ([gfycat link](https://gfycat.com/gifs/detail/VainBeautifulAcornwoodpecker))

## Components
- NodeMCUs
- 7 segment display with MAX7219
- dupont cables

## Wiring 7-segment
NodeMCU | Display
--- | ---
GND | GND
5V/VIN | VCC
D8  | CS
D7  | DIN
D6  | CLK

## How to install
- flash the board (upload source sketch with arduino IDE)
- connect board to power
- connect your smartphone/computer to ESPxxxxxx wifi
- enter your home wifi settings at the captive portal

## My 3D printed case



## Known issues
- compilation error in LedControl.h:  
solution: comment out or delete pgmspace.h include