// START INCLUDES *********************************************************************************

#include "exchanges.h"

#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ArduinoWebsockets.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LedControl.h>
#include <Math.h>
#include <WiFiManager.h>

// END INCLUDES ***********************************************************************************
// START CONFIGURATION  ***************************************************************************

// Set DEBUGGING for addiditonal debugging output over serial
//#define DEBUGGING

// set HOSTNAME
#define HOSTNAME "ESP-CRYPTO-TICKER"

// set SHOW_ASSET_NAME to display text from "exchangePAirs" array (fill "maxAssetLength" chars)
// (see function "convertCharToSegmentDigit" regarding custom character definitions)
#define SHOW_ASSET_NAME
const int maxAssetLength     = 3;
const int assetReservedSpace = 4;

// set exchange and pairs
exchangeSettings exchange    = coinbase;
const String exchangePairs[] = {"BTC-EUR", "ETH-EUR"};
const int exchangePairsCount = 2;

// switch to next pair after ms
const int switchPairMS       = 5000;

// set SHOW_CUSTOM_VALUE to display a custom value from a function
// it will be displayed after each full loop of the exchangePairs
#define SHOW_CUSTOM_VALUE
const double currentEthHodl = 0; // custom const used in function

// set websocket timeouts 
const unsigned long timeoutHardThreshold = 60000; // reconnect after 60sec
const unsigned long timeoutSoftThreshold = 20000; // send ping after 20sec

// END CONFIG *************************************************************************************
// START SETUP ************************************************************************************

// some globals
String              lastPrice[exchangePairsCount];
unsigned long       lastDisplaySwitch = 0;
int                 currentPairIndex = 0;

boolean             timeoutSoftSentPing = false;
unsigned long       timeoutHardNext = 0;
unsigned long       timeoutSoftNext = 0;
unsigned long       timeoutFlashingDot = 0;
unsigned int        timeoutReconnectCount = 0;

// init LedControl library, pin assignment see Readme.md
LedControl lc = LedControl(D8, D6, D7, 1);

// init wifi
WiFiManager wifiManager;
WiFiClientSecure client;

// init websocket (ws)
using namespace websockets;
WebsocketsClient ws;



// setup and connect WiFi
void setup() {
  // start serial for debug output
  Serial.begin(115200);
  Serial.println();
  Serial.println("INIT");
  
  // wakeup 7 segment display, set brightness
  lc.shutdown(0, false);
  lc.setIntensity(0, 0);
  clearDisplay();

  // use 8 dots for startup animation
  setProgress(1);
  Serial.println("WIFI AUTO-CONFIG");
 
  // autoconfiguration portal for wifi settings
  WiFi.hostname(HOSTNAME);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect();

  // wifi setup finished 
  setProgress(3);
  Serial.println("WIFI DONE");

  // MDNS
  if (!MDNS.begin(HOSTNAME)) {
    Serial.println("MDNS ERROR!");
  } else {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS UP");
  }
  setProgress(4);

  // Arduino OTA
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA End");
  });
  ArduinoOTA.begin();
  Serial.println("OTA started");
  setProgress(5);

  connect(false);
}



// websocket message callback handling
void onMessageCallback(WebsocketsMessage message) {
  String line = message.data();

#ifdef DEBUGGING
  Serial.print("Received data: ");
  Serial.println(line);
#endif

  // parse JSON
  StaticJsonDocument<768> jsonDocument;
  DeserializationError error = deserializeJson(jsonDocument, line);
  if (error) {
    Serial.println("!!! Parse ERROR !!!");

  } else {
    yield();
  
    // extract information
    String asset = jsonDocument[exchange.asset] | "parseError";
    String price = jsonDocument[exchange.price] | "-1";
  
    // make sure values are found
    if (asset != "parseError") {
      // update timeout
      setNewTimeout();

      // add value to cache
      bool priceChanged = false;
      for (int i = 0; i < exchangePairsCount; i++) {
        if (exchangePairs[i] == asset) {
          priceChanged = lastPrice[i] != price;
          lastPrice[i] = price;
          break;
        }
      }

      // update display
      if (priceChanged) updateDisplay(exchangePairs[currentPairIndex], lastPrice[currentPairIndex]);
     
#ifdef DEBUGGING
      // print new info
      Serial.print("GOT TICKER: ");
      Serial.print(asset);
      Serial.print(" ");
      Serial.println(price);
#endif

    // print unknown events
    } else {
      Serial.println("!! UNKOWN RESPONSE !!!");
      serializeJsonPretty(jsonDocument, Serial);
      Serial.println();
    }
  }
}



// websocket pong answer on ping
void onEventsCallback(WebsocketsEvent event, String data) {
  if(event == WebsocketsEvent::GotPing) {
    Serial.println("Got a Ping. Sending Pong.");
    ws.pong();
  }
}



// (re)connect websocket
void connect(boolean reconnect) {
  timeoutSoftSentPing = false;

  if (reconnect) {
    timeoutReconnectCount++;
    if (timeoutReconnectCount > 2) reboot();
    clearDisplay();
  }
  
  // Setup Callbacks
  Serial.println("Setup callbacks ...");
  ws.onMessage(onMessageCallback);
  ws.onEvent(onEventsCallback);
  setProgress(6);

  // connect to the server
  Serial.println("Client connecting...");
  if (ws.connect(exchange.url)) {
    Serial.println("WS Connected");
    setProgress(7);

  } else {
    Serial.println("WS Connection failed.");
    reboot();
  }

  // subscribe to event channel
  Serial.println("WS Subscribe");

  String subscribeCommand = exchange.prefix;
  bool firstPair = true;

  for (int i = 0; i < exchangePairsCount; i++) {
    if (!firstPair) subscribeCommand += exchange.separator;
    firstPair = false;
    subscribeCommand += exchangePairs[i];
  }
  subscribeCommand += exchange.suffix;

  Serial.println(subscribeCommand);
  ws.send(subscribeCommand);
  setProgress(8);

  // Finish setup, complete animation, set first timeout
  clearDisplay();
  setProgress(0);
  setNewTimeout();
  
  Serial.println("All set up, waiting for first trade...");
  Serial.println();
}



