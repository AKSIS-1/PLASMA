#pragma once
#include <cmath>

#include "Math.h"
#include <vector>

#include "Filter.h"
#include "Oscillator.h"
#include "Params.h"

namespace plasma {

class Distortion {
 public:
  void prepare(float sr) { sr_ = sr; }
  void process(float& l, float& r, int type, float drive, float tone, float mix) {
    const float g = 1.f + drive * 30.f;
    const float a = 1.f - std::exp(-6.2831853f * (500.f * std::pow(36.f, tone)) / sr_);
    float in[2] = {l, r}, out[2];
    for (int c = 0; c < 2; ++c) {
      float x = in[c], y;
      switch (type) {
        default:
        case 0: y = std::tanh(x * g) * 0.8f; break;
        case 1: y = std::min(1.f, std::max(-1.f, x * g)) * 0.7f; break;
        case 2: y = triFold(x * g) * 0.7f; break;
        case 3: {
          const float steps = std::exp2(std::max(1.f, 12.f - drive * 10.f));
          const int hold = 1 + (int)(drive * drive * 24.f);
          if (++cnt_[c] >= hold) { cnt_[c] = 0; held_[c] = std::round(x * 0.9f * steps) / steps; }
          y = held_[c];
        } break;
      }
      lp_[c] += a * (y - lp_[c]);
      out[c] = in[c] + (lp_[c] - in[c]) * mix;
    }
    l = out[0];
    r = out[1];
  }

 private:
  float sr_ = 48000.f, lp_[2] = {}, held_[2] = {};
  int cnt_[2] = {};
};

class Chorus {
 public:
  void prepare(float sr) {
    sr_ = sr;
    for (auto& b : buf_) b.assign(4096, 0.f);
  }
  void process(float& l, float& r, float rate, float depth, float mix) {
    phase_ += rate / sr_;
    if (phase_ >= 1.f) phase_ -= 1.f;
    const float in[2] = {l, r};
    float out[2];
    for (int c = 0; c < 2; ++c) {
      buf_[c][pos_] = in[c];
      float wet = 0.f;
      for (int t = 0; t < 2; ++t) {
        float ph = phase_ + 0.25f * c + 0.5f * t;
        ph -= std::floor(ph);
        const float d = (0.009f + depth * 0.005f * std::sin(6.2831853f * ph)) * sr_;
        float rp = (float)pos_ - d;
        if (rp < 0.f) rp += 4096.f;
        const int i0 = (int)rp;
        const float fr = rp - (float)i0;
        wet += buf_[c][i0 & 4095] * (1.f - fr) + buf_[c][(i0 + 1) & 4095] * fr;
      }
      out[c] = in[c] * (1.f - 0.5f * mix) + wet * 0.5f * mix;
    }
    pos_ = (pos_ + 1) & 4095;
    l = out[0];
    r = out[1];
  }

 private:
  float sr_ = 48000.f, phase_ = 0.f;
  std::vector<float> buf_[2];
  int pos_ = 0;
};

class StereoDelay {
 public:
  void prepare(float sr) {
    sr_ = sr;
    size_ = (int)(sr * 2.2f);
    for (auto& b : buf_) b.assign(size_, 0.f);
    pos_ = 0;
  }
  void process(float& l, float& r, float timeMs, float fb, float dampHz, bool pingpong, float mix) {
    const float target = std::min(timeMs, 2000.f) * 0.001f * sr_;
    time_ += (target - time_) * 0.0008f;
    const float a = 1.f - std::exp(-6.2831853f * dampHz / sr_);
    float rd[2];
    for (int c = 0; c < 2; ++c) {
      float rp = (float)pos_ - time_;
      while (rp < 0.f) rp += (float)size_;
      const int i0 = (int)rp;
      const float fr = rp - (float)i0;
      rd[c] = buf_[c][i0 % size_] * (1.f - fr) + buf_[c][(i0 + 1) % size_] * fr;
      lp_[c] += a * (rd[c] - lp_[c]);
    }
    const float inL = l, inR = r;
    if (pingpong) {
      buf_[0][pos_] = softClip((inL + inR) * 0.5f + lp_[1] * fb);
      buf_[1][pos_] = softClip(lp_[0] * fb);
    } else {
      buf_[0][pos_] = softClip(inL + lp_[0] * fb);
      buf_[1][pos_] = softClip(inR + lp_[1] * fb);
    }
    pos_ = (pos_ + 1) % size_;
    l = inL + rd[0] * mix;
    r = inR + rd[1] * mix;
  }

