#include "Robot.h"

#include <Wire.h>

#include "config.h"
#include "net/Protocol.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#warning "include/secrets.h not found - using placeholder WiFi credentials"
#endif

namespace {
constexpr uint32_t kSleepAfterMs = 6UL * 60UL * 1000UL;
constexpr uint32_t kBoredAfterMs = 3UL * 60UL * 1000UL;
constexpr int kLoudRms = 7000;
constexpr float kMaxSpeedMmps = 220.0f;
constexpr float kTurnSlipFactor = 1.15f;
constexpr float kImuYawSign = 1.0f;  // flip if turns go the wrong way with the IMU fitted
}  // namespace

Robot& Robot::instance() {
  static Robot robot;
  return robot;
}

Robot::Robot() : anim_(&Robot::executeStep) {}

// --------------------------------------------------------------------------
//  Setup
// --------------------------------------------------------------------------
void Robot::begin() {
  // Motors first so the treads are guaranteed off during the rest of boot.
  motors_.begin({cfg::PIN_MOT_L_IN1, cfg::PIN_MOT_L_IN2, cfg::PIN_MOT_R_IN1, cfg::PIN_MOT_R_IN2},
                cfg::MOTOR_PWM_HZ, cfg::MOTOR_L_INVERT, cfg::MOTOR_R_INVERT);

  Wire.begin(cfg::PIN_I2C_SDA, cfg::PIN_I2C_SCL, cfg::I2C_FREQ_HZ);
  if (!oled_.begin(Wire, cfg::OLED_ADDR,
                   cfg::OLED_IS_SH1106 ? drivers::Ssd1306::Controller::kSh1106
                                       : drivers::Ssd1306::Controller::kSsd1306)) {
    log_e("OLED not found at 0x%02X", cfg::OLED_ADDR);
  }
  face_.begin(&oled_);
  face_.showText("COZMO", "BOOTING...", 1500);

  leds_.begin(cfg::PIN_LEDS, cfg::NUM_LEDS);
  lights_.begin(&leds_, cfg::NUM_LEDS);
  lights_.setBase(behavior::BackpackLights::Pattern::kChase, 200, 0.6f);

  imuOk_ = imu_.begin(Wire, cfg::IMU_ADDR);
  if (imuOk_) {
    imu_.calibrateGyro();  // keep the robot still while booting
  } else {
    log_w("IMU not found - pickup detection and gyro turns disabled");
  }

  headServo_.begin(cfg::PIN_SERVO_HEAD, 4);
  liftServo_.begin(cfg::PIN_SERVO_LIFT, 5);
  head_.begin(&headServo_, {cfg::HEAD_MIN_DEG, cfg::HEAD_MAX_DEG, cfg::HEAD_SERVO_US_AT_MIN,
                            cfg::HEAD_SERVO_US_AT_MAX}, 10.0f);
  lift_.begin(&liftServo_, {cfg::LIFT_MIN_MM, cfg::LIFT_MAX_MM, cfg::LIFT_SERVO_US_AT_MIN,
                            cfg::LIFT_SERVO_US_AT_MAX}, 0.0f);
  lift_.setIdleRelease(1500);  // the lift rests on its stop, no need to hold it

  encLeft_.begin(cfg::PIN_ENC_L);
  encRight_.begin(cfg::PIN_ENC_R);
  const float mmPerTick = PI * cfg::TREAD_SPROCKET_DIAMETER_MM / cfg::ENC_TICKS_PER_WHEEL_REV;
  drive_.begin(&motors_, &encLeft_, &encRight_,
               {mmPerTick, cfg::TRACK_WIDTH_MM, kMaxSpeedMmps, kTurnSlipFactor}, &Robot::headingDeg);

  battery_.begin(cfg::PIN_BATTERY, cfg::BATTERY_DIVIDER);
  touch_.begin(cfg::PIN_TOUCH);

  if (!mic_.begin(I2S_NUM_0, {cfg::PIN_MIC_BCLK, cfg::PIN_MIC_WS, cfg::PIN_MIC_SD},
                  cfg::MIC_SAMPLE_RATE, cfg::MIC_GAIN_SHIFT)) {
    log_e("Microphone init failed");
  }
  if (!speaker_.begin(I2S_NUM_1, {cfg::PIN_SPK_BCLK, cfg::PIN_SPK_LRC, cfg::PIN_SPK_DIN},
                      cfg::SPK_SAMPLE_RATE)) {
    log_e("Speaker init failed");
  }

  link_.begin(COZMO_WIFI_SSID, COZMO_WIFI_PASS, COZMO_SERVER_IP, cfg::PORT_DISCOVERY,
              cfg::PORT_CONTROL, &Robot::onMessage);
  audio_.begin(&mic_, &speaker_,
               {cfg::PORT_AUDIO_UP, cfg::PORT_AUDIO_DOWN, cfg::AUDIO_FRAME_SAMPLES,
                cfg::AUDIO_GATE_ENABLED, cfg::AUDIO_GATE_RMS, cfg::AUDIO_GATE_HANG_MS},
               &Robot::serverIp, &Robot::micFlags);

  // Display + IMU task on core 1 next to the control loop (WiFi lives on core 0).
  xTaskCreatePinnedToCore([](void*) { Robot::instance().displayTask(); }, "display", 6144, nullptr,
                          2, nullptr, 1);

  lastInteractionMs_ = millis();
  playAnimation("wake");
}

