#include "Mpu6050.h"

#include <cmath>

namespace drivers {

namespace {
constexpr uint8_t kRegPwrMgmt1 = 0x6B;
constexpr uint8_t kRegConfig = 0x1A;
constexpr uint8_t kRegGyroConfig = 0x1B;
constexpr uint8_t kRegAccelConfig = 0x1C;
constexpr uint8_t kRegAccelX = 0x3B;
constexpr uint8_t kRegWhoAmI = 0x75;
constexpr float kAccelLsbPerG = 8192.0f;  // +-4 g
constexpr float kGyroLsbPerDps = 65.5f;   // +-500 dps
}  // namespace

void Mpu6050::write(uint8_t reg, uint8_t value) {
  wire_->beginTransmission(address_);
  wire_->write(reg);
  wire_->write(value);
  wire_->endTransmission();
}

bool Mpu6050::begin(TwoWire& wire, uint8_t address) {
  wire_ = &wire;
  address_ = address;
  wire_->beginTransmission(address_);
  wire_->write(kRegWhoAmI);
  if (wire_->endTransmission(false) != 0) return false;
  if (wire_->requestFrom(address_, static_cast<uint8_t>(1)) != 1) return false;
  const uint8_t who = wire_->read();
  // 0x68 = MPU6050, 0x70/0x71/0x73 = common MPU6500/9250 clones (same registers).
  if (who != 0x68 && who != 0x70 && who != 0x71 && who != 0x73 && who != 0x72) return false;
  write(kRegPwrMgmt1, 0x01);     // wake, PLL with gyro X clock
  write(kRegConfig, 0x03);       // DLPF ~44 Hz
  write(kRegGyroConfig, 0x08);   // +-500 dps
  write(kRegAccelConfig, 0x08);  // +-4 g
  present_ = true;
  lastUs_ = micros();
  return true;
}

bool Mpu6050::readRaw(int16_t* out7) {
  wire_->beginTransmission(address_);
  wire_->write(kRegAccelX);
  if (wire_->endTransmission(false) != 0) return false;
  if (wire_->requestFrom(address_, static_cast<uint8_t>(14)) != 14) return false;
  for (int i = 0; i < 7; ++i) {
    const uint8_t hi = wire_->read();
    const uint8_t lo = wire_->read();
    out7[i] = static_cast<int16_t>((hi << 8) | lo);
  }
  return true;
}

void Mpu6050::calibrateGyro(int samples) {
  if (!present_) return;
  double sum[3] = {0, 0, 0};
  int good = 0;
  int16_t raw[7];
  for (int i = 0; i < samples; ++i) {
    if (readRaw(raw)) {
      sum[0] += raw[4];
      sum[1] += raw[5];
      sum[2] += raw[6];
      ++good;
    }
    delay(3);
  }
  if (good == 0) return;
  for (int a = 0; a < 3; ++a) gyroBias_[a] = static_cast<float>(sum[a] / good) / kGyroLsbPerDps;
  yawDeg_ = 0.0f;
  lastUs_ = micros();
}

bool Mpu6050::update() {
  if (!present_) return false;
  int16_t raw[7];
  if (!readRaw(raw)) return false;
  const uint32_t nowUs = micros();
  const float dt = (nowUs - lastUs_) * 1e-6f;
  lastUs_ = nowUs;

  sample_.ax = raw[0] / kAccelLsbPerG;
  sample_.ay = raw[1] / kAccelLsbPerG;
  sample_.az = raw[2] / kAccelLsbPerG;
  // raw[3] is temperature.
  sample_.gx = raw[4] / kGyroLsbPerDps - gyroBias_[0];
  sample_.gy = raw[5] / kGyroLsbPerDps - gyroBias_[1];
  sample_.gz = raw[6] / kGyroLsbPerDps - gyroBias_[2];

  // Yaw: integrate Z rate, ignoring sensor noise below 0.5 dps.
  if (dt > 0 && dt < 0.2f && fabsf(sample_.gz) > 0.5f) yawDeg_ += sample_.gz * dt;

  const float mag = sqrtf(sample_.ax * sample_.ax + sample_.ay * sample_.ay + sample_.az * sample_.az);
  tiltDeg_ = acosf(constrain(sample_.az / std::max(mag, 0.01f), -1.0f, 1.0f)) * 180.0f / PI;
  if (fabsf(mag - 1.0f) > 1.2f) shock_ = true;

  // Picked up = sustained deviation from "flat and still on a table".
  const uint32_t nowMs = millis();
  const bool unstable = fabsf(mag - 1.0f) > 0.18f || tiltDeg_ > 30.0f ||
                        fabsf(sample_.gx) > 60.0f || fabsf(sample_.gy) > 60.0f;
  if (unstable) {
    stableSinceMs_ = 0;
    if (unstableSinceMs_ == 0) unstableSinceMs_ = nowMs;
    if (!pickedUp_ && nowMs - unstableSinceMs_ > 350) pickedUp_ = true;
  } else {
    unstableSinceMs_ = 0;
    if (stableSinceMs_ == 0) stableSinceMs_ = nowMs;
    if (pickedUp_ && nowMs - stableSinceMs_ > 1200) pickedUp_ = false;
  }
  return true;
}

}  // namespace drivers
