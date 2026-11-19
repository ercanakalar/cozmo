#pragma once
// Hobby servo (SG90 / MG90S) driver on an ESP32 LEDC channel at 50 Hz.
#include <Arduino.h>

namespace drivers {

class ServoPwm {
 public:
  void begin(int pin, int ledcChannel);
  void writeMicroseconds(int us);
  // Stop sending pulses: the servo goes limp (saves power, stops buzzing).
  void release();
  int lastMicroseconds() const { return lastUs_; }

 private:
  static constexpr uint32_t kFreq = 50;
  static constexpr int kBits = 16;
  int pin_ = -1;
  int channel_ = 0;
  int lastUs_ = 1500;
};

}  // namespace drivers
