// Headless sanity tests for the Plasma DSP core (no JUCE needed).
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "plasma/Presets.h"
#include "plasma/Synth.h"

using namespace plasma;

static int failures = 0;
#define CHECK(cond, ...)                                  \
  do {                                                    \
    if (!(cond)) { std::printf("FAIL: " __VA_ARGS__); std::printf("\n"); ++failures; } \
    else { std::printf("ok:   " __VA_ARGS__); std::printf("\n"); }                  \
  } while (0)

static bool finiteAndBounded(const std::vector<float>& v, float lim, float& peak) {
  peak = 0;
  for (float x : v) {
    if (!std::isfinite(x)) return false;
    peak = std::max(peak, std::fabs(x));
  }
  return peak <= lim;
}

// Fraction of spectral energy that is NOT at harmonics of f0 (aliasing/noise proxy)
static double inharmonicRatio(const std::vector<float>& x, double f0, double sr) {
  const int N = 1 << 15;
  std::vector<Cplx> a(N);
  for (int i = 0; i < N; ++i) {
    const double w = 0.5 - 0.5 * std::cos(2 * kPi * i / N);  // Hann
    a[i] = x[i] * w;
  }
  fft(a, -1);
  double harm = 0, other = 0;
  for (int k = 1; k < N / 2; ++k) {
    const double f = k * sr / N;
    const double e = std::norm(a[k]);
    const double h = f / f0;
    const double dist = std::fabs(h - std::round(h)) * f0;  // Hz away from nearest harmonic
    if (dist < 4.0 * sr / N) harm += e; else other += e;
  }
  return other / (harm + other);
}

static std::vector<float> renderNote(ParamSet& setup, int note, float seconds, float sr, bool left = true) {
  Synth s;
  s.prepare(sr, 512);
  s.setParams(setup.v);
  s.noteOn(note, 1.f);
  const int total = (int)(seconds * sr);
  std::vector<float> out(total);
  std::vector<float> L(512), R(512);
  int done = 0;
  while (done < total) {
    const int n = std::min(512, total - done);
    s.render(L.data(), R.data(), n);
    for (int i = 0; i < n; ++i) out[done + i] = left ? L[i] : R[i];
    done += n;
  }
  return out;
}