// clear display and reset ESP
void reboot() {
  setDashes();
  delay(1000);
  ESP.reset();
}



// wifi config callback
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered wifi portal config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}



// update timeouts
void setNewTimeout() {
  const unsigned long ts  = millis();

  // update websocket timeouts
  timeoutHardNext         = ts + timeoutHardThreshold;
  timeoutSoftNext         = ts + timeoutSoftThreshold;

  // update flashing dot
  timeoutFlashingDot      = ts + 100;
}

// END SETUP **************************************************************************************
// START MAIN LOOP ********************************************************************************

void loop() {
  unsigned long ts = millis();
  
  // Keep polling websocket 
  ws.poll(); 

  // Check for hard timeout
  if (ts >= timeoutHardNext) {
    Serial.println();
    Serial.println("TIMEOUT -> RECONNECT");
    Serial.println();
    connect(true);
  }

  // check for soft timeout (send ping)
  if (!timeoutSoftSentPing && client.connected() && 
      ts > timeoutSoftThreshold) {
    Serial.println("soft timeout -> sending ping to server");
    ws.ping();
    timeoutSoftSentPing = true;
  }

  // switch displayed pair
  if (ts > lastDisplaySwitch + switchPairMS) {
    lastDisplaySwitch = ts;
    currentPairIndex = currentPairIndex + 1;

#ifdef SHOW_CUSTOM_VALUE
    if (currentPairIndex >= exchangePairsCount) currentPairIndex = -1;
#else
    if (currentPairIndex >= exchangePairsCount) currentPairIndex = 0;    
#endif

    // update display
    if (currentPairIndex >= 0) {
      updateDisplay(exchangePairs[currentPairIndex], lastPrice[currentPairIndex]);

    // display custom value
    } else {
      updateDisplay(String("EUR"), String(lastPrice[1].toDouble() * currentEthHodl, 0));
    }
  }
  
  // flash last dot on new websocket message
  lc.setLed(0, 0, 0, millis() < timeoutFlashingDot);

  // loop delay
  delay(5);
}

// END MAIN LOOP **********************************************************************************
// START DSIPLAY FUNCTIONS ************************************************************************

// format and write display
void updateDisplay(String asset, String price) {  
#ifdef DEBUGGING
  Serial.print(asset);
  Serial.println(price);
#endif

  // make sure both variables are set
  if (asset == "" || price == "") return;
  int assetLength = 0;

#ifdef SHOW_ASSET_NAME
  // Print asset name
  assetLength = assetReservedSpace;
  wrtieStringToDisplay(asset.substring(0, maxAssetLength));
#endif

  // Use rest of space to display last price
  int remainingSpace = 8 - assetLength;

  // Remove cents from price
  price.remove(price.indexOf("."));

  // Get required space
  int requiredSpace = (int)price.length();

  // If not enough space, add k seperator
  int dotPosition = -1;
  if (remainingSpace < requiredSpace) dotPosition = remainingSpace - 3;
  if (dotPosition == remainingSpace) dotPosition = -1; // if dot is at last digit, remove

  // Print price
  int priceStartPos = min(remainingSpace, requiredSpace);
  for (int i = 0; i < priceStartPos; i++) {
    lc.setChar(0, priceStartPos - i - 1, (int)price.charAt(i), dotPosition == i);
  }
  
  // Print empty space between asset and price
  for (int i = assetLength; i > requiredSpace; i--) {
    lc.setChar(0, i, ' ', false);
  }
}

// END DSIPLAY FUNCTIONS **************************************************************************
// START DSIPLAY OUTPUT SHORTCUTS *****************************************************************

// turn display off
void clearDisplay() {
  for (int i = 0; i < 8; i++) {
    lc.setChar(0, i, ' ', false);
  }
}



// display only dashes
void setDashes() {
  for (int i = 0; i < 8; i++) {
    lc.setChar(0, i, '-', false);
  }
}



// use dots as progress bar
void setProgress(int progress) {
  for (int i = 0; i < 8; i++) {
    lc.setLed(0, 7 - i, 0, i < progress);
  }
}



// write string to display
void wrtieStringToDisplay(String s) {
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

  int len = s.length();
  if (len > 8) len = 8;
  char c;
  
  for (int i = 0; i < len; i++) {
    c = s.charAt(i);
    switch (c) {
      //case 'C':
      //  lc.setRow(0, 7 - i, B01001110);
      //  break;
      case 'I':
      case 'i':
        lc.setRow(0, 7 - i, B00110000);
        break;
      case 'N':
      case 'n':
        lc.setRow(0, 7 - i, B00010101);
        break;
      case 'O':
      case 'o':
        lc.setRow(0, 7 - i, B00011101);
        break;
      case 'P':
      case 'p':
        lc.setRow(0, 7 - i, B01100111);
        break;
      case 'R':
      case 'r':
        lc.setRow(0, 7 - i, B00000101);
        break;
      case 'S':
      case 's':
        lc.setRow(0, 7 - i, B01011011);
        break;
      case 'T':
      case 't':
        lc.setRow(0, 7 - i, B00001111);
        break;
      case 'U':
      case 'u':
        lc.setRow(0, 7 - i, B00111110);
        break;
      default:
        lc.setChar(0, 7 - i, c, false);
    }
  }
}

// END DSIPLAY OUTPUT SHORTCUTS *******************************************************************
