#pragma once
#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

#include "Fx.h"
#include "Voice.h"

namespace plasma {

class Synth {
 public:
  void prepare(float sampleRate, int maxBlock) {
    sr_ = sampleRate;
    for (auto& v : voices_) v.prepare(sampleRate);
    fx_.prepare(sampleRate);
    bufL_.assign(std::max(maxBlock, 1), 0.f);
    bufR_.assign(std::max(maxBlock, 1), 0.f);
    allNotesOff(true);
  }

  void setParams(const float* values) { std::memcpy(params_.v, values, sizeof(float) * kNumParams); }
  ParamSet& params() { return params_; }
  void setTempo(double bpm) { global_.bpm = (float)std::max(20.0, bpm); }

  void noteOn(int note, float vel) {
    global_.p = &params_;
    global_.sampleRate = sr_;
    ++counter_;
    if (params_.getb(P::mono)) { monoNoteOn(note, vel); return; }
    int vi = -1;
    // retrigger same note on same voice if still sounding
    for (int i = 0; i < kNumVoices; ++i)
      if (voices_[i].active() && voices_[i].note() == note && voices_[i].keyDown) { vi = i; break; }
    if (vi < 0) vi = allocVoice();
    startVoice(vi, note, vel, false);
    lastNote_ = note;
  }

  void noteOff(int note) {
    if (params_.getb(P::mono)) { monoNoteOff(note); return; }
    for (auto& v : voices_)
      if (v.active() && v.note() == note && v.keyDown) {
        v.keyDown = false;
        if (!sustain_) v.noteOff();
      }
  }

  void allNotesOff(bool hard = false) {
    held_.clear();
    for (auto& v : voices_) { if (hard) v.kill(); else v.noteOff(); }
  }

  void controller(int cc, int value) {
    if (cc == 1) global_.modWheel = value / 127.f;
    else if (cc == 64) {
      const bool down = value >= 64;
      if (sustain_ && !down)
        for (auto& v : voices_) if (v.active() && !v.keyDown && v.gate()) v.noteOff();
      sustain_ = down;
    } else if (cc == 123 || cc == 120) allNotesOff(cc == 120);
  }
  void pitchWheel(float v /*-1..1*/) { bend_ = v; }
  void aftertouch(float v) { global_.aftertouch = v; }

  // Overwrites L/R with n samples of output.
  void render(float* L, float* R, int n) {
    global_.p = &params_;
    global_.sampleRate = sr_;
    global_.bendSemis = bend_ * params_.get(P::bend_range);
    std::fill(L, L + n, 0.f);
    std::fill(R, R + n, 0.f);
    for (auto& v : voices_)
      if (v.active()) v.render(L, R, n, global_);
    fx_.process(L, R, n, params_, global_.bpm);
    const float m = params_.get(P::master);
    const float dcR = 1.f - 2.f * (float)kPi * 6.f / sr_;
    for (int i = 0; i < n; ++i) {
      // master DC blocker (~6 Hz) catches offsets created by distortion
      const float yl = L[i] - dcXl_ + dcR * dcYl_; dcXl_ = L[i]; dcYl_ = yl;
      const float yr = R[i] - dcXr_ + dcR * dcYr_; dcXr_ = R[i]; dcYr_ = yr;
      L[i] = limit(yl * m);
      R[i] = limit(yr * m);
    }
  }

  int activeVoices() const {
    int c = 0;
    for (auto& v : voices_) c += v.active();
    return c;
  }

 private:
  static inline float limit(float x) {
    const float a = std::fabs(x);
    if (a < 0.8f) return x;
    const float s = x < 0 ? -1.f : 1.f;
    return s * (0.8f + 0.2f * std::tanh((a - 0.8f) * 5.f));
  }

  int allocVoice() {
    for (int i = 0; i < kNumVoices; ++i) if (!voices_[i].active()) return i;
    int best = -1;
    float bestLvl = 2.f;
    for (int i = 0; i < kNumVoices; ++i)
      if (!voices_[i].keyDown && voices_[i].ampLevel() < bestLvl) { bestLvl = voices_[i].ampLevel(); best = i; }
    if (best >= 0) return best;
    uint64_t oldest = ~0ull;
    for (int i = 0; i < kNumVoices; ++i) if (voices_[i].age < oldest) { oldest = voices_[i].age; best = i; }
    return best;
  }

  void startVoice(int vi, int note, float vel, bool legato) {
    Voice& v = voices_[vi];
    const bool glide = params_.get(P::glide) > 0.002f && lastNote_ >= 0;
    const float start = (glide && !v.active()) ? (float)lastNote_ : (v.active() ? (float)v.note() : (float)note);
    v.age = counter_;
    v.noteOn(note, vel, global_, start, legato, (uint32_t)counter_);
  }

  void monoNoteOn(int note, float vel) {
    held_.erase(std::remove_if(held_.begin(), held_.end(), [&](auto& h) { return h.first == note; }), held_.end());
    held_.push_back({note, vel});
    const bool legato = params_.getb(P::legato) && voices_[0].active() && voices_[0].gate();
    for (int i = 1; i < kNumVoices; ++i) if (voices_[i].active()) voices_[i].noteOff();
    startVoice(0, note, vel, legato);
    lastNote_ = note;
  }
  void monoNoteOff(int note) {
    held_.erase(std::remove_if(held_.begin(), held_.end(), [&](auto& h) { return h.first == note; }), held_.end());
    if (!held_.empty()) {
      const auto top = held_.back();
      if (voices_[0].note() != top.first) {
        startVoice(0, top.first, top.second, params_.getb(P::legato) && voices_[0].gate());
        lastNote_ = top.first;
      }
    } else {
      voices_[0].keyDown = false;
      if (!sustain_) voices_[0].noteOff();
    }
  }

  float sr_ = 48000.f, bend_ = 0.f, dcXl_ = 0.f, dcYl_ = 0.f, dcXr_ = 0.f, dcYr_ = 0.f;
  bool sustain_ = false;
  int lastNote_ = -1;
  uint64_t counter_ = 0;
  ParamSet params_;
  Global global_;
  std::array<Voice, kNumVoices> voices_;
  std::vector<std::pair<int, float>> held_;
  FxChain fx_;
  std::vector<float> bufL_, bufR_;
};

}  // namespace plasma
