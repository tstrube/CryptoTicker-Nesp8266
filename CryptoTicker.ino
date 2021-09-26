// START CONFIGURATION  ***************************************************************************

// Set DEBUGGING for addiditonal debugging output over serial
// #define DEBUGGING

// set Hostname
#define HOSTNAME "ESP-CRYPTO-TICKER"

// set SHOW_ASSET_NAME to display text from "exchange_assets" array (max 4 chars)
// (see function "convertCharToSegmentDigit" regarding custom character definitions)
#define SHOW_ASSET_NAME

// set exchange 
exchange_settings exchange = okex;
String[] exchange_pairs = {"BTC-USDT", "ETH-USDT", "XCH-USDT"};
String[] exchange_assets = {"BTC ", "ETH ", "CHIA"};
int exchange_pairs_count = 3;

// Variables and constants for timeouts 
const int     timeout_hard_threshold = 60000; // reconnect after 60sec
const int     timeout_soft_threshold = 30000; // timeout 30sec, send ping to check connection
boolean       timeout_soft_sent_ping = false;
unsigned long timeout_next = 0;
unsigned long timeout_flashing_dot = 0;
unsigned int  timeout_reconnect_count = 0;

// END CONFIG *************************************************************************************
// START INCLUDES *********************************************************************************

#include "exchanges.h"

#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <WebSockets.h>
#include <LedControl.h>
#include <math.h>
// todo?: stdlib

// init LedControl library, pin assignment see Readme.md
LedControl lc = LedControl(D7, D6, D8, 1);

// init wifi
WiFiManager wifiManager;
WiFiClientSecure client;

// init websocket (ws)
WebSocketClient ws;

// END INCLUDES ***********************************************************************************
// START SETUP ************************************************************************************

// setup wifi, connect wifi
void setup() {
  // start serial for debug output
  Serial.begin(115200);
  Serial.println();
  Serial.println("INIT");
  
  // wakeup 7 segment display, set brightness
  lc.shutdown(0, false);
  lc.setIntensity(0, 6);
  clearDisplay();

  // use 8 dots for startup animation
  setProgress(1);
  Serial.println("WIFI AUTO-CONFIG");
 
  // autoconfiguration portal for wifi settings
  WiFi.hostname(HOSTNAME);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect();

  // wifi setup finished 
  setProgress(2);
  Serial.println("WIFI DONE");

  // MDNS
  if (!MDNS.begin(HOSTNAME)) {
    Serial.println("MDNS ERROR!");

  } else {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS UP");
  }
  setProgress(3);

  setProgress(5);
  connect(false);
}

// (re)connect ws
void connect(boolean reconnect) {
  timeout_soft_sent_ping = false;

  if (reconnect) {
    timeout_reconnect_count++;
    if (timeout_reconnect_count > 2) reboot();
    clearDisplay();
  }

  // connect to the server
  Serial.println("Client connecting...");
  if (client.connect(exchange.host, exchange.port)) {
    Serial.println("WS Connected");
    setProgress(6);

  } else {
    Serial.println("WS Connection failed.");
    reboot();
  }

  // set up websocket details
  ws.setPath(exchange.url);
  ws.setHost(exchange.host);
  ws.setProtocol(exchange.wsproto);

  Serial.println("Starting WS handshake...");
  
  if (ws.handshake(client)) {
    Serial.println("WS Handshake successful");
    setProgress(7);

  } else {
    Serial.println("WS Handshake failed.");
    reboot();
  }

  // subscribe to event channel "live_trades"
  Serial.println("WS Subscribe");

  String subscribe_command = exchange.prefix;
  bool first_pair = true;

  for (int i = 0; i < exchange_pairs_count; i++) {
    if (!first_pair) subscribe_command += exchange.separator;
    first_pair = false;
    subscribe_command += exchange_pairs[i];
  }
  subscribe_command = exchange.suffix;

  ws.sendData(subscribe_command);
  setProgress(8);

  // Finish setup, complete animation, set first timeout
  clearDisplay();
  setProgress(0);
  timeout_next = millis() + timeout_hard_threshold;
  
  Serial.println("All set up, waiting for first trade...");
  Serial.println();
}

