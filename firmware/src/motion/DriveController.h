#pragma once
// Closed-loop tread control: wheel speed PI loop on the encoders, distance
// moves with a deceleration ramp and IMU-assisted in-place turns.
// Falls back to open-loop automatically if the encoders stop reporting.
#include <Arduino.h>

#include "drivers/Drv8833.h"
#include "drivers/WheelEncoder.h"

namespace motion {

class DriveController {
 public:
  struct Config {
    float mmPerTick;
    float trackWidthMm;
    float maxSpeedMmps;       // speed at full PWM (feed-forward)
    float turnSlipFactor;     // tracks slip when turning; >1 = turn further
  };

  // Heading source (degrees, CCW positive). Return NAN if not available.
  using HeadingFn = float (*)();

  void begin(drivers::Drv8833* motors, drivers::WheelEncoder* left, drivers::WheelEncoder* right,
             const Config& config, HeadingFn heading);

  // Wheel speeds in mm/s. durationMs = 0 means "until told otherwise".
  void drive(float leftMmps, float rightMmps, uint32_t durationMs);
  void moveDistance(float mm, float speedMmps);
  void turnInPlace(float degrees, float speedDps);
  void stop();
  void emergencyStop();  // brake hard (picked up, link lost)

  void update(uint32_t nowMs);  // call at 50..100 Hz
  bool busy() const { return mode_ != Mode::kIdle; }
  float speedLeft() const { return measured_[0]; }
  float speedRight() const { return measured_[1]; }
  bool encodersOk() const { return encodersOk_; }

 private:
  enum class Mode { kIdle, kVelocity, kDistance, kTurn };

  void wheelLoop(float dt);

  drivers::Drv8833* motors_ = nullptr;
  drivers::WheelEncoder* enc_[2] = {nullptr, nullptr};
  Config config_ = {};
  HeadingFn heading_ = nullptr;

  Mode mode_ = Mode::kIdle;
  float target_[2] = {0, 0};    // commanded wheel speed (mm/s), signed
  float measured_[2] = {0, 0};  // measured speed, signed by command
  float integral_[2] = {0, 0};
  uint32_t lastUpdateMs_ = 0;
  uint32_t endMs_ = 0;

  // Distance / turn bookkeeping
  float travelled_ = 0;     // mm (distance) or deg (turn)
  float goal_ = 0;
  float cruise_ = 0;
  float headingStart_ = NAN;

  bool encodersOk_ = true;
  uint32_t noTicksSinceMs_ = 0;
};

}  // namespace motion
