struct exchange_settings {
  char* id;
  char* host;
  int   port;
  char* url;
  char* wsproto;
  char* prefix;
  char* suffix;
  char* separator;
};

const exchange_settings okex = {
  "okex",
  "real.okex.com",
  443,
  "/ws/v3",
  "websocket",
  "{\"op\": \"subscribe\", \"args\": [\"index/ticker:",
  "\"]}",
  "\",\"index/ticker:"
};
