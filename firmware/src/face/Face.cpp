#include "Face.h"

#include <cmath>

namespace face {

namespace {

constexpr float kEyeCenterLeft = 36.0f;
constexpr float kEyeCenterRight = 92.0f;
constexpr float kEyeCenterY = 32.0f;
constexpr uint32_t kBlinkMs = 160;

struct Preset {
  const char* name;
  FaceShape shape;
};

//                       dx   dy    w     h     r   lidTop angle lidBottom
constexpr EyeShape kNeutralEye = {0, 0, 36, 40, 9, 0.00f, 0, 0.00f};

const Preset kPresets[] = {
    {"neutral", {kNeutralEye, kNeutralEye}},
    {"happy", {{0, -2, 38, 40, 12, 0.00f, 0, 0.55f}, {0, -2, 38, 40, 12, 0.00f, 0, 0.55f}}},
    {"sad", {{2, 5, 34, 34, 8, 0.38f, -22, 0.00f}, {-2, 5, 34, 34, 8, 0.38f, -22, 0.00f}}},
    {"angry", {{3, 2, 36, 34, 6, 0.40f, 26, 0.00f}, {-3, 2, 36, 34, 6, 0.40f, 26, 0.00f}}},
    {"surprised", {{-2, -2, 40, 50, 14, 0.00f, 0, 0.00f}, {2, -2, 40, 50, 14, 0.00f, 0, 0.00f}}},
    {"sleepy", {{0, 6, 36, 30, 8, 0.62f, -6, 0.00f}, {0, 6, 36, 30, 8, 0.62f, -6, 0.00f}}},
    {"suspicious", {{0, 2, 36, 38, 8, 0.45f, 8, 0.10f}, {0, 0, 36, 40, 8, 0.15f, -4, 0.00f}}},
    {"scared", {{4, 2, 30, 44, 10, 0.12f, -18, 0.00f}, {-4, 2, 30, 44, 10, 0.12f, -18, 0.00f}}},
    {"focused", {{0, -1, 38, 44, 10, 0.05f, 0, 0.00f}, {0, -1, 38, 44, 10, 0.05f, 0, 0.00f}}},
    {"bored", {{0, 4, 36, 34, 8, 0.50f, 0, 0.12f}, {0, 4, 36, 34, 8, 0.50f, 0, 0.12f}}},
    {"excited", {{-2, -3, 42, 46, 14, 0.00f, 0, 0.45f}, {2, -3, 42, 46, 14, 0.00f, 0, 0.45f}}},
    {"squint", {{0, 0, 38, 40, 6, 0.40f, 0, 0.35f}, {0, 0, 38, 40, 6, 0.40f, 0, 0.35f}}},
};
static_assert(sizeof(kPresets) / sizeof(kPresets[0]) == static_cast<size_t>(Expression::kCount),
              "every expression needs a preset");

float lerp(float a, float b, float t) { return a + (b - a) * t; }

class Lock {
 public:
  explicit Lock(SemaphoreHandle_t m) : m_(m) { xSemaphoreTake(m_, portMAX_DELAY); }
  ~Lock() { xSemaphoreGive(m_); }

