#pragma once
// Keyframe animations that combine face, head, lift, treads, sound and lights.
// All choreography here is original to this project.
#include <Arduino.h>

namespace behavior {

enum class Track : uint8_t {
  kFace,   // a = expression index
  kLook,   // a = x, b = y (-1..1)
  kBlink,  //
  kHead,   // a = degrees, dur
  kLift,   // a = mm, dur
  kDrive,  // a = left mm/s, b = right mm/s, dur
  kTurn,   // a = degrees (CCW +), b = deg/s
  kMove,   // a = mm, b = mm/s
  kTone,   // a = start Hz, b = end Hz, dur
  kLeds,   // a = hue (0..360, <0 = off), b = brightness (0..1)
};

struct Step {
  uint16_t atMs;
  Track track;
  float a;
  float b;
  uint16_t durMs;
};

struct Animation {
  const char* name;
  const Step* steps;
  uint8_t count;
};

const Animation* findAnimation(const char* name);
const Animation* animationAt(size_t index);
size_t animationCount();

// Plays one animation at a time; each step is handed to an executor callback.
class AnimationPlayer {
 public:
  using Executor = void (*)(const Step& step);
  explicit AnimationPlayer(Executor exec) : exec_(exec) {}

  void play(const Animation* anim);
  void stop() { anim_ = nullptr; }
  void update(uint32_t nowMs);
  bool busy() const { return anim_ != nullptr; }
  const char* current() const { return anim_ ? anim_->name : ""; }

 private:
  Executor exec_;
  const Animation* anim_ = nullptr;
  uint32_t startMs_ = 0;
  uint32_t endMs_ = 0;
  uint8_t next_ = 0;
};

}  // namespace behavior