// --------------------------------------------------------------------------
//  Display / IMU task
// --------------------------------------------------------------------------
void Robot::displayTask() {
  uint32_t lastFrame = 0;
  for (;;) {
    const uint32_t now = millis();
    if (imuOk_ && imu_.update()) {
      yawDeg_ = imu_.yawDeg() * kImuYawSign;
      pickedUp_ = imu_.pickedUp();
      if (imu_.shockDetected()) shock_ = true;
    }
    if (now - lastFrame >= 40) {  // 25 fps
      lastFrame = now;
      face_.setTalkLevel(speaker_.level());
      face_.render(now);
    }
    vTaskDelay(pdMS_TO_TICKS(8));
  }
}

float Robot::headingDeg() {
  Robot& r = instance();
  return r.imuOk_ ? r.yawDeg_ : NAN;
}

IPAddress Robot::serverIp() {
  Robot& r = instance();
  return r.link_.hasServer() ? r.link_.serverIp() : IPAddress(0, 0, 0, 0);
}

uint16_t Robot::micFlags() {
  Robot& r = instance();
  const bool moving = r.drive_.busy() || r.head_.busy() || r.lift_.busy();
  return moving ? net::kFlagMotorsActive : 0;
}

// --------------------------------------------------------------------------
//  Main control loop
// --------------------------------------------------------------------------
void Robot::loop() {
  const uint32_t now = millis();

  link_.update(now);
  drive_.update(now);
  head_.update(now);
  lift_.update(now);
  anim_.update(now);
  lights_.update(now);

  if (now - lastSlowUpdateMs_ >= 50) {
    lastSlowUpdateMs_ = now;
    touch_.update();
    battery_.update();
    updateReflexes(now);
    updateIdle(now);
  }

  trackPending(now);
  if (now - lastTelemetryMs_ >= cfg::TELEMETRY_PERIOD_MS) {
    lastTelemetryMs_ = now;
    sendTelemetry(now);
  }

  // Safety: never keep driving if the brain went away.
  if (drive_.busy() && link_.connected() == false && !anim_.busy()) drive_.emergencyStop();
  if (drive_.busy() && link_.connected() && now - link_.lastRxMs() > cfg::LINK_TIMEOUT_MS &&
      !anim_.busy()) {
    drive_.emergencyStop();
  }
}

void Robot::trackPending(uint32_t now) {
  (void)now;
  if (driveId_ >= 0 && !drive_.busy()) { sendDone("drive", driveId_); driveId_ = -1; }
  if (headId_ >= 0 && !head_.busy()) { sendDone("head", headId_); headId_ = -1; }
  if (liftId_ >= 0 && !lift_.busy()) { sendDone("lift", liftId_); liftId_ = -1; }
  if (animId_ >= 0 && !anim_.busy()) { sendDone("anim", animId_); animId_ = -1; }
}