 private:
  SemaphoreHandle_t m_;
};

}  // namespace

const char* expressionName(Expression e) {
  const auto i = static_cast<size_t>(e);
  return i < static_cast<size_t>(Expression::kCount) ? kPresets[i].name : "neutral";
}

bool expressionFromName(const char* name, Expression* out) {
  if (name == nullptr) return false;
  for (size_t i = 0; i < static_cast<size_t>(Expression::kCount); ++i) {
    if (strcasecmp(name, kPresets[i].name) == 0) {
      *out = static_cast<Expression>(i);
      return true;
    }
  }
  return false;
}

void Face::begin(drivers::Ssd1306* oled) {
  oled_ = oled;
  mutex_ = xSemaphoreCreateMutex();
  target_ = kPresets[0].shape;
  current_ = target_;
  current_.left.h = current_.right.h = 2;  // "open the eyes" on boot
}

void Face::setExpression(Expression e) {
  Lock lock(mutex_);
  expression_ = e;
  target_ = kPresets[static_cast<size_t>(e)].shape;
}

void Face::setLook(float x, float y) {
  Lock lock(mutex_);
  lookX_ = constrain(x, -1.0f, 1.0f);
  lookY_ = constrain(y, -1.0f, 1.0f);
}

void Face::blink() { blinkRequested_ = true; }

void Face::setSleeping(bool sleeping) {
  sleeping_ = sleeping;
  setExpression(sleeping ? Expression::kSleepy : Expression::kNeutral);
}

void Face::showText(const char* line1, const char* line2, uint32_t durationMs) {
  Lock lock(mutex_);
  strlcpy(text1_, line1 ? line1 : "", sizeof(text1_));
  strlcpy(text2_, line2 ? line2 : "", sizeof(text2_));
  textUntilMs_ = millis() + durationMs;
}

void Face::lerpShape(EyeShape& c, const EyeShape& t, float k) {
  c.dx = lerp(c.dx, t.dx, k);
  c.dy = lerp(c.dy, t.dy, k);
  c.w = lerp(c.w, t.w, k);
  c.h = lerp(c.h, t.h, k);
  c.r = lerp(c.r, t.r, k);
  c.lidTop = lerp(c.lidTop, t.lidTop, k);
  c.lidAngle = lerp(c.lidAngle, t.lidAngle, k);
  c.lidBottom = lerp(c.lidBottom, t.lidBottom, k);
}

void Face::drawEye(const EyeShape& e, float baseX, int side, float blink) {
  // side: -1 = left eye on screen, +1 = right eye.
  const float perspective = 1.0f + 0.12f * curLookX_ * side;
  const float w = e.w * perspective;
  float h = e.h * perspective * blink * (1.0f - 0.18f * talk_);
  if (sleeping_) h = std::min(h, 6.0f);
  if (h < 2.0f) h = 2.0f;

  const float cx = baseX + e.dx + curLookX_ * 16.0f;
  const float cy = kEyeCenterY + e.dy + curLookY_ * 10.0f + (e.h * perspective - h) * 0.25f;
  const int x0 = static_cast<int>(lroundf(cx - w / 2));
  const int y0 = static_cast<int>(lroundf(cy - h / 2));
  const int wi = static_cast<int>(lroundf(w));
  const int hi = static_cast<int>(lroundf(h));
  const int r = static_cast<int>(lroundf(std::min(e.r, h / 2)));

  oled_->fillRoundRect(x0, y0, wi, hi, r, true);

  if (e.lidTop > 0.01f && !sleeping_) {
    // Angled upper lid: erase everything above a line through the eye.
    const float inner = (side < 0) ? 1.0f : -1.0f;  // direction to the face centre
    const float slope = tanf(e.lidAngle * PI / 180.0f) * inner;
    const float yLine = y0 + e.lidTop * h;
    const int xa = x0 - 2;
    const int xb = x0 + wi + 2;
    const int ya = static_cast<int>(lroundf(yLine + slope * (xa - cx)));
    const int yb = static_cast<int>(lroundf(yLine + slope * (xb - cx)));
    const int top = y0 - 30;
    oled_->fillTriangle(xa, top, xb, top, xb, yb, false);
    oled_->fillTriangle(xa, top, xb, yb, xa, ya, false);
  }
  if (e.lidBottom > 0.01f && !sleeping_) {
    // Curved lower lid: a black ellipse pushed up from below ("^ ^").
    const int rx = static_cast<int>(w * 0.58f);
    const int ry = static_cast<int>(h * 0.62f);
    const int ecy = static_cast<int>(y0 + h + ry - e.lidBottom * h);
    oled_->fillEllipse(static_cast<int>(cx), ecy, rx, ry, false);
  }
}

void Face::render(uint32_t now) {
  if (oled_ == nullptr || !oled_->present()) return;

  FaceShape target;
  float lookX, lookY;
  bool showText;
  char t1[22], t2[22];
  {
    Lock lock(mutex_);
    target = target_;
    lookX = lookX_;
    lookY = lookY_;
    showText = now < textUntilMs_;
    if (showText) {
      memcpy(t1, text1_, sizeof(t1));
      memcpy(t2, text2_, sizeof(t2));
    }
  }

  oled_->clear();

  if (showText) {
    oled_->drawText((drivers::Ssd1306::kWidth - drivers::Ssd1306::textWidth(t1, 2)) / 2, 12, t1, 2);
    oled_->drawText((drivers::Ssd1306::kWidth - drivers::Ssd1306::textWidth(t2, 1)) / 2, 40, t2, 1);
    oled_->flush();
    return;
  }

  // Ease toward the target shape and gaze.
  lerpShape(current_.left, target.left, 0.28f);
  lerpShape(current_.right, target.right, 0.28f);
  curLookX_ = lerp(curLookX_, lookX, 0.35f);
  curLookY_ = lerp(curLookY_, lookY, 0.35f);

  // Blink handling.
  if (autoBlink_ && !sleeping_ && now > nextBlinkMs_) {
    blinkRequested_ = true;
    nextBlinkMs_ = now + 1800 + esp_random() % 4500;
  }
  if (blinkRequested_) {
    blinkRequested_ = false;
    blinkStartMs_ = now;
  }
  float blink = 1.0f;
  if (now - blinkStartMs_ < kBlinkMs) {
    const float t = static_cast<float>(now - blinkStartMs_) / kBlinkMs;
    blink = 1.0f - 0.92f * sinf(t * PI);
  }

  drawEye(current_.left, kEyeCenterLeft, -1, blink);
  drawEye(current_.right, kEyeCenterRight, +1, blink);

  if (sleeping_) {
    // Floating "z z Z" in the corner.
    const int phase = (now / 600) % 3;
    oled_->drawText(100, 14 - phase * 4, "Z", 1);
    if (phase > 0) oled_->drawText(108, 8 - phase * 2, "Z", 1);
    if (phase > 1) oled_->drawText(116, 2, "Z", 2);
  }
  oled_->flush();
}

}  // namespace face