 private:
  float sr_ = 48000.f, time_ = 20000.f, lp_[2] = {};
  int size_ = 1, pos_ = 0;
  std::vector<float> buf_[2];
};

// Freeverb-style reverb
class Reverb {
 public:
  void prepare(float sr) {
    static const int ct[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    static const int at[4] = {556, 441, 341, 225};
    const float k = sr / 44100.f;
    for (int c = 0; c < 2; ++c) {
      for (int i = 0; i < 8; ++i) { comb_[c][i].init((int)((ct[i] + 23 * c) * k)); }
      for (int i = 0; i < 4; ++i) { ap_[c][i].init((int)((at[i] + 23 * c) * k)); }
    }
  }
  void process(float& l, float& r, float size, float damp, float width, float mix) {
    const float fb = 0.7f + 0.28f * size;
    const float in = (l + r) * 0.015f;
    float wl = 0.f, wr = 0.f;
    for (int i = 0; i < 8; ++i) {
      wl += comb_[0][i].tick(in, fb, damp * 0.4f);
      wr += comb_[1][i].tick(in, fb, damp * 0.4f);
    }
    for (int i = 0; i < 4; ++i) { wl = ap_[0][i].tick(wl); wr = ap_[1][i].tick(wr); }
    const float w1 = 0.5f + 0.5f * width, w2 = 0.5f - 0.5f * width;
    const float oL = wl * w1 + wr * w2, oR = wr * w1 + wl * w2;
    l = l * (1.f - 0.5f * mix) + oL * mix * 3.f;
    r = r * (1.f - 0.5f * mix) + oR * mix * 3.f;
  }

 private:
  struct Comb {
    std::vector<float> b; int p = 0; float st = 0.f;
    void init(int n) { b.assign(std::max(1, n), 0.f); p = 0; st = 0.f; }
    float tick(float in, float fb, float damp) {
      const float o = b[p];
      st = o * (1.f - damp) + st * damp;
      b[p] = in + st * fb;
      if (++p >= (int)b.size()) p = 0;
      return o;
    }
  };
  struct Allpass {
    std::vector<float> b; int p = 0;
    void init(int n) { b.assign(std::max(1, n), 0.f); p = 0; }
    float tick(float in) {
      const float bo = b[p];
      const float out = -in + bo;
      b[p] = in + bo * 0.5f;
      if (++p >= (int)b.size()) p = 0;
      return out;
    }
  };
  Comb comb_[2][8];
  Allpass ap_[2][4];
};

class FxChain {
 public:
  void prepare(float sr) {
    dist_.prepare(sr); chorus_.prepare(sr); delay_.prepare(sr); reverb_.prepare(sr);
  }
  void process(float* L, float* R, int n, const ParamSet& p, float bpm) {
    static const float beats[8] = {0.125f, 0.25f, 0.5f, 1.f, 2.f, 4.f, 8.f, 16.f};
    const bool d = p.getb(P::dist_on), c = p.getb(P::cho_on), y = p.getb(P::dly_on), r = p.getb(P::rev_on);
    float dlyMs = p.get(P::dly_time);
    if (p.getb(P::dly_sync)) dlyMs = beats[std::min(7, std::max(0, p.geti(P::dly_div)))] * 60000.f / std::max(20.f, bpm);
    for (int i = 0; i < n; ++i) {
      float l = L[i], rr = R[i];
      if (d) dist_.process(l, rr, p.geti(P::dist_type), p.get(P::dist_drive), p.get(P::dist_tone), p.get(P::dist_mix));
      if (c) chorus_.process(l, rr, p.get(P::cho_rate), p.get(P::cho_depth), p.get(P::cho_mix));
      if (y) delay_.process(l, rr, dlyMs, p.get(P::dly_fb), p.get(P::dly_damp), p.getb(P::dly_pp), p.get(P::dly_mix));
      if (r) reverb_.process(l, rr, p.get(P::rev_size), p.get(P::rev_damp), p.get(P::rev_width), p.get(P::rev_mix));
      L[i] = l;
      R[i] = rr;
    }
  }

 private:
  Distortion dist_;
  Chorus chorus_;
  StereoDelay delay_;
  Reverb reverb_;
};

}  // namespace plasma
