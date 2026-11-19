#include "SmoothJoint.h"

namespace motion {

namespace {
float easeInOut(float t) {
  return t < 0.5f ? 4 * t * t * t : 1 - powf(-2 * t + 2, 3) / 2;
}
}  // namespace

void SmoothJoint::begin(drivers::ServoPwm* servo, const Calibration& cal, float initial) {
  servo_ = servo;
  cal_ = cal;
  from_ = to_ = current_ = constrain(initial, cal_.minValue, cal_.maxValue);
  servo_->writeMicroseconds(toMicroseconds(current_));
  idleSinceMs_ = millis();
}

int SmoothJoint::toMicroseconds(float value) const {
  const float t = (value - cal_.minValue) / (cal_.maxValue - cal_.minValue);
  return static_cast<int>(cal_.usAtMin + t * (cal_.usAtMax - cal_.usAtMin));
}

void SmoothJoint::moveTo(float value, uint32_t durationMs) {
  value = constrain(value, cal_.minValue, cal_.maxValue);
  from_ = current_;
  to_ = value;
  startMs_ = millis();
  // A hobby servo moves ~60 deg in 100 ms; never ask for faster than that.
  const float span = fabsf(cal_.maxValue - cal_.minValue);
  const uint32_t minMs = static_cast<uint32_t>(fabsf(to_ - from_) / span * 250.0f);
  durationMs_ = std::max(durationMs, minMs);
  busy_ = true;
  released_ = false;
}

void SmoothJoint::update(uint32_t now) {
  if (servo_ == nullptr) return;
  if (busy_) {
    const float t = durationMs_ == 0 ? 1.0f : std::min(1.0f, (now - startMs_) / static_cast<float>(durationMs_));
    current_ = from_ + (to_ - from_) * easeInOut(t);
    servo_->writeMicroseconds(toMicroseconds(current_));
    if (t >= 1.0f) {
      busy_ = false;
      idleSinceMs_ = now;
    }
    return;
  }
  if (idleReleaseMs_ > 0 && !released_ && now - idleSinceMs_ > idleReleaseMs_) {
    servo_->release();
    released_ = true;
  }
}

}  // namespace motion
