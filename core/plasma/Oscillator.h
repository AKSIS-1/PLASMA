#pragma once
#include <cmath>

#include "Math.h"
#include <cstdint>

#include "Params.h"
#include "Wavetable.h"

namespace plasma {

enum class Warp { Off, Sync, BendPlus, BendMinus, Mirror, Asym, FmSelf, Quantize, Fold, Clip, Ring };

struct Rng {
  uint32_t s = 0x1234567u;
  inline float next() {  // [-1,1)
    s = s * 1664525u + 1013904223u;
    return (float)(int32_t)s * (1.0f / 2147483648.0f);
  }
  inline float next01() { return next() * 0.5f + 0.5f; }
};

inline float triFold(float x) {  // folds any x into [-1,1]
  x = x * 0.25f + 0.25f;
  x -= std::floor(x);
  return std::fabs(x * 4.f - 2.f) - 1.f;
}

// Frequency multiplier used to pick a safe mip level for a warp mode
inline float warpFreqMult(Warp m, float a) {
  switch (m) {
    case Warp::Sync: return 1.f + 7.f * a;
    case Warp::FmSelf: return 1.f + 3.f * a;
    case Warp::Ring: return 1.f + 4.f * a;
    case Warp::BendPlus: case Warp::BendMinus: case Warp::Asym: return 1.f + 2.f * a;
    case Warp::Mirror: return 1.f + 2.f * a;
    case Warp::Fold: case Warp::Clip: return 1.f + 2.f * a;
    default: return 1.f;
  }
}

// One wavetable oscillator with up to 8 unison voices.
class WtOscillator {
 public:
  struct Setup {
    const Wavetable* wt = nullptr;
    float baseFreq = 440.f;     // Hz
    float frame = 0.f;          // 0..1
    Warp warp = Warp::Off;
    float warpAmt = 0.f;
    int unison = 1;
    float detune = 0.f;         // 0..1
    float blend = 0.8f;
    float stereo = 0.7f;
    float sampleRate = 48000.f;
    float level = 1.f;
    float pan = 0.f;            // -1..1
  };

  void reset(float startPhase, float randAmt, Rng& rng) {
    for (int u = 0; u < kMaxUnison; ++u) {
      phase_[u] = startPhase + randAmt * rng.next01();
      phase_[u] -= std::floor(phase_[u]);
    }
  }

  void update(const Setup& s) {
    wt_ = s.wt;
    frame_ = std::min(1.f, std::max(0.f, s.frame)) * (Wavetable::kFrames - 1);
    warp_ = s.warp;
    amt_ = std::min(1.f, std::max(0.f, s.warpAmt));
    n_ = std::min(kMaxUnison, std::max(1, s.unison));
    const float spread = 100.f * std::pow(std::min(1.f, std::max(0.f, s.detune)), 1.5f);
    const float mult = warpFreqMult(warp_, amt_);
    float gains[kMaxUnison], sumsq = 0.f;
    for (int u = 0; u < n_; ++u) {
      const float offs = n_ > 1 ? ((float)u / (float)(n_ - 1)) * 2.f - 1.f : 0.f;
      const float f = s.baseFreq * std::exp2(offs * spread / 1200.f);
      inc_[u] = f / s.sampleRate;
      level_[u] = wt_->levelFor(f * mult, s.sampleRate);
      const float center = 1.f - std::fabs(offs);
      gains[u] = (1.f - s.blend) * center + s.blend;
      sumsq += gains[u] * gains[u];
      const float pan = std::min(1.f, std::max(-1.f, offs * s.stereo + s.pan));
      const float ang = (pan + 1.f) * 0.25f * (float)kPi;
      pl_[u] = std::cos(ang);
      pr_[u] = std::sin(ang);
    }
    const float norm = s.level / std::sqrt(sumsq);
    for (int u = 0; u < n_; ++u) { gl_[u] = gains[u] * norm * pl_[u] * 1.41421356f; gr_[u] = gains[u] * norm * pr_[u] * 1.41421356f; }
  }