void reboot(void) {
  setDashes();
  delay(1000);
  ESP.reset();
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered wifi portal config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

// END SETUP **************************************************************************************
// START MAIN LOOP ********************************************************************************

void loop() {
  ws.process(); // process websocket 

  // check for hard timeout
  if( (long)(millis() - timeout_next) >= 0) {
    Serial.println();
    Serial.println("TIMEOUT -> RECONNECT");
    Serial.println();
    connect(true);
  }

  // Process okex
  switch (exchange.id) {
    case "okex":

    default:
  }







  // check for soft timeout (slow day?) send websocket ping
  if (!timeout_soft_sent_ping && client.connected() && 
      (long)(millis() - timeout_next + timeout_soft_threshold) >= 0) {
    // ok, lets send a PING to check connection
    Serial.println("soft timeout -> sending ping to server");
    ws.sendData("", WS_OPCODE_PING);
    timeout_soft_sent_ping = true;
    yield();
  }

  // flash the dot when a trade occurs
  flashingDotTradOff();

  // check if socket still connected
  if (client.connected()) {
    String line;
    uint8_t opcode = 0;
    
    if (ws.getData(line, &opcode)) {
  
      // check for PING packets, need to reply with PONG, else we get disconnected
      if (opcode == WS_OPCODE_PING) {
        Serial.println("GOT PING");
        ws.sendData("{\"event\": \"pusher:pong\"}", WS_OPCODE_PONG);
        Serial.println("SENT PONG");
        yield();
      } else if (opcode == WS_OPCODE_PONG) {
        Serial.println("GOT PONG, connection still active");
        timeout_soft_sent_ping = false;
        timeout_next = millis() + timeout_hard_threshold;
      }
  
      // check for data in received packet
      if (line.length() > 0) {
#ifdef DEBUGGING
        Serial.print("Received data: ");
        Serial.println(line);
#endif

        // parse JSON
        StaticJsonBuffer<768> jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(line);
        JsonArray& rootarray = jsonBuffer.parseArray(line);

        yield();
  
        // alright, check for trade events
        if (root["event"] == "trade") {
          timeout_next = millis() + timeout_hard_threshold;
          Serial.print("GOT TRADE ");
  
          // need to deserialize twice, data field is escaped
          JsonObject& trade = jsonBuffer.parseObject(root["data"].as<const char*>());
          yield();
            
          if (!trade.success()) {
            Serial.println("parse json failed");
            return;
          }
#ifdef DEBUGGING 
          trade.printTo(Serial);
#endif
          // extract price
          last = trade["price_str"]; // last USD btc
      
          Serial.print("price = ");
          Serial.println(last);
          
        } else { // print unknown events and arrays
          root.printTo(Serial);
          rootarray.printTo(Serial);
          Serial.println();
        }
  
        updateDisplay(asset, price);
      } 
    }

  } else {
    Serial.println("Client disconnected.");
    connect(true);
  }

  delay(5);
}

// END MAIN LOOP **********************************************************************************
// START DSIPLAY FUNCTIONS ************************************************************************

// Format and write price to display
void updateDisplay(String asset, int price) {
  int asset_length = 0;

#ifdef SHOW_ASSET_NAME
  // Print asset name
  asset_length = asset.length();
  if (asset_length > 4) asset_length = 4;

  for (int i = 0; i < asset_length; i++) {
    lc.setChar(0, 7 - i, asset.charAt(i), false);
  }
#endif

  // Use rest of space to display last price 
  // shorten big numbers using 1k separator - there is space for at least 4 digits
  int remaining_space = 8 - asset_length;
  int required_space = floor(log10(abs(price))) + 1;
  int dotPosition = -1;

  // If not enough space
  if (remaining_space < required_space) {
     // Find dotPosition, if required
    if (required_space > 3) {
      dotPosition = required_space % 3;
      if (dotPosition == 0) dotPosition = 3;

      // Calc LCD position
      dotPosition = remaining_space - dotPosition;
      if (dotPosition == 0) dotPosition = -1;
    }
  }

  // Print price
  String priceString = String(price);
  for (int i = 0; i < min(8 - asset_length, required_space) ; i++) {
    lc.setDigit(0, 7 - i, priceString.charAt(i), dotPosition = 7 - i);
  }
  
  // Print empty space between asset and price
  for (int i = asset_length; i > required_space; i--) {
    lc.setChar(0, i, ' ', false);
  }

  // Set flashing dot timeout (turning off)
  timeout_flashing_dot = millis() + 100;
}

// END DSIPLAY FUNCTIONS **************************************************************************
// START DSIPLAY OUTPUT SHORTCUTS *****************************************************************

// Turn all elements off
void clearDisplay() {
  for (int i = 0; i < 8; i++) {
    lc.setChar(0, i, ' ', false);
  }
}

// Display minus on all elements
void setDashes() {
  for (int i = 0; i < 8; i++) {
    lc.setChar(0, i, '-', false);
  }
}

// Use dots as progress bar
void setProgress(byte progress) {
  for (byte i = 0; i < 8; i++) {
    lc.setLed(0, 7 - i, 0, i < progress);
  }
}

// Turn of flashing dot on trade
void flashingDotTradOff() {
  lc.setLed(0, 0, 0, !(millis() < timeout_flashing_dot);
}

// Convert char to byte usable by LedControl
byte convertCharToSegmentDigit(char c) (
  // Implemented in library: A b c d E F H L P - _ . , <SPACE>

  /*     
  *  display segment bits
  *  
  *      1
  *    -----
  *   |     |
  * 6 |     | 2
  *   |     |
  *    -----
  *   |  7  |
  * 5 |     | 3
  *   |     |
  *    ----- . 0
  *      4
  * 
  */

  switch (c) {
    case 'C':
      return B01001110;
    case 'I':
    case 'i':
      return B00110000;
    case 'N':
    case 'n':
      return B00010101;
    case 'O':
    case 'o':
      return B00011101;
    case 'P':
    case 'p':
      return B01100111;
    case 'R':
    case 'r':
      return B00000101;
    case 'S':
    case 's':
      return B01011011;
    case 'T':
    case 't':
      return B00001111;
    case 'U':
    case 'u':
      return B00111110;
    default:
      return c;
  }   
)

// END DSIPLAY OUTPUT SHORTCUTS *******************************************************************