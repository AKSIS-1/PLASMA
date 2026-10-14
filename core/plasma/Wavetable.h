#pragma once
// Band-limited, mip-mapped wavetables generated procedurally (no sampled
// material, everything here is original maths).
#include <array>
#include <cmath>

#include "Math.h"
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "Fft.h"

namespace plasma {

class Wavetable {
 public:
  static constexpr int kFrameLen = 2048;   // top-level samples per cycle
  static constexpr int kHarmonics = 1024;  // harmonics at the top level
  static constexpr int kFrames = 64;
  static constexpr int kLevels = 18;       // half-octave mip levels

  using Spectrum = std::vector<Cplx>;  // c[h], h = 0..kHarmonics, complex amplitude of cos series

  std::string name;

  // Band-limit (in harmonics) of mip level j
  static double levelHarmonics(int j) { return kHarmonics * std::pow(2.0, -0.5 * j); }

  int levelFor(float freq, float sampleRate) const {
    const double maxHarm = 0.5 * sampleRate / std::max(1.0f, freq) * 0.92;
    if (maxHarm >= kHarmonics) return 0;
    int j = (int)std::ceil(2.0 * std::log2(kHarmonics / std::max(maxHarm, 1.0)));
    return std::min(kLevels - 1, std::max(0, j));
  }

  // Returns cubic-interpolated sample. framePos in [0, kFrames-1], phase in [0,1).
  inline float lookup(int level, float framePos, float phase) const {
    const int len = len_[level];
    const int mask = len - 1;
    const int f0 = std::min(kFrames - 1, (int)framePos);
    const int f1 = std::min(kFrames - 1, f0 + 1);
    const float ff = framePos - (float)f0;
    const float x = phase * (float)len;
    const int i = (int)x;
    const float t = x - (float)i;
    const float* a = &data_[level][(size_t)f0 * len];
    const float s0 = cubic(a, i, mask, t);
    if (ff < 1e-4f || f1 == f0) return s0;
    const float* b = &data_[level][(size_t)f1 * len];
    return s0 + (cubic(b, i, mask, t) - s0) * ff;
  }

  // Display helper: full-resolution frame (level 0)
  const float* frame(int f) const { return &data_[0][(size_t)f * len_[0]]; }
  int frameLength() const { return len_[0]; }

  void build(const std::vector<Spectrum>& frames) {
    for (int j = 0; j < kLevels; ++j) {
      const double H = levelHarmonics(j);
      int len = 64;
      while (len < 4 * H && len < kFrameLen) len <<= 1;
      len_[j] = len;
      data_[j].assign((size_t)kFrames * len, 0.f);
    }
    for (int f = 0; f < kFrames; ++f) {
      const Spectrum& c = frames[std::min<size_t>(f, frames.size() - 1)];
      // Normalise using the top level reconstruction
      std::vector<double> ref = synth(c, kHarmonics, kFrameLen);
      double peak = 1e-9, rms = 0;
      for (double v : ref) { peak = std::max(peak, std::abs(v)); rms += v * v; }
      rms = std::sqrt(rms / ref.size()) + 1e-9;
      const double scale = std::min(1.0 / peak, 0.5 / rms);
      for (int j = 0; j < kLevels; ++j) {
        const int len = len_[j];
        const int maxH = std::min((int)levelHarmonics(j), len / 2 - 1);
        std::vector<double> s = synth(c, maxH, len);
        float* dst = &data_[j][(size_t)f * len];
        for (int n = 0; n < len; ++n) dst[n] = (float)(s[n] * scale);
      }
    }
  }

 private:
  static inline float cubic(const float* d, int i, int mask, float t) {
    const float y0 = d[(i - 1) & mask], y1 = d[i & mask], y2 = d[(i + 1) & mask], y3 = d[(i + 2) & mask];
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * t + c2) * t + c1) * t + y1;
  }

  // Inverse transform of the first maxH harmonics with a gentle taper at the top.
  static std::vector<double> synth(const Spectrum& c, int maxH, int len) {
    std::vector<Cplx> y(len, Cplx(0, 0));
    const double taperStart = 0.8 * maxH;
    for (int h = 1; h <= maxH && h < len / 2; ++h) {
      double w = 1.0;
      if (h > taperStart) w = 0.5 * (1.0 + std::cos(kPi * (h - taperStart) / (maxH - taperStart + 1e-9)));
      const Cplx v = c[h] * (w * len * 0.5);
      y[h] = v;
      y[len - h] = std::conj(v);
    }
    fft(y, +1);
    std::vector<double> out(len);
    for (int n = 0; n < len; ++n) out[n] = y[n].real() / len;
    return out;
  }

  std::array<std::vector<float>, kLevels> data_;
  std::array<int, kLevels> len_{};
};

