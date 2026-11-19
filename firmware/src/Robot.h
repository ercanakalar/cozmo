#pragma once
// Top level robot: owns every driver, runs local reflexes and personality,
// executes commands from the PC server and reports telemetry/events.
#include <Arduino.h>
#include <ArduinoJson.h>

#include "behavior/Animations.h"
#include "behavior/BackpackLights.h"
#include "drivers/Battery.h"
#include "drivers/Drv8833.h"
#include "drivers/I2sMicrophone.h"
#include "drivers/I2sSpeaker.h"
#include "drivers/Mpu6050.h"
#include "drivers/ServoPwm.h"
#include "drivers/Ssd1306.h"
#include "drivers/TouchPad.h"
#include "drivers/WheelEncoder.h"
#include "drivers/Ws2812.h"
#include "face/Face.h"
#include "motion/DriveController.h"
#include "motion/SmoothJoint.h"
#include "net/AudioStreamer.h"
#include "net/Link.h"

class Robot {
 public:
  // What the conversation pipeline on the PC is doing (sent by the server).
  enum class Mode : uint8_t { kIdle, kListening, kThinking, kSpeaking };

  static Robot& instance();

  void begin();
  void loop();  // control loop, called from Arduino loop() at ~100 Hz
  void displayTask();  // runs forever: IMU + face rendering (owns the I2C bus)

 private:
  Robot();

  // Command handling (server -> robot)
  static void onMessage(JsonDocument& msg);
  void handleCommand(JsonDocument& msg);
  void sendDone(const char* what, int32_t id);
  void sendEvent(const char* name);
  void sendTelemetry(uint32_t now);
  void trackPending(uint32_t now);

  // Personality / local reflexes
  void updateReflexes(uint32_t now);
  void updateIdle(uint32_t now);
  void setMode(Mode m);
  void noteInteraction(uint32_t now) { lastInteractionMs_ = now; }
  void playAnimation(const char* name, int32_t id = -1);
  static void executeStep(const behavior::Step& step);
  void wake();

  static float headingDeg();
  static IPAddress serverIp();
  static uint16_t micFlags();

  // Drivers
  drivers::Ssd1306 oled_;
  drivers::Mpu6050 imu_;
  drivers::I2sMicrophone mic_;
  drivers::I2sSpeaker speaker_;
  drivers::Drv8833 motors_;
  drivers::WheelEncoder encLeft_, encRight_;
  drivers::ServoPwm headServo_, liftServo_;
  drivers::Ws2812 leds_;
  drivers::Battery battery_;
  drivers::TouchPad touch_;

  // Subsystems
  face::Face face_;
  motion::DriveController drive_;
  motion::SmoothJoint head_, lift_;
  behavior::BackpackLights lights_;
  behavior::AnimationPlayer anim_;
  net::Link link_;
  net::AudioStreamer audio_;

  // Shared with the display task
  volatile float yawDeg_ = 0;
  volatile bool imuOk_ = false;
  volatile bool pickedUp_ = false;
  volatile bool shock_ = false;

  // State
  Mode mode_ = Mode::kIdle;
  bool sleeping_ = false;
  bool wasPickedUp_ = false;
  bool wasTouched_ = false;
  bool wasLinked_ = false;
  bool lowBatteryWarned_ = false;
  uint32_t lastInteractionMs_ = 0;
  uint32_t nextGlanceMs_ = 0;
  uint32_t lastTelemetryMs_ = 0;
  uint32_t lastSlowUpdateMs_ = 0;
  uint32_t lastLoudMs_ = 0;
  uint32_t expressionUntilMs_ = 0;
  bool boredPlayed_ = false;

  // Pending completion ids (-1 = none)
  int32_t driveId_ = -1, headId_ = -1, liftId_ = -1, animId_ = -1;
};
