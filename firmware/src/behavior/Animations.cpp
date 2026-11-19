#include "Animations.h"

#include "face/Face.h"

namespace behavior {

namespace {

using face::Expression;
constexpr float E(Expression e) { return static_cast<float>(e); }

// Steps MUST be sorted by atMs.
const Step kHappy[] = {
    {0, Track::kFace, E(Expression::kHappy), 0, 0},
    {0, Track::kHead, 32, 0, 250},
    {0, Track::kTone, 600, 1200, 110},
    {0, Track::kLeds, 120, 1.0f, 0},
    {100, Track::kLift, 25, 0, 200},
    {160, Track::kTone, 900, 1500, 110},
    {420, Track::kLift, 0, 0, 250},
    {520, Track::kHead, 12, 0, 300},
};

const Step kSad[] = {
    {0, Track::kFace, E(Expression::kSad), 0, 0},
    {0, Track::kLeds, 230, 0.3f, 0},
    {0, Track::kLift, 0, 0, 600},
    {0, Track::kTone, 520, 300, 450},
    {100, Track::kHead, -22, 0, 900},
    {300, Track::kLook, 0.2f, 0.6f, 0},
    {600, Track::kTone, 350, 240, 500},
};

const Step kAngry[] = {
    {0, Track::kFace, E(Expression::kAngry), 0, 0},
    {0, Track::kLeds, 0, 1.0f, 0},
    {0, Track::kHead, 8, 0, 150},
    {0, Track::kLift, 40, 0, 120},
    {0, Track::kTone, 220, 150, 300},
    {120, Track::kDrive, 140, 140, 150},
    {320, Track::kDrive, -140, -140, 150},
    {420, Track::kLift, 0, 0, 150},
    {520, Track::kTone, 200, 140, 250},
};

const Step kSurprised[] = {
    {0, Track::kFace, E(Expression::kSurprised), 0, 0},
    {0, Track::kHead, 42, 0, 120},
    {0, Track::kLift, 30, 0, 150},
    {0, Track::kTone, 400, 1700, 150},
    {50, Track::kDrive, -90, -90, 220},
    {600, Track::kLift, 0, 0, 300},
    {900, Track::kFace, E(Expression::kNeutral), 0, 0},
};

const Step kCelebrate[] = {
    {0, Track::kFace, E(Expression::kExcited), 0, 0},
    {0, Track::kLeds, 60, 1.0f, 0},
    {0, Track::kHead, 40, 0, 200},
    {0, Track::kTone, 523, 523, 120},
    {0, Track::kLift, 60, 0, 200},
    {140, Track::kTone, 659, 659, 120},
    {250, Track::kTurn, 360, 400, 0},
    {280, Track::kTone, 784, 784, 120},
    {300, Track::kLift, 0, 0, 200},
    {420, Track::kTone, 1046, 1300, 260},
    {550, Track::kLift, 60, 0, 200},
    {800, Track::kLift, 0, 0, 200},
    {1050, Track::kLift, 60, 0, 200},
    {1300, Track::kLift, 0, 0, 250},
    {1400, Track::kTone, 784, 1046, 200},
    {1700, Track::kFace, E(Expression::kHappy), 0, 0},
};

const Step kCurious[] = {
    {0, Track::kFace, E(Expression::kFocused), 0, 0},
    {0, Track::kHead, 22, 0, 300},
    {0, Track::kLook, -0.8f, 0, 0},
    {0, Track::kTone, 700, 1050, 120},
    {200, Track::kTurn, 20, 80, 0},
    {700, Track::kLook, 0.8f, 0, 0},
    {800, Track::kTurn, -40, 80, 0},
    {1400, Track::kLook, 0, 0, 0},
    {1450, Track::kTurn, 20, 80, 0},
    {1500, Track::kTone, 900, 1300, 120},
};

const Step kSleep[] = {
    {0, Track::kFace, E(Expression::kSleepy), 0, 0},
    {0, Track::kLift, 0, 0, 800},
    {0, Track::kTone, 420, 200, 700},
    {200, Track::kHead, -25, 0, 1600},
    {300, Track::kLeds, 240, 0.08f, 0},
};

const Step kWake[] = {
    {0, Track::kBlink, 0, 0, 0},
    {0, Track::kFace, E(Expression::kSurprised), 0, 0},
    {0, Track::kHead, 30, 0, 350},
    {0, Track::kLeds, 180, 0.8f, 0},
    {100, Track::kTone, 300, 900, 200},
    {450, Track::kBlink, 0, 0, 0},
    {600, Track::kFace, E(Expression::kNeutral), 0, 0},
    {650, Track::kTone, 800, 1100, 100},
};

const Step kNodYes[] = {
    {0, Track::kFace, E(Expression::kHappy), 0, 0},
    {0, Track::kHead, 30, 0, 170},
    {200, Track::kHead, 0, 0, 170},
    {400, Track::kHead, 30, 0, 170},
    {600, Track::kHead, 5, 0, 170},
    {650, Track::kTone, 800, 1100, 120},
    {800, Track::kHead, 15, 0, 200},
};

const Step kShakeNo[] = {
    {0, Track::kFace, E(Expression::kSuspicious), 0, 0},
    {0, Track::kLook, -1, 0, 0},
    {0, Track::kTurn, 15, 220, 0},
    {0, Track::kTone, 500, 380, 200},
    {220, Track::kLook, 1, 0, 0},
    {250, Track::kTurn, -30, 220, 0},
    {500, Track::kLook, -1, 0, 0},
    {520, Track::kTurn, 15, 220, 0},
    {750, Track::kLook, 0, 0, 0},
    {800, Track::kFace, E(Expression::kNeutral), 0, 0},
};

const Step kDance[] = {
    {0, Track::kFace, E(Expression::kExcited), 0, 0},
    {0, Track::kLeds, 300, 1.0f, 0},
    {0, Track::kTone, 660, 660, 100},
    {0, Track::kLift, 50, 0, 150},
    {0, Track::kDrive, -110, 110, 250},
    {250, Track::kLift, 0, 0, 150},
    {250, Track::kTone, 880, 880, 100},
    {260, Track::kDrive, 110, -110, 250},
    {500, Track::kLift, 50, 0, 150},
    {500, Track::kHead, 40, 0, 150},
    {500, Track::kTone, 990, 990, 100},
    {510, Track::kDrive, -110, 110, 250},
    {750, Track::kLift, 0, 0, 150},
    {750, Track::kHead, 0, 0, 150},
    {750, Track::kTone, 880, 880, 100},
    {760, Track::kDrive, 110, -110, 250},
    {1000, Track::kLeds, 60, 1.0f, 0},
    {1000, Track::kLift, 60, 0, 200},
    {1000, Track::kHead, 40, 0, 200},
    {1000, Track::kTurn, 360, 450, 0},
    {1000, Track::kTone, 660, 1320, 300},
    {1900, Track::kLift, 0, 0, 300},
    {1900, Track::kFace, E(Expression::kHappy), 0, 0},
};

const Step kGreet[] = {
    {0, Track::kFace, E(Expression::kHappy), 0, 0},
    {0, Track::kHead, 35, 0, 250},
    {0, Track::kLeds, 160, 1.0f, 0},
    {100, Track::kTone, 800, 1150, 140},
    {150, Track::kLift, 35, 0, 180},
    {280, Track::kTone, 1100, 750, 160},
    {350, Track::kLift, 10, 0, 180},
    {550, Track::kLift, 35, 0, 180},
    {750, Track::kLift, 0, 0, 250},
};

const Step kScared[] = {
    {0, Track::kFace, E(Expression::kScared), 0, 0},
    {0, Track::kLeds, 30, 1.0f, 0},
    {0, Track::kHead, -10, 0, 150},
    {0, Track::kDrive, -160, -160, 300},
    {0, Track::kTone, 1500, 2000, 80},
    {100, Track::kTone, 1500, 2000, 80},
    {200, Track::kTone, 1500, 2100, 80},
    {300, Track::kLook, -0.6f, 0.2f, 0},
    {700, Track::kLook, 0.6f, 0.2f, 0},
    {1100, Track::kLook, 0, 0, 0},
};

const Step kBored[] = {
    {0, Track::kFace, E(Expression::kBored), 0, 0},
    {0, Track::kHead, -10, 0, 800},
    {200, Track::kLook, -0.5f, 0.3f, 0},
    {300, Track::kTone, 330, 240, 700},
    {1200, Track::kLook, 0.5f, 0.3f, 0},
    {2000, Track::kLook, 0, 0, 0},
};

const Step kThinking[] = {
    {0, Track::kFace, E(Expression::kSquint), 0, 0},
    {0, Track::kLook, 0.6f, -0.6f, 0},
    {0, Track::kHead, 25, 0, 400},
    {500, Track::kTone, 600, 650, 60},
    {650, Track::kTone, 700, 750, 60},
    {1200, Track::kFace, E(Expression::kNeutral), 0, 0},
    {1200, Track::kLook, 0, 0, 0},
};

const Step kPurr[] = {
    {0, Track::kFace, E(Expression::kHappy), 0, 0},
    {0, Track::kLeds, 320, 1.0f, 0},
    {0, Track::kHead, 38, 0, 400},
    {0, Track::kTone, 110, 130, 250},
    {260, Track::kTone, 130, 110, 250},
    {520, Track::kTone, 110, 130, 250},
    {780, Track::kTone, 130, 100, 300},
};

const Step kLiftUp[] = {
    {0, Track::kFace, E(Expression::kFocused), 0, 0},
    {0, Track::kLift, 60, 0, 500},
    {200, Track::kTone, 500, 800, 150},
};

const Step kLiftDown[] = {
    {0, Track::kLift, 0, 0, 500},
    {200, Track::kTone, 800, 500, 150},
};

#define ANIM(name, arr) {name, arr, static_cast<uint8_t>(sizeof(arr) / sizeof(arr[0]))}
const Animation kAnimations[] = {
    ANIM("happy", kHappy),       ANIM("sad", kSad),           ANIM("angry", kAngry),
    ANIM("surprised", kSurprised), ANIM("celebrate", kCelebrate), ANIM("curious", kCurious),
    ANIM("sleep", kSleep),       ANIM("wake", kWake),         ANIM("nod_yes", kNodYes),
    ANIM("shake_no", kShakeNo),  ANIM("dance", kDance),       ANIM("greet", kGreet),
    ANIM("scared", kScared),     ANIM("bored", kBored),       ANIM("thinking", kThinking),
    ANIM("purr", kPurr),         ANIM("lift_up", kLiftUp),    ANIM("lift_down", kLiftDown),
};
#undef ANIM

}  // namespace

const Animation* findAnimation(const char* name) {
  if (name == nullptr) return nullptr;
  for (const auto& a : kAnimations) {
    if (strcasecmp(a.name, name) == 0) return &a;
  }
  return nullptr;
}

const Animation* animationAt(size_t index) {
  return index < animationCount() ? &kAnimations[index] : nullptr;
}

size_t animationCount() { return sizeof(kAnimations) / sizeof(kAnimations[0]); }

void AnimationPlayer::play(const Animation* anim) {
  anim_ = anim;
  next_ = 0;
  startMs_ = millis();
  uint32_t length = 0;
  for (uint8_t i = 0; anim && i < anim->count; ++i) {
    uint32_t extra = anim->steps[i].durMs;
    if (anim->steps[i].track == Track::kTurn) {
      extra = static_cast<uint32_t>(fabsf(anim->steps[i].a) / std::max(1.0f, anim->steps[i].b) * 1000.0f);
    }
    length = std::max(length, static_cast<uint32_t>(anim->steps[i].atMs) + extra);
  }
  endMs_ = startMs_ + length + 150;
}

void AnimationPlayer::update(uint32_t now) {
  if (anim_ == nullptr) return;
  const uint32_t t = now - startMs_;
  while (next_ < anim_->count && anim_->steps[next_].atMs <= t) {
    exec_(anim_->steps[next_]);
    ++next_;
  }
  if (next_ >= anim_->count && static_cast<int32_t>(now - endMs_) >= 0) anim_ = nullptr;
}

}  // namespace behavior
