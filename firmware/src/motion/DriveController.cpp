#include "DriveController.h"

#include <cmath>

namespace motion {

namespace {
constexpr float kKp = 0.0025f;  // PWM per mm/s of error
constexpr float kKi = 0.0060f;
constexpr float kMinSpeed = 25.0f;  // mm/s, below this the N20s stall
constexpr float kDecel = 400.0f;    // mm/s^2 for distance moves

float sgn(float v) { return (v > 0) - (v < 0); }
}  // namespace

void DriveController::begin(drivers::Drv8833* motors, drivers::WheelEncoder* left,
                            drivers::WheelEncoder* right, const Config& config, HeadingFn heading) {
  motors_ = motors;
  enc_[0] = left;
  enc_[1] = right;
  config_ = config;
  heading_ = heading;
  lastUpdateMs_ = millis();
}

void DriveController::drive(float leftMmps, float rightMmps, uint32_t durationMs) {
  const float maxV = config_.maxSpeedMmps;
  target_[0] = constrain(leftMmps, -maxV, maxV);
  target_[1] = constrain(rightMmps, -maxV, maxV);
  endMs_ = durationMs ? millis() + durationMs : 0;
  mode_ = (target_[0] == 0 && target_[1] == 0) ? Mode::kIdle : Mode::kVelocity;
  if (mode_ == Mode::kIdle) stop();
}

void DriveController::moveDistance(float mm, float speedMmps) {
  if (fabsf(mm) < 1.0f) return;
  goal_ = fabsf(mm);
  travelled_ = 0;
  cruise_ = constrain(fabsf(speedMmps), kMinSpeed, config_.maxSpeedMmps) * sgn(mm);
  target_[0] = target_[1] = cruise_;
  endMs_ = 0;
  mode_ = Mode::kDistance;
}

void DriveController::turnInPlace(float degrees, float speedDps) {
  if (fabsf(degrees) < 1.0f) return;
  goal_ = fabsf(degrees);
  travelled_ = 0;
  headingStart_ = heading_ ? heading_() : NAN;
  // Wheel speed needed for the requested rotation rate (CCW positive -> right wheel forward).
  const float wheel = fabsf(speedDps) * PI / 180.0f * config_.trackWidthMm / 2.0f;
  cruise_ = constrain(wheel, kMinSpeed * 1.5f, config_.maxSpeedMmps) * sgn(degrees);
  target_[0] = -cruise_;
  target_[1] = cruise_;
  endMs_ = 0;
  mode_ = Mode::kTurn;
}

void DriveController::stop() {
  mode_ = Mode::kIdle;
  target_[0] = target_[1] = 0;
  integral_[0] = integral_[1] = 0;
  motors_->coast();
}

void DriveController::emergencyStop() {
  stop();
  motors_->brake();
}

void DriveController::wheelLoop(float dt) {
  for (int i = 0; i < 2; ++i) {
    const float ff = target_[i] / config_.maxSpeedMmps;
    float pwm = ff;
    if (encodersOk_) {
      const float err = target_[i] - measured_[i];
      integral_[i] = constrain(integral_[i] + err * dt, -60.0f, 60.0f);
      pwm += kKp * err + kKi * integral_[i];
    }
    // Small boost to overcome static friction of the gearbox.
    if (target_[i] != 0 && fabsf(pwm) < 0.18f) pwm = 0.18f * sgn(target_[i]);
    if (target_[i] == 0) pwm = 0;
    if (i == 0) motors_->left().set(pwm); else motors_->right().set(pwm);
  }
}

void DriveController::update(uint32_t now) {
  const float dt = (now - lastUpdateMs_) / 1000.0f;
  if (dt <= 0.0f) return;
  lastUpdateMs_ = now;

  // Measure wheel speeds (encoders give magnitude; sign follows the command).
  float stepMm[2];
  bool anyTicks = false;
  for (int i = 0; i < 2; ++i) {
    const uint32_t ticks = enc_[i] ? enc_[i]->takeDelta() : 0;
    anyTicks |= ticks > 0;
    stepMm[i] = ticks * config_.mmPerTick * sgn(target_[i]);
    measured_[i] = 0.6f * measured_[i] + 0.4f * (stepMm[i] / dt);
  }

  // Encoder health: driving for 600 ms without a single tick -> open loop.
  if (mode_ != Mode::kIdle && !anyTicks) {
    if (noTicksSinceMs_ == 0) noTicksSinceMs_ = now;
    if (encodersOk_ && now - noTicksSinceMs_ > 600) {
      encodersOk_ = false;
      log_w("No encoder ticks: switching to open-loop drive");
    }
  } else if (anyTicks) {
    noTicksSinceMs_ = 0;
    encodersOk_ = true;
  }

  switch (mode_) {
    case Mode::kIdle:
      return;

    case Mode::kVelocity:
      if (endMs_ != 0 && static_cast<int32_t>(now - endMs_) >= 0) {
        stop();
        return;
      }
      break;

    case Mode::kDistance: {
      if (encodersOk_) {
        travelled_ += fabsf(stepMm[0] + stepMm[1]) / 2.0f;
      } else {
        travelled_ += fabsf(cruise_) * dt;  // dead reckoning
      }
      const float remaining = goal_ - travelled_;
      if (remaining <= 0) {
        emergencyStop();
        return;
      }
      const float v = std::max(kMinSpeed, std::min(fabsf(cruise_), sqrtf(2 * kDecel * remaining)));
      target_[0] = target_[1] = v * sgn(cruise_);
      break;
    }

    case Mode::kTurn: {
      const float h = heading_ ? heading_() : NAN;
      if (!isnan(h) && !isnan(headingStart_)) {
        travelled_ = fabsf(h - headingStart_);
      } else if (encodersOk_) {
        const float arc = (fabsf(stepMm[0]) + fabsf(stepMm[1])) / 2.0f;
        travelled_ += arc / (config_.trackWidthMm / 2.0f) * 180.0f / PI / config_.turnSlipFactor;
      } else {
        travelled_ += fabsf(cruise_) / (config_.trackWidthMm / 2.0f) * 180.0f / PI * dt /
                      config_.turnSlipFactor;
      }
      const float remaining = goal_ - travelled_;
      if (remaining <= 0.5f) {
        emergencyStop();
        return;
      }
      // Slow down for the last 25 degrees to avoid overshoot.
      const float scale = constrain(remaining / 25.0f, 0.45f, 1.0f);
      target_[0] = -cruise_ * scale;
      target_[1] = cruise_ * scale;
      break;
    }
  }
  wheelLoop(dt);
}

}  // namespace motion
