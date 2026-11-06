// ==========================================
// CameraWebServer - serves the camera over HTTP
//   /capture -> single JPEG
//   /stream  -> live MJPEG video
// ==========================================
class CameraWebServer {
public:
  CameraWebServer(ICamera& camera) : _camera(camera), _server(nullptr) {}

  bool begin(uint16_t port) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    if (httpd_start(&_server, &config) != ESP_OK) {
      return false;
    }
    registerRoute("/capture", captureHandler);
    registerRoute("/stream", streamHandler);
    return true;
  }

private:
  void registerRoute(const char* uri, esp_err_t (*handler)(httpd_req_t*)) {
    httpd_uri_t route = {};
    route.uri      = uri;
    route.method   = HTTP_GET;
    route.handler  = handler;
    route.user_ctx = this;
    httpd_register_uri_handler(_server, &route);
  }

  static esp_err_t captureHandler(httpd_req_t* req) {
    CameraWebServer* self = (CameraWebServer*)req->user_ctx;
    return self->sendCapture(req);
  }

  static esp_err_t streamHandler(httpd_req_t* req) {
    CameraWebServer* self = (CameraWebServer*)req->user_ctx;
    return self->sendStream(req);
  }

  esp_err_t sendCapture(httpd_req_t* req) {
    camera_fb_t* frame = _camera.capture();
    if (!frame) {
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    httpd_resp_set_type(req, "image/jpeg");
    esp_err_t result = httpd_resp_send(req, (const char*)frame->buf, frame->len);
    _camera.release(frame);
    return result;
  }

  esp_err_t sendStream(httpd_req_t* req) {
    httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
    while (sendStreamFrame(req) == ESP_OK) {
    }
    return ESP_OK;   // client disconnected
  }

  esp_err_t sendStreamFrame(httpd_req_t* req) {
    camera_fb_t* frame = _camera.capture();
    if (!frame) {
      return ESP_FAIL;
    }
    char header[96];
    int headerLength = snprintf(header, sizeof(header),
      "\r\n--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
      (unsigned)frame->len);

    esp_err_t result = httpd_resp_send_chunk(req, header, headerLength);
    if (result == ESP_OK) {
      result = httpd_resp_send_chunk(req, (const char*)frame->buf, frame->len);
    }
    _camera.release(frame);
    return result;
  }

  ICamera&       _camera;
  httpd_handle_t _server;
};

// ==========================================
// Objects
// ==========================================
CameraWebServer webServer(camera);

// ==========================================
// Called from MISA_Cam.ino
// ==========================================
void webServerSetup() {
  if (webServer.begin(WEB_SERVER_PORT)) {
    Serial.println("Web server OK");
  } else {
    Serial.println("Web server FAILED");
  }
}