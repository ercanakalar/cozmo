#pragma once
// ---------------------------------------------------------------------------
//  Cozmo replica - hardware configuration and pin map (ESP32 DevKit 38-pin)
//  Keep this file in sync with docs/WIRING.md.
// ---------------------------------------------------------------------------
#include <cstdint>

namespace cfg {

// ---- I2C bus: OLED face (SSD1306) + IMU (MPU6050) -------------------------
constexpr int PIN_I2C_SDA = 21;
constexpr int PIN_I2C_SCL = 22;
constexpr uint32_t I2C_FREQ_HZ = 400000;
constexpr uint8_t OLED_ADDR = 0x3C;
constexpr uint8_t IMU_ADDR = 0x68;
constexpr bool OLED_IS_SH1106 = false;  // true for most 1.3" modules

// ---- Microphone: INMP441 on I2S port 0 ------------------------------------
constexpr int PIN_MIC_BCLK = 26;  // INMP441 SCK
constexpr int PIN_MIC_WS = 25;    // INMP441 WS
constexpr int PIN_MIC_SD = 34;    // INMP441 SD (input only pin is fine)
constexpr uint32_t MIC_SAMPLE_RATE = 16000;
constexpr int MIC_GAIN_SHIFT = 11;  // 32-bit I2S word -> int16 (bigger = quieter)

// ---- Speaker: MAX98357A on I2S port 1 -------------------------------------
constexpr int PIN_SPK_BCLK = 27;
constexpr int PIN_SPK_LRC = 14;
constexpr int PIN_SPK_DIN = 13;
constexpr uint32_t SPK_SAMPLE_RATE = 16000;

// ---- Tread motors: DRV8833 dual H-bridge ----------------------------------
constexpr int PIN_MOT_L_IN1 = 16;
constexpr int PIN_MOT_L_IN2 = 17;
constexpr int PIN_MOT_R_IN1 = 18;
constexpr int PIN_MOT_R_IN2 = 19;
constexpr uint32_t MOTOR_PWM_HZ = 20000;  // inaudible
constexpr bool MOTOR_L_INVERT = false;
constexpr bool MOTOR_R_INVERT = true;  // motors are mirrored in the chassis

// ---- Wheel encoders (N20 hall encoder, channel A only) --------------------
constexpr int PIN_ENC_L = 35;  // needs external 10k pull-up to 3V3
constexpr int PIN_ENC_R = 39;  // needs external 10k pull-up to 3V3
constexpr float ENC_TICKS_PER_WHEEL_REV = 7.0f * 150.0f;  // 7 PPR * 150:1 gearbox
constexpr float TREAD_SPROCKET_DIAMETER_MM = 30.0f;        // see hardware/3d
constexpr float TRACK_WIDTH_MM = 62.0f;                    // tread centre distance

// ---- Servos: head tilt + lift ---------------------------------------------
constexpr int PIN_SERVO_HEAD = 4;
constexpr int PIN_SERVO_LIFT = 23;
// Calibrate these per robot (see docs/ASSEMBLY.md -> "Servo calibration").
constexpr float HEAD_MIN_DEG = -25.0f;  // looking down
constexpr float HEAD_MAX_DEG = 44.5f;   // looking up
constexpr int HEAD_SERVO_US_AT_MIN = 1000;
constexpr int HEAD_SERVO_US_AT_MAX = 2000;
constexpr float LIFT_MIN_MM = 0.0f;   // forks on the floor
constexpr float LIFT_MAX_MM = 60.0f;  // fully raised
constexpr int LIFT_SERVO_US_AT_MIN = 2100;
constexpr int LIFT_SERVO_US_AT_MAX = 1000;

// ---- Backpack lights: 4x WS2812B ------------------------------------------
constexpr int PIN_LEDS = 32;
constexpr int NUM_LEDS = 4;

// ---- Capacitive touch pad on the head (copper tape) -----------------------
constexpr int PIN_TOUCH = 33;  // ESP32 touch channel T8

// ---- Battery sense: 100k/100k divider from the LiPo + --------------------
constexpr int PIN_BATTERY = 36;  // ADC1_CH0 (works while WiFi is on)
constexpr float BATTERY_DIVIDER = 2.0f;
constexpr float BATTERY_LOW_V = 3.45f;
constexpr float BATTERY_FULL_V = 4.15f;

// ---- Network ---------------------------------------------------------------
constexpr uint16_t PORT_DISCOVERY = 5006;    // UDP: server beacon (broadcast)
constexpr uint16_t PORT_AUDIO_UP = 5005;     // UDP: robot mic -> server
constexpr uint16_t PORT_AUDIO_DOWN = 5007;   // UDP: server TTS -> robot speaker
constexpr uint16_t PORT_CONTROL = 5008;      // TCP: JSON lines, both directions
constexpr uint32_t LINK_TIMEOUT_MS = 3000;   // stop motors when server is silent
constexpr uint32_t TELEMETRY_PERIOD_MS = 200;

// ---- Audio streaming -------------------------------------------------------
constexpr int AUDIO_FRAME_SAMPLES = 320;  // 20 ms at 16 kHz
// Simple on-robot noise gate to save bandwidth. The server runs the real VAD.
constexpr bool AUDIO_GATE_ENABLED = false;
constexpr int AUDIO_GATE_RMS = 300;
constexpr uint32_t AUDIO_GATE_HANG_MS = 800;

}  // namespace cfg
