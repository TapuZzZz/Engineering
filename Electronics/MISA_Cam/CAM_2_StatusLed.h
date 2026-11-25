#pragma once
#include <Arduino.h>
#include "CAM_1_Settings.h"

struct LedColor {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

struct LedPattern {
  LedColor color;
  bool isBlinking;
  unsigned long blinkIntervalMs;
};

const LedColor COLOR_BOOT = {LED_BRIGHTNESS, LED_BRIGHTNESS, 0};
const LedColor COLOR_CAMERA = {LED_BRIGHTNESS, 0, LED_BRIGHTNESS};
const LedColor COLOR_WIFI = {0, 0, LED_BRIGHTNESS};
const LedColor COLOR_WAIT_MAC = {0, LED_BRIGHTNESS, LED_BRIGHTNESS};
const LedColor COLOR_STREAMING = {0, LED_BRIGHTNESS, 0};
const LedColor COLOR_ERROR = {LED_BRIGHTNESS, 0, 0};

LedPattern getLedPattern(CamState state) {
  switch (state) {
    case CamState::BOOT:
      return {COLOR_BOOT, false, 0};
    case CamState::CAMERA_INIT:
      return {COLOR_CAMERA, false, 0};
    case CamState::WIFI_CONNECT:
      return {COLOR_WIFI, true, LED_BLINK_FAST_MS};
    case CamState::WAIT_FOR_MAC:
      return {COLOR_WAIT_MAC, true, LED_BLINK_SLOW_MS};
    case CamState::STREAMING:
      return {COLOR_STREAMING, false, 0};
    default:
      return {COLOR_ERROR, false, 0};
  }
}