// --------------------------------------------------------------------------
//  Local reflexes: these work even without the PC server
// --------------------------------------------------------------------------
void Robot::updateReflexes(uint32_t now) {
  // Picked up -> stop treads immediately, look scared, tell the server.
  const bool picked = pickedUp_;
  if (picked && !wasPickedUp_) {
    drive_.emergencyStop();
    anim_.stop();
    face_.setExpression(face::Expression::kScared);
    face_.setLook(0, 0.5f);
    speaker_.tone(900, 1400, 120);
    lights_.setOverride(behavior::BackpackLights::Pattern::kBlink, 30, 1.0f, 0);
    sendEvent("picked_up");
    noteInteraction(now);
    if (sleeping_) wake();
  } else if (!picked && wasPickedUp_) {
    lights_.clearOverride();
    face_.setLook(0, 0);
    playAnimation("happy");
    sendEvent("put_down");
    noteInteraction(now);
  }
  wasPickedUp_ = picked;

  if (shock_) {
    shock_ = false;
    if (!picked) {
      playAnimation("surprised");
      sendEvent("bumped");
    }
  }

  // Petting the head.
  const bool touched = touch_.touched();
  if (touched && !wasTouched_) {
    noteInteraction(now);
    if (sleeping_) {
      wake();
    } else if (!anim_.busy()) {
      playAnimation("purr");
    }
    sendEvent("touched");
  }
  wasTouched_ = touched;

  // Loud noise (clap) startles an idle robot.
  if (audio_.micRms() > kLoudRms && mode_ == Mode::kIdle && !speaker_.active() &&
      now - lastLoudMs_ > 4000) {
    lastLoudMs_ = now;
    noteInteraction(now);
    if (sleeping_) {
      wake();
    } else if (!anim_.busy() && !drive_.busy()) {
      playAnimation("surprised");
    }
    sendEvent("loud_sound");
  }

  // Server link status on the backpack.
  const bool linked = link_.connected();
  if (linked != wasLinked_) {
    wasLinked_ = linked;
    if (linked) {
      face_.showText("HELLO", link_.serverIp().toString().c_str(), 1200);
      lights_.setBase(behavior::BackpackLights::Pattern::kBreathe, 140, 0.5f);
      speaker_.tone(700, 1200, 120);
    } else {
      lights_.setBase(behavior::BackpackLights::Pattern::kBreathe, 220, 0.4f);
      if (!link_.wifiUp()) face_.showText("NO WIFI", COZMO_WIFI_SSID, 2000);
    }
  }

  // Battery.
  if (battery_.volts() > 2.5f && battery_.volts() < cfg::BATTERY_LOW_V && !lowBatteryWarned_) {
    lowBatteryWarned_ = true;
    face_.showText("BATTERY", "PLEASE CHARGE ME", 3000);
    lights_.setBase(behavior::BackpackLights::Pattern::kBreathe, 0, 0.5f);
    sendEvent("battery_low");
  } else if (battery_.volts() > cfg::BATTERY_LOW_V + 0.15f) {
    lowBatteryWarned_ = false;
  }
}

void Robot::updateIdle(uint32_t now) {
  // Temporary expressions set by the server fall back to the mode's default.
  if (expressionUntilMs_ != 0 && static_cast<int32_t>(now - expressionUntilMs_) >= 0 &&
      !anim_.busy()) {
    expressionUntilMs_ = 0;
    setMode(mode_);
  }

  if (mode_ != Mode::kIdle || anim_.busy() || drive_.busy() || pickedUp_ || sleeping_) return;

  const uint32_t idleFor = now - lastInteractionMs_;
  if (idleFor > kSleepAfterMs) {
    sleeping_ = true;
    playAnimation("sleep");
    face_.setSleeping(true);
    sendEvent("sleeping");
    return;
  }
  if (idleFor > kBoredAfterMs && !boredPlayed_) {
    boredPlayed_ = true;
    playAnimation("bored");
    return;
  }

  // Random glances keep the robot looking alive.
  if (now > nextGlanceMs_) {
    nextGlanceMs_ = now + 2500 + esp_random() % 6000;
    const float x = (static_cast<int>(esp_random() % 200) - 100) / 100.0f;
    const float y = (static_cast<int>(esp_random() % 100) - 50) / 100.0f;
    const bool center = esp_random() % 3 == 0;
    face_.setLook(center ? 0 : x, center ? 0 : y);
    if (esp_random() % 5 == 0) head_.moveTo(5 + esp_random() % 30, 500);
  }
}

void Robot::wake() {
  sleeping_ = false;
  boredPlayed_ = false;
  face_.setSleeping(false);
  playAnimation("wake");
  sendEvent("woke_up");
}

