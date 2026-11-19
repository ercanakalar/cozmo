#include "BackpackLights.h"

#include <cmath>

namespace behavior {

void BackpackLights::begin(drivers::Ws2812* leds, int count) {
  leds_ = leds;
  count_ = count;
}

void BackpackLights::setBase(Pattern p, float hue, float brightness) {
  base_ = {p, hue, brightness};
}

void BackpackLights::setOverride(Pattern p, float hue, float brightness, uint32_t durationMs) {
  override_ = {p, hue, brightness};
  overrideForever_ = durationMs == 0;
  overrideUntil_ = millis() + durationMs;
}

drivers::Rgb BackpackLights::hsv(float h, float s, float v) {
  h = fmodf(fmaxf(h, 0.0f), 360.0f);
  const float c = v * s;
  const float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
  const float m = v - c;
  float r = 0, g = 0, b = 0;
  if (h < 60) { r = c; g = x; }
  else if (h < 120) { r = x; g = c; }
  else if (h < 180) { g = c; b = x; }
  else if (h < 240) { g = x; b = c; }
  else if (h < 300) { r = x; b = c; }
  else { r = c; b = x; }
  return {static_cast<uint8_t>((r + m) * 255), static_cast<uint8_t>((g + m) * 255),
          static_cast<uint8_t>((b + m) * 255)};
}

void BackpackLights::update(uint32_t now) {
  if (leds_ == nullptr || now - lastShowMs_ < 33) return;
  lastShowMs_ = now;
  const bool useOverride = overrideForever_ || static_cast<int32_t>(overrideUntil_ - now) > 0;
  const Layer& l = useOverride ? override_ : base_;

  for (int i = 0; i < count_; ++i) {
    float v = l.brightness;
    switch (l.pattern) {
      case Pattern::kOff:
        v = 0;
        break;
      case Pattern::kSolid:
        break;
      case Pattern::kBreathe:
        v *= 0.15f + 0.85f * (0.5f + 0.5f * sinf(now / 600.0f));
        break;
      case Pattern::kChase: {
        const int lit = (now / 150) % count_;
        v *= (i == lit) ? 1.0f : 0.1f;
        break;
      }
      case Pattern::kBlink:
        v *= ((now / 250) % 2) ? 1.0f : 0.0f;
        break;
    }
    leds_->set(i, hsv(l.hue, 1.0f, v));
  }
  leds_->show();
}

}  // namespace behavior
