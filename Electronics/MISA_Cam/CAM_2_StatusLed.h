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

LedColor const COLOR_ERROR = {LED_BRIGHTNESS, 0, 0};

LedPattern getLedPattern(CamState state) {
  switch (state) {
    case CamState::BOOT:
      return {COLOR_BOOT, false, 0};
    
    default:
      return {COLOR_ERROR, false, 0};
  }
}