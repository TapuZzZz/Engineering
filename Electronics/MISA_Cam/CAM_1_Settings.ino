struct WifiNetwork { 
  const char* ssid;
  const char* pass;
};


const WifiNetwork NETWORKS[] = {
  {"Amal-Student", "Amal1@st"},
  {"MISA-NET", "03032007"},
  {"Artyom Kiselhof’s iPhone", "03032007"},
};


const char* HOST_NAME = "MISA-CAM";
const int STREAM_PORT = 80;
const int TARGET_FPS = 20;


const unsigned long WIFI_TIMEOUT_MS = 10000;
const unsigned long WIFI_RETRY_MS = 500;


const int BAUD_RATE = 115200;


const int XCLK_GPIO = 15;
const int SIOD_GPIO = 4;
const int SIOC_GPIO = 5;
const int Y9_GPIO = 16;
const int Y8_GPIO = 17;
const int Y7_GPIO = 18;
const int Y6_GPIO = 12;
const int Y5_GPIO = 10;
const int Y4_GPIO = 8;
const int Y3_GPIO = 9;
const int Y2_GPIO = 11;
const int VSYNC_GPIO = 6;
const int HREF_GPIO = 7;
const int PCLK_GPIO = 13;


const int PIN_NOT_USED = -1;
const int XCLK_FREQ_HZ = 15000000;
const int JPEG_QUALITY = 10;
const int FRAME_BUFFER_COUNT = 2;