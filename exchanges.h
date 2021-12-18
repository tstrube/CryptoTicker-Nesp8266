// exchange struct used in script
struct exchangeSettings {
  char*  url;
  String prefix;
  String suffix;
  String separator;
  String asset;
  String price;
};

const exchangeSettings coinbase = {
  "wss://ws-feed.exchange.coinbase.com",
  "{\"type\": \"subscribe\", \"channels\": [\"ticker\"], \"product_ids\": [\"",
  "\"]}",
  "\", \"",
  "product_id",
  "price"
};
