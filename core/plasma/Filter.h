#pragma once
#include <cmath>

#include "Math.h"
#include <vector>

namespace plasma {

enum class FilterType { LP12, LP24, HP12, HP24, BP, Notch, Ladder, Comb };

inline float softClip(float x) {
  if (x > 3.f) x = 3.f; else if (x < -3.f) x = -3.f;
  return x * (27.f + x * x) / (27.f + 9.f * x * x);
}

// Stereo multi-mode filter: ZDF state-variable (12/24), ZDF ladder, tuned comb.
class Filter {
 public:
  void prepare(float sampleRate) {
    sr_ = sampleRate;
    combBuf_[0].assign(8192, 0.f);
    combBuf_[1].assign(8192, 0.f);
    reset();
  }
  void reset() {
    for (int c = 0; c < 2; ++c) {
      for (int s = 0; s < 2; ++s) ic1_[c][s] = ic2_[c][s] = 0.f;
      for (int i = 0; i < 4; ++i) lad_[c][i] = 0.f;
      std::fill(combBuf_[c].begin(), combBuf_[c].end(), 0.f);
    }
    combPos_ = 0;
  }

  void set(FilterType t, float cutoff, float res, float drive) {
    type_ = t;
    cutoff = std::min(std::max(cutoff, 15.f), 0.49f * sr_);
    res = std::min(1.f, std::max(0.f, res));
    drive_ = 1.f + drive * 7.f;
    driveComp_ = 1.f / std::sqrt(drive_);
    switch (t) {
      case FilterType::Ladder: {
        g_ = std::tan((float)kPi * cutoff / sr_);
        G_ = g_ / (1.f + g_);
        kf_ = 4.f * res * 0.98f;
        break;
      }
      case FilterType::Comb: {
        delay_ = std::min(8000.f, std::max(2.f, sr_ / cutoff));
        fb_ = res * 0.96f;
        break;
      }
      default: {
        g_ = std::tan((float)kPi * cutoff / sr_);
        float k = 2.f * std::exp(-res * 4.60517f);
        if (t == FilterType::LP24 || t == FilterType::HP24) k *= 1.4f;
        k_ = k;
        a1_ = 1.f / (1.f + g_ * (g_ + k_));
        a2_ = g_ * a1_;
        a3_ = g_ * a2_;
      }
    }
  }

  inline void process(float& l, float& r) {
    float in[2] = {l, r};
    for (int c = 0; c < 2; ++c) {
      float x = in[c];
      if (drive_ > 1.001f) x = softClip(x * drive_) * driveComp_;
      switch (type_) {
        case FilterType::LP12: x = svf(c, 0, x, 0); break;
        case FilterType::LP24: x = svf(c, 1, svf(c, 0, x, 0), 0); break;
        case FilterType::HP12: x = svf(c, 0, x, 1); break;
        case FilterType::HP24: x = svf(c, 1, svf(c, 0, x, 1), 1); break;
        case FilterType::BP: x = svf(c, 0, x, 2) * k_ ; break;
        case FilterType::Notch: x = svf(c, 0, x, 3); break;
        case FilterType::Ladder: x = ladder(c, x); break;
        case FilterType::Comb: x = comb(c, x); break;
      }
      in[c] = x;
    }
    if (type_ == FilterType::Comb) combPos_ = (combPos_ + 1) & 8191;
    l = in[0];
    r = in[1];
  }

 private:
  inline float svf(int c, int stage, float v0, int mode) {
    float& ic1 = ic1_[c][stage];
    float& ic2 = ic2_[c][stage];
    const float v3 = v0 - ic2;
    const float v1 = a1_ * ic1 + a2_ * v3;
    const float v2 = ic2 + a2_ * ic1 + a3_ * v3;
    ic1 = 2.f * v1 - ic1;
    ic2 = 2.f * v2 - ic2;
    switch (mode) {
      case 0: return v2;
      case 1: return v0 - k_ * v1 - v2;
      case 2: return v1;
      default: return v0 - k_ * v1;
    }
  }
  inline float ladder(int c, float x) {
    float* s = lad_[c];
    const float beta = 1.f / (1.f + g_);
    const float S = G_ * G_ * G_ * beta * s[0] + G_ * G_ * beta * s[1] + G_ * beta * s[2] + beta * s[3];
    const float G4 = G_ * G_ * G_ * G_;
    const float y4 = (G4 * x + S) / (1.f + kf_ * G4);
    float u = x - kf_ * y4;
    u = softClip(u * 0.8f) * 1.25f;
    float in = u;
    for (int i = 0; i < 4; ++i) {
      const float v = (in - s[i]) * G_;
      const float y = v + s[i];
      s[i] = y + v;
      in = y;
    }
    return in * (1.f + 0.4f * kf_ / 4.f);
  }
  inline float comb(int c, float x) {
    std::vector<float>& b = combBuf_[c];
    float rp = (float)combPos_ - delay_;
    while (rp < 0.f) rp += 8192.f;
    const int i0 = (int)rp;
    const float fr = rp - (float)i0;
    const float d = b[i0 & 8191] * (1.f - fr) + b[(i0 + 1) & 8191] * fr;
    float y = x + fb_ * d;
    y = softClip(y);
    b[combPos_] = y;
    return y * (1.f - 0.4f * fb_);
  }

  float sr_ = 48000.f;
  FilterType type_ = FilterType::LP12;
  float g_ = 0.f, G_ = 0.f, k_ = 1.f, a1_ = 0.f, a2_ = 0.f, a3_ = 0.f, kf_ = 0.f;
  float drive_ = 1.f, driveComp_ = 1.f, delay_ = 100.f, fb_ = 0.f;
  float ic1_[2][2] = {}, ic2_[2][2] = {}, lad_[2][4] = {};
  std::vector<float> combBuf_[2];
  int combPos_ = 0;
};

}  // namespace plasma
