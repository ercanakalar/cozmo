#pragma once
// 1S LiPo voltage monitor through a resistor divider on an ADC1 pin.
#include <Arduino.h>

namespace drivers {

class Battery {
 public:
  void begin(int pin, float dividerRatio);
  void update();  // call ~10 Hz
  float volts() const { return volts_; }
  int percent() const;

 private:
  int pin_ = -1;
  float ratio_ = 2.0f;
  float volts_ = 0.0f;
};

}  // namespace drivers