// ---------------------------------------------------------------- generators
namespace gen {

using Spectrum = Wavetable::Spectrum;
constexpr int H = Wavetable::kHarmonics;

inline Spectrum zeros() { return Spectrum(H + 1, Cplx(0, 0)); }
// real amplitude with sine phase: a*sin(h*theta)
inline Cplx sinTerm(double a) { return Cplx(0, -a); }

inline Spectrum sine() { auto s = zeros(); s[1] = sinTerm(1); return s; }
inline Spectrum tri() {
  auto s = zeros();
  for (int h = 1; h <= H; h += 2) s[h] = sinTerm(((h / 2) % 2 ? -1.0 : 1.0) * 8.0 / (kPi * kPi * h * h));
  return s;
}
inline Spectrum saw() {
  auto s = zeros();
  for (int h = 1; h <= H; ++h) s[h] = sinTerm((h % 2 ? 1.0 : -1.0) * 2.0 / (kPi * h));
  return s;
}
inline Spectrum square() {
  auto s = zeros();
  for (int h = 1; h <= H; h += 2) s[h] = sinTerm(4.0 / (kPi * h));
  return s;
}
inline Spectrum pulse(double duty) {
  auto s = zeros();
  for (int h = 1; h <= H; ++h)
    s[h] = (2.0 / (kPi * h)) * std::sin(kPi * h * duty) * std::polar(1.0, -kPi * h * duty);
  return s;
}
inline Spectrum lerp(const Spectrum& a, const Spectrum& b, double t) {
  Spectrum o(H + 1);
  for (int h = 0; h <= H; ++h) o[h] = a[h] * (1.0 - t) + b[h] * t;
  return o;
}
// Spectrum from an oversampled time-domain function f(phase in [0,1)).
inline Spectrum fromTime(const std::function<double(double)>& f) {
  const int N = Wavetable::kFrameLen * 4;
  std::vector<Cplx> x(N);
  for (int n = 0; n < N; ++n) x[n] = Cplx(f((double)n / N), 0);
  fft(x, -1);
  Spectrum s = zeros();
  for (int h = 1; h <= H; ++h) s[h] = x[h] * (2.0 / N);
  return s;
}

inline std::vector<Spectrum> basic() {
  const Spectrum keys[5] = {sine(), tri(), saw(), square(), pulse(0.18)};
  std::vector<Spectrum> out;
  for (int f = 0; f < Wavetable::kFrames; ++f) {
    const double u = (double)f / (Wavetable::kFrames - 1) * 4.0;
    const int k = std::min(3, (int)u);
    out.push_back(lerp(keys[k], keys[k + 1], u - k));
  }
  return out;
}
inline std::vector<Spectrum> pwm() {
  std::vector<Spectrum> out;
  for (int f = 0; f < Wavetable::kFrames; ++f)
    out.push_back(pulse(0.5 - 0.47 * (double)f / (Wavetable::kFrames - 1)));
  return out;
}
inline std::vector<Spectrum> harmonicSweep() {
  std::vector<Spectrum> out;
  const Spectrum sw = saw();
  for (int f = 0; f < Wavetable::kFrames; ++f) {
    const double t = (double)f / (Wavetable::kFrames - 1);
    const double hc = 0.9 + 220.0 * t * t;
    auto s = zeros();
    for (int h = 1; h <= H; ++h) s[h] = sw[h] / std::sqrt(1.0 + std::pow(h / hc, 6.0));
    out.push_back(s);
  }
  return out;
}
inline std::vector<Spectrum> vowels() {
  // formant centres (Hz) for A E I O U (+ back to A), fundamental assumed 110 Hz
  static const double F[6][3] = {{800, 1150, 2900}, {400, 1600, 2700}, {270, 2140, 2950},
                                 {450, 800, 2830},  {325, 700, 2700},  {800, 1150, 2900}};
  static const double G[3] = {1.0, 0.6, 0.25};
  static const double BW[3] = {90, 110, 160};
  std::vector<Spectrum> out;
  for (int f = 0; f < Wavetable::kFrames; ++f) {
    const double u = (double)f / (Wavetable::kFrames - 1) * 5.0;
    const int k = std::min(4, (int)u);
    const double t = u - k;
    auto s = zeros();
    for (int h = 1; h <= H; ++h) {
      const double hz = 110.0 * h;
      double a = 0.03;
      for (int i = 0; i < 3; ++i) {
        const double fc = F[k][i] * (1 - t) + F[k + 1][i] * t;
        const double d = (hz - fc) / BW[i];
        a += G[i] * std::exp(-0.5 * d * d);
      }
      s[h] = sinTerm(a / std::pow(h, 0.7));
    }
    out.push_back(s);
  }
  return out;
}
inline std::vector<Spectrum> fm() {
  std::vector<Spectrum> out;
  for (int f = 0; f < Wavetable::kFrames; ++f) {
    const double idx = 9.0 * std::pow((double)f / (Wavetable::kFrames - 1), 1.3);
    out.push_back(fromTime([idx](double p) { return std::sin(2 * kPi * p + idx * std::sin(4 * kPi * p)); }));
  }
  return out;
}
inline std::vector<Spectrum> hardSync() {
  std::vector<Spectrum> out;
  for (int f = 0; f < Wavetable::kFrames; ++f) {
    const double ratio = 1.0 + 7.0 * (double)f / (Wavetable::kFrames - 1);
    out.push_back(fromTime([ratio](double p) {
      double q = p * ratio;
      q -= std::floor(q);
      return 2.0 * q - 1.0;
    }));
  }
  return out;
}
inline std::vector<Spectrum> organ() {
  // drawbar levels for harmonics 1,2,3,4,5,6,8
  static const int hs[7] = {1, 2, 3, 4, 5, 6, 8};
  static const double reg[5][7] = {{1, 0, 0, 0, 0, 0, 0},
                                   {1, 0.6, 0.8, 0, 0, 0, 0},
                                   {0.8, 1, 0.7, 0.6, 0.4, 0.3, 0.2},
                                   {1, 0, 1, 0, 0.8, 0, 0.6},
                                   {0.5, 0.9, 1, 1, 0.9, 0.8, 0.7}};
  std::vector<Spectrum> out;
  for (int f = 0; f < Wavetable::kFrames; ++f) {
    const double u = (double)f / (Wavetable::kFrames - 1) * 4.0;
    const int k = std::min(3, (int)u);
    const double t = u - k;
    auto s = zeros();
    for (int i = 0; i < 7; ++i) s[hs[i]] = sinTerm(reg[k][i] * (1 - t) + reg[k + 1][i] * t);
    out.push_back(s);
  }
  return out;
}
inline std::vector<Spectrum> glass() {
  std::mt19937 rng(1337);
  std::uniform_real_distribution<double> U(0.0, 1.0);
  const int K = 6;
  std::vector<std::vector<double>> amps(K, std::vector<double>(H + 1, 0.0));
  std::vector<double> ph(H + 1);
  for (int h = 1; h <= H; ++h) ph[h] = U(rng) * 2 * kPi;
  for (int k = 0; k < K; ++k)
    for (int h = 1; h <= 200; ++h) {
      const bool on = U(rng) < (k == 0 ? 0.0 : 0.22);
      amps[k][h] = (h == 1 ? 1.0 : (on ? 1.0 : 0.02)) / std::pow(h, 0.6);
    }
  amps[0][1] = 1.0;
  std::vector<Spectrum> out;
  for (int f = 0; f < Wavetable::kFrames; ++f) {
    const double u = (double)f / (Wavetable::kFrames - 1) * (K - 1);
    const int k = std::min(K - 2, (int)u);
    double t = u - k;
    t = t * t * (3 - 2 * t);
    auto s = zeros();
    for (int h = 1; h <= H; ++h) s[h] = std::polar(amps[k][h] * (1 - t) + amps[k + 1][h] * t, ph[h]);
    out.push_back(s);
  }
  return out;
}

}  // namespace gen

class WavetableBank {
 public:
  static constexpr int kCount = 8;
  static const WavetableBank& get() {
    static const WavetableBank bank;
    return bank;
  }
  const Wavetable& table(int i) const { return *tables_[std::min(kCount - 1, std::max(0, i))]; }

 private:
  WavetableBank() {
    const char* names[kCount] = {"Basic Shapes", "PWM Sweep", "Harmonic Sweep", "Vowels",
                                 "Digital FM",   "Hard Sync", "Organ",          "Glass"};
    const std::vector<gen::Spectrum> sets[kCount] = {gen::basic(), gen::pwm(),      gen::harmonicSweep(),
                                                    gen::vowels(), gen::fm(),       gen::hardSync(),
                                                    gen::organ(),  gen::glass()};
    for (int i = 0; i < kCount; ++i) {
      tables_[i] = std::make_unique<Wavetable>();
      tables_[i]->name = names[i];
      tables_[i]->build(sets[i]);
    }
  }
  std::array<std::unique_ptr<Wavetable>, kCount> tables_;
};

}  // namespace plasma
