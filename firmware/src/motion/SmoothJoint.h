#pragma once
// A servo-driven joint (head tilt or lift) with eased, timed motion in real
// units (degrees for the head, millimetres of fork height for the lift).
#include <Arduino.h>

#include "drivers/ServoPwm.h"

namespace motion {

class SmoothJoint {
 public:
  struct Calibration {
    float minValue, maxValue;  // user units
    int usAtMin, usAtMax;      // servo pulse at those limits
  };

  void begin(drivers::ServoPwm* servo, const Calibration& cal, float initial);
  // Move to `value` in `durationMs` with ease-in-out. 0 ms = as fast as possible.
  void moveTo(float value, uint32_t durationMs);
  void update(uint32_t nowMs);
  bool busy() const { return busy_; }
  float value() const { return current_; }
  float target() const { return to_; }
  // Release the servo after it has been idle for a while (quiet + cool).
  void setIdleRelease(uint32_t ms) { idleReleaseMs_ = ms; }

 private:
  int toMicroseconds(float value) const;

  drivers::ServoPwm* servo_ = nullptr;
  Calibration cal_ = {};
  float from_ = 0, to_ = 0, current_ = 0;
  uint32_t startMs_ = 0, durationMs_ = 0;
  bool busy_ = false;
  bool released_ = false;
  uint32_t idleSinceMs_ = 0;
  uint32_t idleReleaseMs_ = 0;  // 0 = never release
};

}  // namespace motion
