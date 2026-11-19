#pragma once
// WS2812B ("NeoPixel") driver using the ESP32 RMT peripheral.
// Drives the backpack status lights.
#include <Arduino.h>
#include <driver/rmt.h>

namespace drivers {

struct Rgb {
  uint8_t r, g, b;
};

class Ws2812 {
 public:
  static constexpr int kMaxLeds = 8;
  bool begin(int pin, int count, rmt_channel_t channel = RMT_CHANNEL_0);
  void set(int index, Rgb color);
  void fill(Rgb color);
  void setBrightness(uint8_t b) { brightness_ = b; }
  void show();

 private:
  rmt_channel_t channel_ = RMT_CHANNEL_0;
  int count_ = 0;
  uint8_t brightness_ = 80;
  Rgb pixels_[kMaxLeds] = {};
  rmt_item32_t items_[kMaxLeds * 24];
  bool ok_ = false;
};

}  // namespace drivers
