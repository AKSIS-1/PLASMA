#pragma once
#include <array>
#include <cmath>

#include "Envelope.h"
#include "Filter.h"
#include "Oscillator.h"
#include "Params.h"
#include "Wavetable.h"

namespace plasma {

struct Global {
  const ParamSet* p = nullptr;
  float sampleRate = 48000.f;
  float bpm = 120.f;
  float modWheel = 0.f;
  float aftertouch = 0.f;
  float bendSemis = 0.f;  // already scaled by bend range
};

inline float clamp01(float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }
inline float midiToHz(float note) { return 440.f * std::exp2((note - 69.f) / 12.f); }

class Voice {
 public:
  static constexpr int kControl = 16;

  void prepare(float sr) {
    sr_ = sr;
    kDcR = 1.f - 2.f * (float)kPi * 8.f / sr;
    fadeInc_ = 1.f / (0.004f * sr);
    filter_.prepare(sr);
  }
  bool active() const { return env1_.active(); }
  bool gate() const { return gate_; }
  bool keyDown = false;
  int note() const { return note_; }
  float ampLevel() const { return env1_.level(); }
  uint64_t age = 0;

  void kill() { env1_.kill(); env2_.kill(); env3_.kill(); gate_ = false; keyDown = false; }

  void noteOn(int note, float vel, const Global& g, float startNote, bool legatoChange, uint32_t seed) {
    const ParamSet& p = *g.p;
    note_ = note;
    if (legatoChange && active()) {
      gate_ = true;
      keyDown = true;
      return;
    }
    const bool fresh = !active();
    vel_ = vel;
    rng_.s = seed * 2654435761u + 12345u;
    rnd_ = rng_.next();
    curNote_ = startNote;
    if (fresh) {
      filter_.reset();
      env1_.kill(); env2_.kill(); env3_.kill();
      for (auto& m : lastMod_) m = 0.f;
      fade_ = 0.f;
      dcX_[0] = dcX_[1] = dcY_[0] = dcY_[1] = 0.f;
      for (int i = 0; i < 4; ++i) lfo_[i].reset(0.f, rng_);
      subOsc_.reset();
    }
    if (fresh || true) {
      for (int i = 0; i < 4; ++i) {
        const P base = P::lfo1_shape + i * (int)(P::lfo2_shape - P::lfo1_shape);
        if (p.getb(base + 4)) lfo_[i].reset(0.f, rng_);  // retrig
        else if (fresh) lfo_[i].reset(rng_.next01(), rng_);
      }
      if (fresh) {
        oscA_.reset(p.get(P::a_phase), p.get(P::a_rand), rng_);
        oscB_.reset(p.get(P::b_phase), p.get(P::b_rand), rng_);
      }
    }
    gate_ = true;
    keyDown = true;
    env1_.noteOn(); env2_.noteOn(); env3_.noteOn();
    countdown_ = 0;  // force control update on first sample
  }

  void noteOff() {
    gate_ = false;
    env1_.noteOff(); env2_.noteOff(); env3_.noteOff();
  }

  // adds into L/R
  void render(float* L, float* R, int n, const Global& g) {
    for (int i = 0; i < n; ++i) {
      if (countdown_ <= 0) { controlStep(g); countdown_ = kControl; }
      --countdown_;
      float l = 0.f, r = 0.f;
      if (aOn_) oscA_.tick(l, r);
      if (bOn_) oscB_.tick(l, r);
      if (subOn_) { const float s = subOsc_.tick(subShape_, subInc_) * subLevel_; l += s; r += s; }
      if (noiseLevel_ > 0.f) { l += noiseLevel_ * rng_.next(); r += noiseLevel_ * rng_.next(); }
      if (fltOn_) filter_.process(l, r);
      // DC blocker (~8 Hz): asymmetric waveshapes/warps would otherwise shift the output when
      // a note starts or ends, which is heard as a pop.
      {
        const float yl = l - dcX_[0] + kDcR * dcY_[0]; dcX_[0] = l; dcY_[0] = yl; l = yl;
        const float yr = r - dcX_[1] + kDcR * dcY_[1]; dcX_[1] = r; dcY_[1] = yr; r = yr;
      }
      float e = env1_.tick();
      if (fade_ < 1.f) { fade_ += fadeInc_; if (fade_ > 1.f) fade_ = 1.f; e *= fade_ * fade_ * (3.f - 2.f * fade_); }
      env2_.tick(); env3_.tick();
      ampCur_ += ampStep_;
      panCurL_ += panStepL_;
      panCurR_ += panStepR_;
      const float a = e * ampCur_;
      L[i] += l * a * panCurL_;
      R[i] += r * a * panCurR_;
      if (!env1_.active()) { gate_ = false; break; }
    }
  }

