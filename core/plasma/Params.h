#pragma once
// Single source of truth for every Plasma parameter. The DSP core, the JUCE
// wrapper and the presets all derive from the PLASMA_PARAMS list below.
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace plasma {

enum class ParamType { Float, Int, Bool, Choice };
enum class Curve { Lin, Log, Pow2 };

#define PL_TABLES "Basic Shapes|PWM Sweep|Harmonic Sweep|Vowels|Digital FM|Hard Sync|Organ|Glass"
#define PL_WARPS "Off|Sync|Bend +|Bend -|Mirror|Asym|FM Self|Quantize|Fold|Clip|Ring"
#define PL_SUBSHAPES "Sine|Triangle|Saw|Square"
#define PL_FILTERS "LP 12|LP 24|HP 12|HP 24|BP|Notch|Ladder|Comb"
#define PL_LFOSHAPES "Sine|Triangle|Saw Up|Saw Down|Square|S&H|Smooth Rnd"
#define PL_DIVS "1/32|1/16|1/8|1/4|1/2|1 bar|2 bars|4 bars"
#define PL_DISTS "Tanh|Hard|Fold|Crush"
#define PL_SRCS "None|Env 2|Env 3|LFO 1|LFO 2|LFO 3|LFO 4|Velocity|Key|Mod Wheel|Aftertouch|Macro 1|Macro 2|Macro 3|Macro 4|Random"
#define PL_DESTS "None|A Pos|A Warp|A Level|A Pan|A Pitch|A Detune|B Pos|B Warp|B Level|B Pan|B Pitch|B Detune|Sub Level|Noise Level|Cutoff|Resonance|Drive|Pitch|Amp|Pan|LFO1 Rate|LFO2 Rate|LFO3 Rate|LFO4 Rate"

enum class Src { None, Env2, Env3, Lfo1, Lfo2, Lfo3, Lfo4, Velocity, Key, ModWheel, Aftertouch,
                 Macro1, Macro2, Macro3, Macro4, Random, Count };
enum class Dest { None, APos, AWarp, ALevel, APan, APitch, ADetune, BPos, BWarp, BLevel, BPan,
                  BPitch, BDetune, SubLevel, NoiseLevel, Cutoff, Resonance, Drive, Pitch, Amp, Pan,
                  Lfo1Rate, Lfo2Rate, Lfo3Rate, Lfo4Rate, Count };

constexpr int kNumSlots = 10;
constexpr int kMaxUnison = 8;
constexpr int kNumVoices = 16;

// X(id, display name, type, min, max, default, curve, choices)
#define PL_OSC(X, p, N, ON, TBL, POS, LVL, UNI)                                  \
  X(p##_on, N " On", Bool, 0, 1, ON, Lin, "")                                    \
  X(p##_table, N " Table", Choice, 0, 0, TBL, Lin, PL_TABLES)                    \
  X(p##_pos, N " Position", Float, 0, 1, POS, Lin, "")                           \
  X(p##_warp, N " Warp", Choice, 0, 0, 0, Lin, PL_WARPS)                         \
  X(p##_warpamt, N " Warp Amt", Float, 0, 1, 0, Lin, "")                         \
  X(p##_level, N " Level", Float, 0, 1, LVL, Lin, "")                            \
  X(p##_pan, N " Pan", Float, -1, 1, 0, Lin, "")                                 \
  X(p##_oct, N " Octave", Int, -3, 3, 0, Lin, "")                                \
  X(p##_semi, N " Semi", Int, -12, 12, 0, Lin, "")                               \
  X(p##_fine, N " Fine", Float, -100, 100, 0, Lin, "")                           \
  X(p##_uni, N " Unison", Int, 1, 8, UNI, Lin, "")                               \
  X(p##_detune, N " Detune", Float, 0, 1, 0.25, Lin, "")                         \
  X(p##_blend, N " Blend", Float, 0, 1, 0.8, Lin, "")                            \
  X(p##_stereo, N " Stereo", Float, 0, 1, 0.7, Lin, "")                          \
  X(p##_phase, N " Phase", Float, 0, 1, 0, Lin, "")                              \
  X(p##_rand, N " Rand Phase", Float, 0, 1, 1, Lin, "")

#define PL_ENV(X, e, N, A, D, S, R)                                              \
  X(e##_a, N " Attack", Float, 0.001, 10, A, Log, "")                            \
  X(e##_d, N " Decay", Float, 0.001, 10, D, Log, "")                             \
  X(e##_s, N " Sustain", Float, 0, 1, S, Lin, "")                                \
  X(e##_r, N " Release", Float, 0.001, 10, R, Log, "")

#define PL_LFO(X, l, N, SHAPE, RATE)                                             \
  X(l##_shape, N " Shape", Choice, 0, 0, SHAPE, Lin, PL_LFOSHAPES)               \
  X(l##_rate, N " Rate", Float, 0.02, 30, RATE, Log, "")                         \
  X(l##_sync, N " Sync", Bool, 0, 1, 0, Lin, "")                                 \
  X(l##_div, N " Division", Choice, 0, 0, 3, Lin, PL_DIVS)                       \
  X(l##_retrig, N " Retrigger", Bool, 0, 1, 1, Lin, "")                          \
  X(l##_fade, N " Fade In", Float, 0, 5, 0, Pow2, "")

#define PL_SLOT(X, s, N, SRC, DST, AMT)                                          \
  X(s##_src, N " Source", Choice, 0, 0, SRC, Lin, PL_SRCS)                       \
  X(s##_dst, N " Dest", Choice, 0, 0, DST, Lin, PL_DESTS)                        \
  X(s##_amt, N " Amount", Float, -1, 1, AMT, Lin, "")

#define PLASMA_PARAMS(X)                                                         \
  X(master, "Master", Float, 0, 1, 0.7, Lin, "")                                 \
  X(glide, "Glide", Float, 0, 2, 0, Pow2, "")                                    \
  X(mono, "Mono", Bool, 0, 1, 0, Lin, "")                                        \
  X(legato, "Legato", Bool, 0, 1, 0, Lin, "")                                    \
  X(bend_range, "Bend Range", Int, 0, 24, 2, Lin, "")                            \
  PL_OSC(X, a, "A", 1, 0, 0.35, 0.8, 3)                                          \
  PL_OSC(X, b, "B", 0, 2, 0.0, 0.8, 1)                                           \
  X(sub_on, "Sub On", Bool, 0, 1, 0, Lin, "")                                    \
  X(sub_shape, "Sub Shape", Choice, 0, 0, 0, Lin, PL_SUBSHAPES)                  \
  X(sub_oct, "Sub Octave", Int, -3, 0, -1, Lin, "")                              \
  X(sub_level, "Sub Level", Float, 0, 1, 0.5, Lin, "")                           \
  X(noise_level, "Noise Level", Float, 0, 1, 0, Lin, "")                         \
  X(flt_on, "Filter On", Bool, 0, 1, 1, Lin, "")                                 \
  X(flt_type, "Filter Type", Choice, 0, 0, 1, Lin, PL_FILTERS)                   \
  X(flt_cutoff, "Cutoff", Float, 20, 20000, 6000, Log, "")                       \
  X(flt_res, "Resonance", Float, 0, 1, 0.15, Lin, "")                            \
  X(flt_drive, "Drive", Float, 0, 1, 0, Lin, "")                                 \
  X(flt_key, "Key Track", Float, -1, 1, 0, Lin, "")                              \
  PL_ENV(X, env1, "Env 1", 0.003, 0.3, 0.8, 0.25)                                \
  PL_ENV(X, env2, "Env 2", 0.005, 0.4, 0.0, 0.3)                                 \
  PL_ENV(X, env3, "Env 3", 0.01, 1.0, 0.5, 0.5)                                  \
  PL_LFO(X, lfo1, "LFO 1", 0, 2.0)                                               \
  PL_LFO(X, lfo2, "LFO 2", 1, 0.5)                                               \
  PL_LFO(X, lfo3, "LFO 3", 0, 5.0)                                               \
  PL_LFO(X, lfo4, "LFO 4", 5, 1.0)                                               \
  X(macro1, "Macro 1", Float, 0, 1, 0, Lin, "")                                  \
  X(macro2, "Macro 2", Float, 0, 1, 0, Lin, "")                                  \
  X(macro3, "Macro 3", Float, 0, 1, 0, Lin, "")                                  \
  X(macro4, "Macro 4", Float, 0, 1, 0, Lin, "")                                  \
  PL_SLOT(X, slot1, "Mod 1", 1, 15, 0.3)                                         \
  PL_SLOT(X, slot2, "Mod 2", 0, 0, 0)                                            \
  PL_SLOT(X, slot3, "Mod 3", 0, 0, 0)                                            \
  PL_SLOT(X, slot4, "Mod 4", 0, 0, 0)                                            \
  PL_SLOT(X, slot5, "Mod 5", 0, 0, 0)                                            \
  PL_SLOT(X, slot6, "Mod 6", 0, 0, 0)                                            \
  PL_SLOT(X, slot7, "Mod 7", 0, 0, 0)                                            \
  PL_SLOT(X, slot8, "Mod 8", 0, 0, 0)                                            \
  PL_SLOT(X, slot9, "Mod 9", 0, 0, 0)                                            \
  PL_SLOT(X, slot10, "Mod 10", 0, 0, 0)                                          \
  X(dist_on, "Dist On", Bool, 0, 1, 0, Lin, "")                                  \
  X(dist_type, "Dist Type", Choice, 0, 0, 0, Lin, PL_DISTS)                      \
  X(dist_drive, "Dist Drive", Float, 0, 1, 0.3, Lin, "")                         \
  X(dist_tone, "Dist Tone", Float, 0, 1, 0.7, Lin, "")                           \
  X(dist_mix, "Dist Mix", Float, 0, 1, 1, Lin, "")                               \
  X(cho_on, "Chorus On", Bool, 0, 1, 0, Lin, "")                                 \
  X(cho_rate, "Chorus Rate", Float, 0.05, 5, 0.6, Log, "")                       \
  X(cho_depth, "Chorus Depth", Float, 0, 1, 0.5, Lin, "")                        \
  X(cho_mix, "Chorus Mix", Float, 0, 1, 0.4, Lin, "")                            \
  X(dly_on, "Delay On", Bool, 0, 1, 0, Lin, "")                                  \
  X(dly_time, "Delay Time", Float, 1, 1500, 350, Log, "")                        \
  X(dly_sync, "Delay Sync", Bool, 0, 1, 0, Lin, "")                              \
  X(dly_div, "Delay Division", Choice, 0, 0, 2, Lin, PL_DIVS)                    \
  X(dly_fb, "Delay Feedback", Float, 0, 0.95, 0.35, Lin, "")                     \
  X(dly_damp, "Delay Damp", Float, 200, 20000, 8000, Log, "")                   \
  X(dly_pp, "Delay PingPong", Bool, 0, 1, 1, Lin, "")                            \
  X(dly_mix, "Delay Mix", Float, 0, 1, 0.3, Lin, "")                             \
  X(rev_on, "Reverb On", Bool, 0, 1, 0, Lin, "")                                 \
  X(rev_size, "Reverb Size", Float, 0, 1, 0.6, Lin, "")                          \
  X(rev_damp, "Reverb Damp", Float, 0, 1, 0.4, Lin, "")                          \
  X(rev_width, "Reverb Width", Float, 0, 1, 1, Lin, "")                          \
  X(rev_mix, "Reverb Mix", Float, 0, 1, 0.25, Lin, "")

enum class P : int {
#define X(id, ...) id,
  PLASMA_PARAMS(X)
#undef X
      Count
};
constexpr int kNumParams = static_cast<int>(P::Count);

struct ParamDef {
  const char* id;
  const char* name;
  ParamType type;
  float min, max, def;
  Curve curve;
  const char* choices;

  std::vector<std::string> choiceList() const {
    std::vector<std::string> out;
    std::string cur;
    for (const char* c = choices; *c; ++c) {
      if (*c == '|') { out.push_back(cur); cur.clear(); } else cur += *c;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
  }
  int numChoices() const { return (int)choiceList().size(); }
  float maxValue() const { return type == ParamType::Choice ? (float)(numChoices() - 1) : max; }

  float fromNorm(float n) const {
    n = std::min(1.f, std::max(0.f, n));
    const float mx = maxValue();
    float v;
    switch (curve) {
      case Curve::Log: v = min * std::pow(mx / min, n); break;
      case Curve::Pow2: v = min + (mx - min) * n * n; break;
      default: v = min + (mx - min) * n; break;
    }
    if (type != ParamType::Float) v = std::round(v);
    return v;
  }
  float toNorm(float v) const {
    const float mx = maxValue();
    v = std::min(mx, std::max(min, v));
    switch (curve) {
      case Curve::Log: return std::log(v / min) / std::log(mx / min);
      case Curve::Pow2: return std::sqrt(std::max(0.f, (v - min) / (mx - min)));
      default: return mx > min ? (v - min) / (mx - min) : 0.f;
    }
  }
};

inline const std::vector<ParamDef>& paramDefs() {
  static const std::vector<ParamDef> defs = {
#define X(id, name, type, mn, mx, df, curve, choices) \
  {#id, name, ParamType::type, (float)(mn), (float)(mx), (float)(df), Curve::curve, choices},
      PLASMA_PARAMS(X)
#undef X
  };
  return defs;
}

inline int findParam(const std::string& id) {
  const auto& d = paramDefs();
  for (size_t i = 0; i < d.size(); ++i)
    if (id == d[i].id) return (int)i;
  return -1;
}

// Flat array of real-unit parameter values (choice index / bool 0|1 / float).
struct ParamSet {
  float v[kNumParams];
  ParamSet() { reset(); }
  void reset() {
    const auto& d = paramDefs();
    for (int i = 0; i < kNumParams; ++i) v[i] = d[i].def;
  }
  float get(P p) const { return v[(int)p]; }
  int geti(P p) const { return (int)std::lround(v[(int)p]); }
  bool getb(P p) const { return v[(int)p] > 0.5f; }
  float norm(P p) const { return paramDefs()[(int)p].toNorm(v[(int)p]); }
};

constexpr P operator+(P p, int o) { return static_cast<P>(static_cast<int>(p) + o); }
constexpr int operator-(P a, P b) { return static_cast<int>(a) - static_cast<int>(b); }

}  // namespace plasma
