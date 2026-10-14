#pragma once
#include <cmath>

#include "Math.h"

#include "Oscillator.h"  // Rng

namespace plasma {

class Adsr {
 public:
  enum class Stage { Idle, Attack, Decay, Sustain, Release };

  void noteOn() { stage_ = Stage::Attack; atkP_ = 0.f; atkFrom_ = level_; }
  void noteOff() { if (stage_ != Stage::Idle) stage_ = Stage::Release; }
  void kill() { stage_ = Stage::Idle; level_ = 0.f; }
  bool active() const { return stage_ != Stage::Idle; }
  Stage stage() const { return stage_; }
  float level() const { return level_; }

  // call once per control step with current times (seconds)
  void setParams(float a, float d, float s, float r, float sampleRate) {
    sus_ = s;
    sr_ = sampleRate;
    // Minimum times keep very short settings from clicking: a 3 ms attack and 8 ms release
    // is already shorter than most notes' first cycle.
    atkInc_ = 1.f / (std::max(a, kMinAttack) * sampleRate);
    decCoef_ = coef(d / 4.6f);
    relCoef_ = coef(std::max(r, kMinRelease) / 4.6f);
  }

  inline float tick() {
    switch (stage_) {
      case Stage::Idle: break;
      case Stage::Attack: {
        // Smoothstep rise: starts with zero slope, so there is no hard corner at note-on
        atkP_ += atkInc_;
        if (atkP_ >= 1.f) { level_ = 1.f; stage_ = Stage::Decay; break; }
        const float sm = atkP_ * atkP_ * (3.f - 2.f * atkP_);
        level_ = atkFrom_ + (1.f - atkFrom_) * sm;
        break;
      }
      case Stage::Decay:
        level_ = sus_ + (level_ - sus_) * decCoef_;
        if (std::fabs(level_ - sus_) < 1e-4f) { level_ = sus_; stage_ = Stage::Sustain; }
        break;
      case Stage::Sustain: level_ = sus_; break;
      case Stage::Release:
        level_ *= relCoef_;
        if (level_ < 1e-4f) { level_ = 0.f; stage_ = Stage::Idle; }
        break;
    }
    return level_;
  }

 private:
  float coef(float tau) const { return std::exp(-1.f / (std::max(tau, 1e-4f) * sr_)); }
  Stage stage_ = Stage::Idle;
  static constexpr float kMinAttack = 0.003f, kMinRelease = 0.008f;
  float level_ = 0.f, sus_ = 0.8f, sr_ = 48000.f, atkInc_ = 0.f, atkP_ = 0.f, atkFrom_ = 0.f, decCoef_ = 0.f, relCoef_ = 0.f;
};

enum class LfoShape { Sine, Tri, SawUp, SawDown, Square, SampleHold, Smooth };

class Lfo {
 public:
  void reset(float startPhase, Rng& rng) {
    phase_ = startPhase;
    cur_ = rng.next();
    next_ = rng.next();
    fade_ = 0.f;
  }
  // advance by dt seconds, rate in Hz, returns bipolar value
  float tick(LfoShape shape, float rate, float dt, float fadeTime, Rng& rng) {
    phase_ += rate * dt;
    while (phase_ >= 1.f) {
      phase_ -= 1.f;
      cur_ = next_;
      next_ = rng.next();
    }
    if (fadeTime > 0.001f) fade_ = std::min(1.f, fade_ + dt / fadeTime); else fade_ = 1.f;
    float y = 0.f;
    switch (shape) {
      case LfoShape::Sine: y = std::sin(6.2831853f * phase_); break;
      case LfoShape::Tri: y = 4.f * std::fabs(phase_ - 0.5f) - 1.f; break;
      case LfoShape::SawUp: y = 2.f * phase_ - 1.f; break;
      case LfoShape::SawDown: y = 1.f - 2.f * phase_; break;
      case LfoShape::Square: y = phase_ < 0.5f ? 1.f : -1.f; break;
      case LfoShape::SampleHold: y = cur_; break;
      case LfoShape::Smooth: { const float t = phase_ * phase_ * (3.f - 2.f * phase_); y = cur_ + (next_ - cur_) * t; } break;
    }
    return y * fade_;
  }

 private:
  float phase_ = 0.f, cur_ = 0.f, next_ = 0.f, fade_ = 0.f;
};

}  // namespace plasma
