#pragma once
#include <Arduino.h>

const int SERIAL_BAUD_RATE = 115200;

// ---- Camera clock & control ----
const int CAM_PIN_XCLK  = 15;
const int CAM_PIN_SIOD  = 4;
const int CAM_PIN_SIOC  = 5;

// ---- Camera sync ----
const int CAM_PIN_PCLK  = 13;
const int CAM_PIN_VSYNC = 6;
const int CAM_PIN_HREF  = 7;

// ---- Camera parallel data bus ----
const int CAM_PIN_D0 = 11;
const int CAM_PIN_D1 = 9;
const int CAM_PIN_D2 = 8;
const int CAM_PIN_D3 = 10;
const int CAM_PIN_D4 = 12;
const int CAM_PIN_D5 = 18;
const int CAM_PIN_D6 = 17;
const int CAM_PIN_D7 = 16;

// ---- Camera driver settings ----
const int PIN_NOT_USED = -1;
const int XCLK_FREQ_HZ = 15000000;
const int JPEG_QUALITY = 12;
const int FRAME_BUFFER_COUNT = 2;

// ---- LED Status ----
const int LED_PIN = 48;
const uint8_t LED_BRIGHTNESS = 40;
const unsigned long LED_BLINK_FAST_MS = 200;
const unsigned long LED_BLINK_SLOW_MS = 600;

// ---- Camera State ----
enum class CamState {
  INITIALIZING,
  WAITING,
  STREAMING,
  ERROR,
};