void Robot::setMode(Mode m) {
  mode_ = m;
  using P = behavior::BackpackLights::Pattern;
  switch (m) {
    case Mode::kIdle:
      face_.setExpression(face::Expression::kNeutral);
      face_.setLook(0, 0);
      lights_.setBase(P::kBreathe, 140, 0.5f);
      audio_.setMicEnabled(true);
      break;
    case Mode::kListening:
      face_.setExpression(face::Expression::kFocused);
      face_.setLook(0, -0.2f);
      head_.moveTo(30, 250);  // look up at the person talking
      lights_.setBase(P::kSolid, 185, 0.8f);
      break;
    case Mode::kThinking:
      face_.setExpression(face::Expression::kSquint);
      face_.setLook(0.6f, -0.6f);
      lights_.setBase(P::kChase, 280, 0.7f);
      break;
    case Mode::kSpeaking:
      face_.setExpression(face::Expression::kHappy);
      face_.setLook(0, 0);
      lights_.setBase(P::kBreathe, 50, 0.8f);
      break;
  }
}

void Robot::playAnimation(const char* name, int32_t id) {
  const behavior::Animation* a = behavior::findAnimation(name);
  if (a == nullptr) {
    log_w("Unknown animation %s", name);
    if (id >= 0) sendDone("anim", id);
    return;
  }
  if (animId_ >= 0) sendDone("anim", animId_);  // previous one was interrupted
  anim_.play(a);
  animId_ = id;
}

void Robot::executeStep(const behavior::Step& s) {
  Robot& r = instance();
  using behavior::Track;
  switch (s.track) {
    case Track::kFace:
      r.face_.setExpression(static_cast<face::Expression>(static_cast<int>(s.a)));
      break;
    case Track::kLook:
      r.face_.setLook(s.a, s.b);
      break;
    case Track::kBlink:
      r.face_.blink();
      break;
    case Track::kHead:
      r.head_.moveTo(s.a, s.durMs);
      break;
    case Track::kLift:
      r.lift_.moveTo(s.a, s.durMs);
      break;
    case Track::kDrive:
      if (!r.pickedUp_) r.drive_.drive(s.a, s.b, s.durMs);
      break;
    case Track::kTurn:
      if (!r.pickedUp_) r.drive_.turnInPlace(s.a, s.b);
      break;
    case Track::kMove:
      if (!r.pickedUp_) r.drive_.moveDistance(s.a, s.b);
      break;
    case Track::kTone:
      r.speaker_.tone(s.a, s.b, s.durMs, 0.45f);
      break;
    case Track::kLeds:
      if (s.a < 0) {
        r.lights_.setOverride(behavior::BackpackLights::Pattern::kOff, 0, 0, 1500);
      } else {
        r.lights_.setOverride(behavior::BackpackLights::Pattern::kSolid, s.a, s.b, 1500);
      }
      break;
  }
}

// --------------------------------------------------------------------------
//  Server protocol
// --------------------------------------------------------------------------
void Robot::onMessage(JsonDocument& msg) { instance().handleCommand(msg); }