  inline void tick(float& outL, float& outR) {
    float l = 0.f, r = 0.f;
    for (int u = 0; u < n_; ++u) {
      const float y = shape(phase_[u], level_[u]);
      l += y * gl_[u];
      r += y * gr_[u];
      phase_[u] += inc_[u];
      if (phase_[u] >= 1.f) phase_[u] -= 1.f;
    }
    outL += l;
    outR += r;
  }

 private:
  inline float shape(float p, int lvl) const {
    float q = p;
    float post = 0.f;  // amplitude-domain parameter
    switch (warp_) {
      case Warp::Off: break;
      case Warp::Sync: q = p * (1.f + 7.f * amt_); q -= std::floor(q); break;
      case Warp::BendPlus: q = std::pow(p, 1.f + 3.f * amt_); break;
      case Warp::BendMinus: q = 1.f - std::pow(1.f - p, 1.f + 3.f * amt_); break;
      case Warp::Mirror: {
        float t = p * (1.f + 3.f * amt_);
        t = 0.5f * t; t -= std::floor(t);
        const float m = std::fabs(t * 2.f - 1.f);
        q = p * (1.f - amt_) + m * amt_;
        break;
      }
      case Warp::Asym: {
        const float c = 0.5f + 0.45f * (amt_ * 2.f - 1.f) * 0.999f;
        q = p < c ? 0.5f * p / c : 0.5f + 0.5f * (p - c) / (1.f - c);
        break;
      }
      case Warp::FmSelf: q = p + amt_ * 0.35f * std::sin(6.2831853f * p); q -= std::floor(q); break;
      case Warp::Quantize: {
        const float steps = std::exp2(7.f - 6.f * amt_);
        q = std::floor(p * steps) / steps;
        break;
      }
      case Warp::Fold: case Warp::Clip: case Warp::Ring: post = amt_; break;
    }
    float y = wt_->lookup(lvl, frame_, q);
    switch (warp_) {
      case Warp::Fold: y = triFold(y * (1.f + 6.f * post)); break;
      case Warp::Clip: { const float g = 1.f + 9.f * post; y = std::tanh(y * g) / std::tanh(g * 0.7f + 0.3f); } break;
      case Warp::Ring: {
        const float k = 1.f + std::floor(post * 8.f);
        const float rm = y * std::sin(6.2831853f * p * k);
        y = y + (rm - y) * std::min(1.f, post * 2.f);
        break;
      }
      default: break;
    }
    return y;
  }

  const Wavetable* wt_ = nullptr;
  float frame_ = 0.f, amt_ = 0.f;
  Warp warp_ = Warp::Off;
  int n_ = 1;
  float phase_[kMaxUnison] = {};
  float inc_[kMaxUnison] = {};
  int level_[kMaxUnison] = {};
  float gl_[kMaxUnison] = {}, gr_[kMaxUnison] = {}, pl_[kMaxUnison] = {}, pr_[kMaxUnison] = {};
};

// Sub oscillator (naive-with-polyBLEP shapes)
class SubOscillator {
 public:
  void reset() { phase_ = 0.f; }
  inline float tick(int shape, float inc) {
    float p = phase_;
    float y;
    switch (shape) {
      default:
      case 0: y = std::sin(6.2831853f * p); break;
      case 1: y = 4.f * std::fabs(p - 0.5f) - 1.f; break;
      case 2: y = 2.f * p - 1.f - polyBlep(p, inc); break;
      case 3: y = (p < 0.5f ? 1.f : -1.f) + polyBlep(p, inc) - polyBlep(std::fmod(p + 0.5f, 1.f), inc); break;
    }
    phase_ += inc;
    if (phase_ >= 1.f) phase_ -= 1.f;
    return y;
  }

 private:
  static inline float polyBlep(float t, float dt) {
    if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
  }
  float phase_ = 0.f;
};

}  // namespace plasma
