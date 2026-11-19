#pragma once
// MPU6050 6-axis IMU driver: raw read, gyro bias calibration, yaw integration
// (for accurate tread turns) and "picked up" detection.
#include <Arduino.h>
#include <Wire.h>

namespace drivers {

class Mpu6050 {
 public:
  struct Sample {
    float ax, ay, az;  // g
    float gx, gy, gz;  // deg/s
  };

  bool begin(TwoWire& wire, uint8_t address);
  bool present() const { return present_; }
  // Robot must be still. Takes ~ samples * 3 ms.
  void calibrateGyro(int samples = 300);
  // Read a sample and update yaw + pickup state. Call at >= 25 Hz.
  bool update();

  const Sample& sample() const { return sample_; }
  float yawDeg() const { return yawDeg_; }
  float tiltDeg() const { return tiltDeg_; }
  bool pickedUp() const { return pickedUp_; }
  // Large sudden acceleration (bumped / dropped / slapped).
  bool shockDetected() { bool s = shock_; shock_ = false; return s; }

 private:
  bool readRaw(int16_t* out7);
  void write(uint8_t reg, uint8_t value);

  TwoWire* wire_ = nullptr;
  uint8_t address_ = 0x68;
  bool present_ = false;
  Sample sample_ = {};
  float gyroBias_[3] = {0, 0, 0};
  float yawDeg_ = 0.0f;
  float tiltDeg_ = 0.0f;
  uint32_t lastUs_ = 0;
  bool pickedUp_ = false;
  bool shock_ = false;
  uint32_t unstableSinceMs_ = 0;
  uint32_t stableSinceMs_ = 0;
};

}  // namespace drivers