void Robot::handleCommand(JsonDocument& msg) {
  const char* cmd = msg["cmd"] | "";
  const int32_t id = msg["id"] | -1;
  const uint32_t now = millis();

  if (strcmp(cmd, "ping") == 0) return;  // keep-alive only

  noteInteraction(now);
  if (sleeping_ && strcmp(cmd, "mode") != 0) wake();

  if (strcmp(cmd, "mode") == 0) {
    const char* v = msg["value"] | "idle";
    Mode m = Mode::kIdle;
    if (strcmp(v, "listening") == 0) m = Mode::kListening;
    else if (strcmp(v, "thinking") == 0) m = Mode::kThinking;
    else if (strcmp(v, "speaking") == 0) m = Mode::kSpeaking;
    if (m != Mode::kIdle && sleeping_) wake();
    if (!anim_.busy()) setMode(m); else mode_ = m;
    return;
  }

  if (strcmp(cmd, "drive") == 0) {
    if (pickedUp_) { if (id >= 0) sendDone("drive", id); return; }
    drive_.drive(msg["left"] | 0.0f, msg["right"] | 0.0f, msg["duration_ms"] | 0u);
    driveId_ = id;
  } else if (strcmp(cmd, "move") == 0) {
    if (pickedUp_) { if (id >= 0) sendDone("drive", id); return; }
    drive_.moveDistance(msg["distance_mm"] | 0.0f, msg["speed"] | 100.0f);
    driveId_ = id;
  } else if (strcmp(cmd, "turn") == 0) {
    if (pickedUp_) { if (id >= 0) sendDone("drive", id); return; }
    drive_.turnInPlace(msg["angle_deg"] | 0.0f, msg["speed_dps"] | 120.0f);
    driveId_ = id;
  } else if (strcmp(cmd, "stop") == 0) {
    drive_.emergencyStop();
    anim_.stop();
    if (driveId_ >= 0) { sendDone("drive", driveId_); driveId_ = -1; }
  } else if (strcmp(cmd, "head") == 0) {
    head_.moveTo(msg["angle_deg"] | 10.0f, msg["duration_ms"] | 300u);
    headId_ = id;
  } else if (strcmp(cmd, "lift") == 0) {
    lift_.moveTo(msg["height_mm"] | 0.0f, msg["duration_ms"] | 300u);
    liftId_ = id;
  } else if (strcmp(cmd, "face") == 0) {
    face::Expression e;
    if (face::expressionFromName(msg["expression"] | "neutral", &e)) {
      face_.setExpression(e);
      const uint32_t dur = msg["duration_ms"] | 3000u;
      expressionUntilMs_ = dur ? now + dur : 0;
    }
  } else if (strcmp(cmd, "look") == 0) {
    face_.setLook(msg["x"] | 0.0f, msg["y"] | 0.0f);
  } else if (strcmp(cmd, "anim") == 0) {
    playAnimation(msg["name"] | "", id);
  } else if (strcmp(cmd, "leds") == 0) {
    const char* p = msg["pattern"] | "solid";
    using P = behavior::BackpackLights::Pattern;
    P pattern = P::kSolid;
    if (strcmp(p, "off") == 0) pattern = P::kOff;
    else if (strcmp(p, "breathe") == 0) pattern = P::kBreathe;
    else if (strcmp(p, "chase") == 0) pattern = P::kChase;
    else if (strcmp(p, "blink") == 0) pattern = P::kBlink;
    lights_.setOverride(pattern, msg["hue"] | 0.0f, msg["brightness"] | 1.0f,
                        msg["duration_ms"] | 0u);
  } else if (strcmp(cmd, "tone") == 0) {
    speaker_.tone(msg["start_hz"] | 800.0f, msg["end_hz"] | 800.0f, msg["duration_ms"] | 150u,
                  msg["volume"] | 0.5f);
  } else if (strcmp(cmd, "volume") == 0) {
    speaker_.setVolume(msg["value"] | 0.7f);
  } else if (strcmp(cmd, "text") == 0) {
    face_.showText(msg["line1"] | "", msg["line2"] | "", msg["duration_ms"] | 2000u);
  } else if (strcmp(cmd, "audio_flush") == 0) {
    speaker_.flushQueue();
  } else if (strcmp(cmd, "sleep") == 0) {
    sleeping_ = true;
    playAnimation("sleep");
    face_.setSleeping(true);
  } else {
    log_w("Unknown command '%s'", cmd);
    JsonDocument err;
    err["type"] = "error";
    err["message"] = "unknown command";
    err["cmd"] = cmd;
    link_.send(err);
  }
}

void Robot::sendDone(const char* what, int32_t id) {
  JsonDocument doc;
  doc["type"] = "done";
  doc["what"] = what;
  doc["id"] = id;
  link_.send(doc);
}

void Robot::sendEvent(const char* name) {
  JsonDocument doc;
  doc["type"] = "event";
  doc["name"] = name;
  link_.send(doc);
}

void Robot::sendTelemetry(uint32_t now) {
  if (!link_.connected()) return;
  JsonDocument doc;
  doc["type"] = "status";
  doc["t"] = now;
  doc["battery_v"] = serialized(String(battery_.volts(), 2));
  doc["battery_pct"] = battery_.percent();
  doc["picked_up"] = static_cast<bool>(pickedUp_);
  doc["touched"] = touch_.touched();
  doc["sleeping"] = sleeping_;
  doc["head_deg"] = serialized(String(head_.value(), 1));
  doc["lift_mm"] = serialized(String(lift_.value(), 1));
  doc["yaw_deg"] = serialized(String(static_cast<float>(yawDeg_), 1));
  doc["speed_l"] = serialized(String(drive_.speedLeft(), 0));
  doc["speed_r"] = serialized(String(drive_.speedRight(), 0));
  doc["encoders_ok"] = drive_.encodersOk();
  doc["imu_ok"] = static_cast<bool>(imuOk_);
  doc["busy"] = drive_.busy() || anim_.busy() || head_.busy() || lift_.busy();
  doc["anim"] = anim_.current();
  doc["expression"] = face::expressionName(face_.expression());
  doc["mic_rms"] = audio_.micRms();
  doc["rssi"] = WiFi.RSSI();
  doc["audio_tx"] = audio_.packetsSent();
  doc["audio_rx"] = audio_.packetsReceived();
  link_.send(doc);
}
