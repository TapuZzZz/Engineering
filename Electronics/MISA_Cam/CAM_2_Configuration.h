#pragma once

camera_config_t buildCameraConfig() {
  camera_config_t config = {};

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO;
  config.pin_d1 = Y3_GPIO;
  config.pin_d2 = Y4_GPIO;
  config.pin_d3 = Y5_GPIO;
  config.pin_d4 = Y6_GPIO;
  config.pin_d5 = Y7_GPIO;
  config.pin_d6 = Y8_GPIO;
  config.pin_d7 = Y9_GPIO;

  config.pin_xclk = XCLK_GPIO;
  config.pin_pclk = PCLK_GPIO;
  config.pin_vsync = VSYNC_GPIO;
  config.pin_href = HREF_GPIO;

  config.pin_sccb_sda = SIOD_GPIO;
  config.pin_sccb_scl = SIOC_GPIO;

  config.pin_pwdn = PIN_NOT_USED;
  config.pin_reset = PIN_NOT_USED;

  config.xclk_freq_hz = XCLK_FREQ_HZ;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = JPEG_QUALITY;

  config.fb_count = FRAME_BUFFER_COUNT;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  return config;
}


bool initCameraConfig() {
  camera_config_t config = buildCameraConfig();
  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {
    Serial.printf("[initCameraConfig] Camera error: 0x%x (%s)\n", err, esp_err_to_name(err));
  }

  return err == ESP_OK;
}


bool initWiFiConnection() {
  for (int i = 0; i < NETWORKS_COUNT && WiFi.status() != WL_CONNECTED; i++) {
    WiFi.disconnect();
    Serial.printf("[initWiFiConnection] Trying: %s\n", NETWORKS[i].ssid);
    WiFi.begin(NETWORKS[i].ssid, NETWORKS[i].pass);
    unsigned long startMs = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startMs < WIFI_TIMEOUT_MS) {
      delay(WIFI_RETRY_MS);
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[initWiFiConnection] WiFi connection failed!");
    return false;
  }
  
  WiFi.setSleep(false);

  Serial.print("[+] Camera IP is: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin(HOST_NAME)) {
    Serial.println("[initWiFiConnection] MDNS start failed!");
    return false;
  }
  Serial.printf("[+] MDNS Name: %s.local\n", HOST_NAME);
  return true;
}