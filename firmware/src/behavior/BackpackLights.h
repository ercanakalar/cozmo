#pragma once
// Backpack light patterns on top of the WS2812 driver.
#include <Arduino.h>

#include "drivers/Ws2812.h"

namespace behavior {

class BackpackLights {
 public:
  enum class Pattern : uint8_t { kOff, kSolid, kBreathe, kChase, kBlink };

  void begin(drivers::Ws2812* leds, int count);
  // Base pattern, follows the robot state.
  void setBase(Pattern p, float hue, float brightness);
  // Temporary override (from animations or the server). 0 ms = until cleared.
  void setOverride(Pattern p, float hue, float brightness, uint32_t durationMs);
  void clearOverride() { overrideUntil_ = 0; overrideForever_ = false; }
  void update(uint32_t nowMs);  // ~30 Hz

  static drivers::Rgb hsv(float hue, float sat, float val);

 private:
  struct Layer {
    Pattern pattern;
    float hue;
    float brightness;
  };

  drivers::Ws2812* leds_ = nullptr;
  int count_ = 0;
  Layer base_ = {Pattern::kBreathe, 200, 0.5f};
  Layer override_ = {Pattern::kOff, 0, 0};
  uint32_t overrideUntil_ = 0;
  bool overrideForever_ = false;
  uint32_t lastShowMs_ = 0;
};

}  // namespace behavior
