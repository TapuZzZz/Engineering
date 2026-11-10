WiFiMulti wifiMulti;
httpd_handle_t streamServer = NULL;

const char* STREAM_TYPE = "multipart/x-mixed-replace;boundary=frame";
const char* STREAM_BOUNDARY = "\r\n--frame\r\n";
const char* PART_FORMAT = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

void connectWifi() {
  for (const WifiNetwork& n : NETWORKS) wifiMulti.addAP(n.ssid, n.pass);
  while (wifiMulti.run() != WL_CONNECTED) delay(500);
}

void startMdns() {
  if (MDNS.begin(HOST_NAME)) Serial.println("Name: MISA-CAM.local");
}

void limitFps(unsigned long& lastMs) {
  unsigned long interval = 1000 / TARGET_FPS;
  unsigned long elapsed = millis() - lastMs;
  if (elapsed < interval) delay(interval - elapsed);
  lastMs = millis();
}

esp_err_t sendFrame(httpd_req_t* req, camera_fb_t* fb) {
  char header[64];
  size_t n = snprintf(header, sizeof(header), PART_FORMAT, (unsigned)fb->len);
  esp_err_t res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
  if (res == ESP_OK) res = httpd_resp_send_chunk(req, header, n);
  if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
  return res;
}

esp_err_t streamHandler(httpd_req_t* req) {
  httpd_resp_set_type(req, STREAM_TYPE);
  unsigned long lastFrameMs = 0;
  while (true) {
    limitFps(lastFrameMs);
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) return ESP_FAIL;
    esp_err_t res = sendFrame(req, fb);
    esp_camera_fb_return(fb);
    if (res != ESP_OK) return res;
  }
}

bool startStreamServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = STREAM_PORT;
  httpd_uri_t uri = {};
  uri.uri = "/stream";
  uri.method = HTTP_GET;
  uri.handler = streamHandler;
  if (httpd_start(&streamServer, &config) != ESP_OK) return false;
  return httpd_register_uri_handler(streamServer, &uri) == ESP_OK;
}