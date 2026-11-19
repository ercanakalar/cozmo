#include "TouchPad.h"

namespace drivers {

void TouchPad::begin(int pin) {
  pin_ = pin;
  float sum = 0;
  for (int i = 0; i < 16; ++i) {
    sum += touchRead(pin_);
    delay(5);
  }
  baseline_ = sum / 16.0f;
}

void TouchPad::update() {
  const float v = touchRead(pin_);
  // ESP32 touch value DROPS when touched.
  const bool now = v < baseline_ * 0.75f;
  if (!now) baseline_ = baseline_ * 0.995f + v * 0.005f;  // track humidity drift
  if (now && !touched_) touchStartMs_ = millis();
  touched_ = now;
}

}  // namespace drivers
