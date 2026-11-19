#include "Battery.h"

namespace drivers {

void Battery::begin(int pin, float dividerRatio) {
  pin_ = pin;
  ratio_ = dividerRatio;
  analogSetPinAttenuation(pin_, ADC_11db);
  volts_ = analogReadMilliVolts(pin_) * ratio_ / 1000.0f;
}

void Battery::update() {
  const float v = analogReadMilliVolts(pin_) * ratio_ / 1000.0f;
  volts_ = volts_ * 0.9f + v * 0.1f;  // motors make the reading jumpy
}

int Battery::percent() const {
  // Rough resting-voltage curve for a 1S LiPo.
  static const float kVolts[] = {3.30f, 3.50f, 3.65f, 3.73f, 3.80f, 3.87f, 3.95f, 4.05f, 4.15f};
  static const int kPct[] = {0, 5, 15, 30, 45, 60, 75, 90, 100};
  constexpr int n = sizeof(kVolts) / sizeof(kVolts[0]);
  if (volts_ <= kVolts[0]) return 0;
  if (volts_ >= kVolts[n - 1]) return 100;
  for (int i = 1; i < n; ++i) {
    if (volts_ < kVolts[i]) {
      const float t = (volts_ - kVolts[i - 1]) / (kVolts[i] - kVolts[i - 1]);
      return kPct[i - 1] + static_cast<int>(t * (kPct[i] - kPct[i - 1]));
    }
  }
  return 100;
}

}  // namespace drivers