 private:
  void controlStep(const Global& g) {
    const ParamSet& p = *g.p;
    const auto& defs = paramDefs();
    const float dt = kControl / sr_;

    // ---- glide
    const float glide = p.get(P::glide);
    if (glide > 0.002f) curNote_ += ((float)note_ - curNote_) * (1.f - std::exp(-dt / (glide / 3.f)));
    else curNote_ = (float)note_;

    // ---- LFOs (rate may be modulated by previous step's matrix result)
    float lfoVal[4];
    for (int i = 0; i < 4; ++i) {
      const P b = P::lfo1_shape + i * (int)(P::lfo2_shape - P::lfo1_shape);
      float rate;
      const float m = lastMod_[(int)Dest::Lfo1Rate + i];
      if (p.getb(b + 2)) {
        static const float beats[8] = {0.125f, 0.25f, 0.5f, 1.f, 2.f, 4.f, 8.f, 16.f};
        rate = g.bpm / 60.f / beats[std::min(7, std::max(0, p.geti(b + 3)))] * std::exp2(m * 4.f);
      } else {
        rate = defs[(int)b + 1].fromNorm(clamp01(p.norm(b + 1) + m));
      }
      lfoVal[i] = lfo_[i].tick((LfoShape)p.geti(b), rate, dt, p.get(b + 5), rng_);
    }

    // ---- sources
    float src[(int)Src::Count] = {};
    src[(int)Src::Env2] = env2_.level();
    src[(int)Src::Env3] = env3_.level();
    for (int i = 0; i < 4; ++i) src[(int)Src::Lfo1 + i] = lfoVal[i];
    src[(int)Src::Velocity] = vel_;
    src[(int)Src::Key] = ((float)note_ - 60.f) / 60.f;
    src[(int)Src::ModWheel] = g.modWheel;
    src[(int)Src::Aftertouch] = g.aftertouch;
    for (int i = 0; i < 4; ++i) src[(int)Src::Macro1 + i] = p.get(P::macro1 + i);
    src[(int)Src::Random] = rnd_;

    // ---- matrix
    float mod[(int)Dest::Count] = {};
    for (int s = 0; s < kNumSlots; ++s) {
      const P b = P::slot1_src + s * (int)(P::slot2_src - P::slot1_src);
      const int si = p.geti(b), di = p.geti(b + 1);
      if (si > 0 && di > 0) mod[di] += src[si] * p.get(b + 2);
    }
    for (int d = 0; d < (int)Dest::Count; ++d) lastMod_[d] = mod[d];

    // ---- envelopes
    env1_.setParams(p.get(P::env1_a), p.get(P::env1_d), p.get(P::env1_s), p.get(P::env1_r), sr_);
    env2_.setParams(p.get(P::env2_a), p.get(P::env2_d), p.get(P::env2_s), p.get(P::env2_r), sr_);
    env3_.setParams(p.get(P::env3_a), p.get(P::env3_d), p.get(P::env3_s), p.get(P::env3_r), sr_);

    auto val = [&](P id, Dest d) { return defs[(int)id].fromNorm(clamp01(p.norm(id) + mod[(int)d])); };
    auto valNoMod = [&](P id) { return p.get(id); };

    const float pitchAll = curNote_ + g.bendSemis + mod[(int)Dest::Pitch] * 24.f;

    // ---- oscillators
    auto setupOsc = [&](WtOscillator& osc, bool& on, P base, Dest dPos, Dest dWarp, Dest dLvl, Dest dPan,
                        Dest dPitch, Dest dDet) {
      on = p.getb(base);
      if (!on) return;
      const P table = base + 1, pos = base + 2, warp = base + 3, wamt = base + 4, lvl = base + 5,
              pan = base + 6, oct = base + 7, semi = base + 8, fine = base + 9, uni = base + 10,
              det = base + 11, blend = base + 12, stereo = base + 13;
      WtOscillator::Setup s;
      s.wt = &WavetableBank::get().table(p.geti(table));
      s.frame = val(pos, dPos);
      s.warp = (Warp)p.geti(warp);
      s.warpAmt = val(wamt, dWarp);
      s.level = val(lvl, dLvl);
      s.pan = val(pan, dPan);
      s.unison = p.geti(uni);
      s.detune = val(det, dDet);
      s.blend = valNoMod(blend);
      s.stereo = valNoMod(stereo);
      s.sampleRate = sr_;
      const float semis = pitchAll + valNoMod(oct) * 12.f + valNoMod(semi) + valNoMod(fine) / 100.f +
                          mod[(int)dPitch] * 24.f;
      s.baseFreq = midiToHz(semis);
      osc.update(s);
    };
    setupOsc(oscA_, aOn_, P::a_on, Dest::APos, Dest::AWarp, Dest::ALevel, Dest::APan, Dest::APitch, Dest::ADetune);
    setupOsc(oscB_, bOn_, P::b_on, Dest::BPos, Dest::BWarp, Dest::BLevel, Dest::BPan, Dest::BPitch, Dest::BDetune);

    // ---- sub / noise
    subOn_ = p.getb(P::sub_on);
    subShape_ = p.geti(P::sub_shape);
    subLevel_ = val(P::sub_level, Dest::SubLevel) * 0.7f;
    subInc_ = midiToHz(pitchAll + p.get(P::sub_oct) * 12.f) / sr_;
    noiseLevel_ = val(P::noise_level, Dest::NoiseLevel) * 0.5f;

    // ---- filter
    fltOn_ = p.getb(P::flt_on);
    if (fltOn_) {
      float cutoff = val(P::flt_cutoff, Dest::Cutoff);
      cutoff *= std::exp2(p.get(P::flt_key) * (curNote_ - 60.f) / 12.f);
      filter_.set((FilterType)p.geti(P::flt_type), cutoff, val(P::flt_res, Dest::Resonance),
                  val(P::flt_drive, Dest::Drive));
    }

    // ---- amp / pan (linear ramps over the control step)
    const float velGain = 0.25f + 0.75f * vel_;
    const float ampT = velGain * std::min(2.f, std::max(0.f, 1.f + mod[(int)Dest::Amp]));
    const float pan = std::min(1.f, std::max(-1.f, mod[(int)Dest::Pan]));
    const float ang = (pan + 1.f) * 0.25f * (float)kPi;
    const float pl = std::cos(ang) * 1.41421356f, pr = std::sin(ang) * 1.41421356f;
    if (first_) { ampCur_ = ampT; panCurL_ = pl; panCurR_ = pr; first_ = false; }
    ampStep_ = (ampT - ampCur_) / kControl;
    panStepL_ = (pl - panCurL_) / kControl;
    panStepR_ = (pr - panCurR_) / kControl;
  }

  float sr_ = 48000.f, kDcR = 0.999f, fadeInc_ = 0.005f, fade_ = 1.f;
  float dcX_[2] = {}, dcY_[2] = {};
  int note_ = 60, countdown_ = 0;
  bool gate_ = false, first_ = true;
  float vel_ = 1.f, rnd_ = 0.f, curNote_ = 60.f;
  Rng rng_;
  WtOscillator oscA_, oscB_;
  SubOscillator subOsc_;
  Filter filter_;
  Adsr env1_, env2_, env3_;
  Lfo lfo_[4];
  float lastMod_[(int)Dest::Count] = {};
  bool aOn_ = false, bOn_ = false, subOn_ = false, fltOn_ = true;
  int subShape_ = 0;
  float subLevel_ = 0.f, subInc_ = 0.f, noiseLevel_ = 0.f;
  float ampCur_ = 1.f, ampStep_ = 0.f, panCurL_ = 1.f, panCurR_ = 1.f, panStepL_ = 0.f, panStepR_ = 0.f;
};

}  // namespace plasma
