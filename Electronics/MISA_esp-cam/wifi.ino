void initWiFi() {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_STA);
}

bool waitForConnection() {
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > WIFI_TIMEOUT_MS) return false;
    delay(250);
  }
  return true;
}

bool tryConnect(const char* ssid, const char* password) {
  Serial.printf("WiFi => Trying %s\n", ssid);
  WiFi.begin(ssid, password);

  if (waitForConnection()) return true;

  WiFi.disconnect();
  return false;
}

bool connectWiFi() {
  for (int i = 0; i < WIFI_NETWORK_COUNT; i++) {
    if (tryConnect(WIFI_NETWORKS[i].ssid, WIFI_NETWORKS[i].password)) {
      Serial.printf("WiFi => Connected to %s (%s)\n",
                    WIFI_NETWORKS[i].ssid,
                    WiFi.localIP().toString().c_str());
      return true;
    }
  }
  return false;
}

bool ensureNetwork() {
  if (WiFi.status() == WL_CONNECTED) return true;

  Serial.println("WiFi => Connection lost");
  showErrorLed();

  if (!connectWiFi()) return false;
  return restartMdns();
}