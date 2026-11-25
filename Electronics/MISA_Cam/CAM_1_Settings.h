#pragma once
#include <Arduino.h>

const int SERIAL_BAUD_RATE = 115200;

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
const int JPEG_QUALITY = 12;
const int FRAME_BUFFER_COUNT = 2;

const int LED_PIN = 48;
const uint8_t LED_BRIGHTNESS = 40;

const unsigned long LED_BLINK_FAST_MS = 200;
const unsigned long LED_BLINK_SLOW_MS = 600;

enum class CamState {
  BOOT,
  CAMERA_INIT,
  WIFI_CONNECT,
  WAIT_FOR_MAC,
  STREAMING,
  ERROR,
};