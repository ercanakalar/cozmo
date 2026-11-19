#pragma once
// Procedural animated eyes for the 128x64 OLED.
//
// Each eye is a rounded rectangle that can be "cut" by an angled upper lid
// (sad / angry / sleepy) and a curved lower lid (happy). Expressions are just
// target shapes; the renderer eases the current shape toward the target every
// frame, so every change is smoothly animated. Blinks, gaze and a "talking"
// bounce are layered on top.
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "drivers/Ssd1306.h"

namespace face {

enum class Expression : uint8_t {
  kNeutral,
  kHappy,
  kSad,
  kAngry,
  kSurprised,
  kSleepy,
  kSuspicious,
  kScared,
  kFocused,
  kBored,
  kExcited,
  kSquint,
  kCount,
};

const char* expressionName(Expression e);
bool expressionFromName(const char* name, Expression* out);

struct EyeShape {
  float dx, dy;      // offset from the default eye centre (px)
  float w, h, r;     // size and corner radius (px)
  float lidTop;      // 0..1 of the eye height covered by the upper lid
  float lidAngle;    // degrees, + = inner corner lower (angry), - = outer lower (sad)
  float lidBottom;   // 0..1 curved lower lid (happy "^ ^" eyes)
};

struct FaceShape {
  EyeShape left;
  EyeShape right;
};

class Face {
 public:
  void begin(drivers::Ssd1306* oled);

  // --- Thread-safe control API (call from any task) ---
  void setExpression(Expression e);
  Expression expression() const { return expression_; }
  void setLook(float x, float y);  // -1..1, + = right / down
  void blink();
  void setAutoBlink(bool enabled) { autoBlink_ = enabled; }
  void setTalkLevel(float level) { talk_ = level; }
  void setSleeping(bool sleeping);
  bool sleeping() const { return sleeping_; }
  void showText(const char* line1, const char* line2, uint32_t durationMs);

  // --- Render one frame (call from the display task, ~25 fps) ---
  void render(uint32_t nowMs);

 private:
  void drawEye(const EyeShape& eye, float baseX, int side, float blink);
  static void lerpShape(EyeShape& cur, const EyeShape& target, float k);

  drivers::Ssd1306* oled_ = nullptr;
  SemaphoreHandle_t mutex_ = nullptr;

  Expression expression_ = Expression::kNeutral;
  FaceShape target_ = {};
  FaceShape current_ = {};
  float lookX_ = 0, lookY_ = 0;
  float curLookX_ = 0, curLookY_ = 0;
  volatile float talk_ = 0;
  bool autoBlink_ = true;
  bool sleeping_ = false;
  uint32_t blinkStartMs_ = 0;
  uint32_t nextBlinkMs_ = 2000;
  bool blinkRequested_ = false;

  char text1_[22] = {};
  char text2_[22] = {};
  uint32_t textUntilMs_ = 0;
};

}  // namespace face