int main() {
  const float sr = 48000.f;
  std::printf("building wavetables...\n");
  const auto& bank = WavetableBank::get();
  CHECK(bank.table(0).frameLength() == 2048, "wavetable bank built");

  // 1. All tables x warps: finite + bounded
  {
    bool ok = true;
    float worst = 0;
    for (int t = 0; t < WavetableBank::kCount; ++t)
      for (int w = 0; w < 11; ++w) {
        ParamSet p;
        p.v[(int)P::a_table] = t;
        p.v[(int)P::a_warp] = w;
        p.v[(int)P::a_warpamt] = 0.7f;
        p.v[(int)P::a_uni] = 5;
        p.v[(int)P::a_pos] = 0.6f;
        p.v[(int)P::flt_type] = 0;
        p.v[(int)P::flt_cutoff] = 20000;
        auto o = renderNote(p, 48, 0.3f, sr);
        float pk;
        ok &= finiteAndBounded(o, 1.0f, pk);
        worst = std::max(worst, pk);
      }
    CHECK(ok, "all 8 tables x 11 warp modes finite and under limiter (peak %.2f)", worst);
  }

  // 2. All filter types stable at extreme settings
  {
    bool ok = true;
    for (int ft = 0; ft < 8; ++ft)
      for (float res : {0.0f, 0.7f, 1.0f})
        for (float cut : {20.f, 440.f, 19000.f}) {
          ParamSet p;
          p.v[(int)P::flt_type] = ft;
          p.v[(int)P::flt_res] = res;
          p.v[(int)P::flt_cutoff] = cut;
          p.v[(int)P::flt_drive] = 1.f;
          p.v[(int)P::slot1_amt] = 0.f;
          auto o = renderNote(p, 60, 0.25f, sr);
          float pk;
          ok &= finiteAndBounded(o, 1.0f, pk);
        }
    CHECK(ok, "8 filter types x extreme settings stable");
  }

  // 3. Anti-aliasing: high saw note should be mostly harmonic energy
  {
    ParamSet p;
    p.v[(int)P::a_pos] = 0.5f;           // saw region of "Basic Shapes"
    p.v[(int)P::a_table] = 0;
    p.v[(int)P::a_uni] = 1;
    p.v[(int)P::flt_on] = 0;
    p.v[(int)P::env1_a] = 0.001f;
    p.v[(int)P::slot1_amt] = 0.f;
    p.v[(int)P::master] = 0.5f;
    auto o = renderNote(p, 96, 1.2f, sr);  // C7 ~ 2093 Hz
    std::vector<float> seg(o.begin() + 8000, o.begin() + 8000 + (1 << 15));
    const double r = inharmonicRatio(seg, 2093.0, sr);
    CHECK(r < 0.001, "saw at C7 inharmonic energy %.5f (<0.001)", r);
    auto o2 = renderNote(p, 60, 1.2f, sr);
    std::vector<float> seg2(o2.begin() + 8000, o2.begin() + 8000 + (1 << 15));
    const double r2 = inharmonicRatio(seg2, 261.63, sr);
    CHECK(r2 < 0.001, "saw at C4 inharmonic energy %.5f (<0.001)", r2);
  }

  // 4. Polyphony / note off / silence after release
  {
    ParamSet p;
    p.v[(int)P::env1_r] = 0.05f;
    Synth s;
    s.prepare(sr, 512);
    s.setParams(p.v);
    for (int n = 40; n < 60; ++n) s.noteOn(n, 0.8f);  // 20 notes -> stealing
    std::vector<float> L(512), R(512);
    for (int i = 0; i < 20; ++i) s.render(L.data(), R.data(), 512);
    CHECK(s.activeVoices() == 16, "voice stealing caps at 16 (got %d)", s.activeVoices());
    for (int n = 40; n < 60; ++n) s.noteOff(n);
    for (int i = 0; i < 200; ++i) s.render(L.data(), R.data(), 512);
    CHECK(s.activeVoices() == 0, "all voices released (%d active)", s.activeVoices());
  }

  // 5. Mod matrix: LFO->cutoff changes the output; macro->level works
  {
    ParamSet p;
    p.v[(int)P::slot1_amt] = 0.f;
    auto base = renderNote(p, 48, 0.4f, sr);
    p.v[(int)P::slot1_src] = (float)Src::Lfo1;
    p.v[(int)P::slot1_dst] = (float)Dest::Cutoff;
    p.v[(int)P::slot1_amt] = 0.8f;
    p.v[(int)P::lfo1_rate] = 8.f;
    auto modded = renderNote(p, 48, 0.4f, sr);
    double diff = 0;
    for (size_t i = 0; i < base.size(); ++i) diff += std::fabs(base[i] - modded[i]);
    CHECK(diff > 1.0, "mod matrix LFO1 -> cutoff alters output (diff %.1f)", diff);
  }

  // 6. FX chain finite
  {
    ParamSet p;
    for (P on : {P::dist_on, P::cho_on, P::dly_on, P::rev_on}) p.v[(int)on] = 1.f;
    p.v[(int)P::dist_drive] = 1.f;
    for (int t = 0; t < 4; ++t) {
      p.v[(int)P::dist_type] = (float)t;
      auto o = renderNote(p, 55, 0.6f, sr);
      float pk;
      CHECK(finiteAndBounded(o, 1.0f, pk), "FX chain finite, dist type %d (peak %.2f)", t, pk);
    }
  }

  // 7. Different sample rates
  for (float rate : {44100.f, 96000.f}) {
    ParamSet p;
    p.v[(int)P::a_uni] = 8;
    auto o = renderNote(p, 72, 0.3f, rate);
    float pk;
    CHECK(finiteAndBounded(o, 1.0f, pk) && pk > 0.01f, "sample rate %.0f renders audio (peak %.2f)", rate, pk);
  }

  // 8. Factory presets: valid ids, audible, finite
  for (const auto& pr : factoryPresets()) {
    for (const auto& kv : pr.values)
      CHECK(findParam(kv.first) >= 0, "preset '%s' param id '%s' exists", pr.name, kv.first);
    ParamSet p;
    applyPreset(p, pr);
    auto o = renderNote(p, 48, 0.5f, sr);
    float pk;
    const bool fin = finiteAndBounded(o, 1.0f, pk);
    CHECK(fin && pk > 0.05f, "preset '%s' audible and finite (peak %.2f)", pr.name, pk);
  }

  // 9. Declick: no DC offset and a clean onset/release (regression for note-start pops)
  {
    auto dcMean = [&](ParamSet p, int note) {
      auto o = renderNote(p, note, 1.0f, sr);
      double m = 0; int c = 0;
      for (size_t i = (size_t)(0.4f * sr); i < o.size(); ++i) { m += o[i]; ++c; }
      return m / c;
    };
    ParamSet organ;
    for (const auto& pr : factoryPresets()) if (std::string(pr.name) == "Organ Wobble") applyPreset(organ, pr);
    CHECK(std::fabs(dcMean(organ, 48)) < 0.003, "Organ Wobble has no DC offset (mean %.5f)", dcMean(organ, 48));
    ParamSet asym; asym.v[(int)P::a_warp] = 5; asym.v[(int)P::a_warpamt] = 0.6f;
    CHECK(std::fabs(dcMean(asym, 48)) < 0.003, "Asym warp has no DC offset (mean %.5f)", dcMean(asym, 48));
    ParamSet dist; dist.v[(int)P::dist_on] = 1; dist.v[(int)P::dist_drive] = 0.8f; dist.v[(int)P::a_table] = 6;
    CHECK(std::fabs(dcMean(dist, 48)) < 0.01, "Distortion keeps DC low (mean %.5f)", dcMean(dist, 48));

    // first samples start from silence, even with 1 ms attack requested
    ParamSet fast; fast.v[(int)P::env1_a] = 0.001f;
    for (int note : {36, 60, 84}) {
      auto o = renderNote(fast, note, 0.1f, sr);
      CHECK(std::fabs(o[0]) < 0.001f, "note %d starts from silence (%.5f)", note, o[0]);
    }
    // attack is monotone (no overshoot) after the minimum-time clamp
    Adsr env; env.setParams(0.001f, 0.2f, 0.7f, 0.2f, sr); env.noteOn();
    float prev = 0, worstDrop = 0;
    for (int i = 0; i < 140; ++i) { float v = env.tick(); worstDrop = std::max(worstDrop, prev - v); prev = v; }
    CHECK(worstDrop < 1e-6f, "attack stage is monotone");
  }

  std::printf("\n%s (%d failures)\n", failures ? "FAILED" : "ALL PASSED", failures);
  return failures ? 1 : 0;
}
