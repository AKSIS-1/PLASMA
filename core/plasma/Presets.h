#pragma once
#include <utility>
#include <vector>

#include "Params.h"

namespace plasma {

struct Preset {
  const char* name;
  std::vector<std::pair<const char*, float>> values;  // real units; everything else = default
};

inline float F(Src s) { return (float)s; }
inline float F(Dest d) { return (float)d; }

inline const std::vector<Preset>& factoryPresets() {
  static const std::vector<Preset> presets = {
      {"Init", {}},
      {"Plasma Lead",
       {{"a_table", 0}, {"a_pos", 0.5f}, {"a_uni", 5}, {"a_detune", 0.3f},
        {"b_on", 1}, {"b_table", 5}, {"b_pos", 0.25f}, {"b_level", 0.45f}, {"b_uni", 3}, {"b_oct", 0},
        {"flt_type", 1}, {"flt_cutoff", 5200}, {"flt_res", 0.25f},
        {"env1_a", 0.004f}, {"env1_d", 0.2f}, {"env1_s", 0.8f}, {"env1_r", 0.3f},
        {"mono", 1}, {"legato", 1}, {"glide", 0.08f}, {"master", 0.5f},
        {"slot1_src", F(Src::Env2)}, {"slot1_dst", F(Dest::BPos)}, {"slot1_amt", 0.5f},
        {"slot2_src", F(Src::Lfo1)}, {"slot2_dst", F(Dest::APitch)}, {"slot2_amt", 0.01f},
        {"lfo1_rate", 5.5f}, {"lfo1_fade", 0.6f}, {"lfo1_retrig", 1},
        {"cho_on", 1}, {"cho_mix", 0.3f}, {"dly_on", 1}, {"dly_mix", 0.2f}, {"dly_sync", 1}, {"dly_div", 2}}},
      {"Sub Bass",
       {{"a_pos", 0.12f}, {"a_uni", 1}, {"sub_on", 1}, {"sub_level", 0.85f}, {"sub_oct", -1},
        {"flt_type", 1}, {"flt_cutoff", 650}, {"flt_res", 0.2f}, {"flt_drive", 0.2f},
        {"env1_a", 0.002f}, {"env1_d", 0.4f}, {"env1_s", 0.9f}, {"env1_r", 0.12f},
        {"env2_d", 0.25f}, {"slot1_amt", 0.35f}, {"mono", 1}, {"glide", 0.04f}}},
      {"Glass Pad",
       {{"a_table", 7}, {"a_pos", 0.3f}, {"a_uni", 6}, {"a_detune", 0.35f}, {"a_stereo", 1},
        {"b_on", 1}, {"b_table", 3}, {"b_pos", 0.5f}, {"b_level", 0.4f}, {"b_uni", 4}, {"b_oct", 1},
        {"flt_type", 0}, {"flt_cutoff", 4500}, {"flt_res", 0.1f},
        {"env1_a", 0.9f}, {"env1_d", 1.5f}, {"env1_s", 0.8f}, {"env1_r", 1.8f},
        {"slot1_src", F(Src::Lfo2)}, {"slot1_dst", F(Dest::APos)}, {"slot1_amt", 0.5f},
        {"slot2_src", F(Src::Lfo4)}, {"slot2_dst", F(Dest::BPos)}, {"slot2_amt", 0.4f},
        {"lfo2_rate", 0.23f}, {"lfo2_retrig", 0}, {"lfo4_shape", 6}, {"lfo4_rate", 0.4f},
        {"cho_on", 1}, {"rev_on", 1}, {"rev_size", 0.85f}, {"rev_mix", 0.35f}}},
      {"Vowel Talker",
       {{"a_table", 3}, {"a_pos", 0.0f}, {"a_uni", 2}, {"a_detune", 0.12f},
        {"flt_type", 0}, {"flt_cutoff", 9000}, {"flt_res", 0.1f},
        {"slot1_src", F(Src::Lfo1)}, {"slot1_dst", F(Dest::APos)}, {"slot1_amt", 0.5f},
        {"slot2_src", F(Src::Macro1)}, {"slot2_dst", F(Dest::APos)}, {"slot2_amt", 0.5f},
        {"lfo1_shape", 1}, {"lfo1_rate", 0.6f}, {"env1_a", 0.05f}, {"env1_r", 0.4f},
        {"dly_on", 1}, {"dly_mix", 0.25f}, {"rev_on", 1}, {"rev_mix", 0.2f}}},
      {"Digital Bell",
       {{"a_table", 4}, {"a_pos", 0.35f}, {"a_oct", 1}, {"a_uni", 2}, {"a_detune", 0.05f},
        {"flt_cutoff", 14000}, {"flt_res", 0.05f},
        {"env1_a", 0.001f}, {"env1_d", 1.8f}, {"env1_s", 0.0f}, {"env1_r", 1.4f},
        {"env2_d", 0.9f}, {"slot1_src", F(Src::Env2)}, {"slot1_dst", F(Dest::APos)}, {"slot1_amt", 0.55f},
        {"rev_on", 1}, {"rev_size", 0.75f}, {"rev_mix", 0.3f}}},
      {"Organ Wobble",
       {{"a_table", 6}, {"a_pos", 0.5f}, {"a_uni", 2}, {"a_detune", 0.08f},
        {"flt_cutoff", 9000}, {"env1_a", 0.012f}, {"env1_d", 0.1f}, {"env1_s", 1.0f}, {"env1_r", 0.14f},
        {"slot1_src", F(Src::Lfo1)}, {"slot1_dst", F(Dest::APos)}, {"slot1_amt", 0.3f},
        {"lfo1_rate", 0.4f}, {"lfo1_shape", 1}, {"cho_on", 1}, {"dist_on", 1}, {"dist_drive", 0.12f}, {"dist_mix", 0.6f}}},
      {"Acid Pluck",
       {{"a_pos", 0.5f}, {"a_uni", 1}, {"flt_type", 6}, {"flt_cutoff", 500}, {"flt_res", 0.65f}, {"flt_drive", 0.3f},
        {"env1_a", 0.002f}, {"env1_d", 0.3f}, {"env1_s", 0.6f}, {"env1_r", 0.1f},
        {"env2_d", 0.22f}, {"slot1_amt", 0.55f}, {"mono", 1}, {"glide", 0.06f},
        {"dly_on", 1}, {"dly_sync", 1}, {"dly_div", 2}, {"dly_mix", 0.25f}}},
      {"Wide Supersaw",
       {{"a_pos", 0.5f}, {"a_uni", 8}, {"a_detune", 0.55f}, {"a_stereo", 1}, {"a_blend", 0.9f},
        {"b_on", 1}, {"b_pos", 0.5f}, {"b_uni", 4}, {"b_detune", 0.3f}, {"b_level", 0.4f}, {"b_oct", -1},
        {"flt_type", 1}, {"flt_cutoff", 9500}, {"env1_a", 0.01f}, {"env1_s", 0.9f}, {"env1_r", 0.4f},
        {"slot1_amt", 0.0f}, {"cho_on", 1}, {"rev_on", 1}, {"rev_mix", 0.2f}}},
      {"Fold Growl",
       {{"a_table", 4}, {"a_pos", 0.3f}, {"a_warp", 8}, {"a_warpamt", 0.3f}, {"a_uni", 3}, {"a_detune", 0.15f},
        {"sub_on", 1}, {"sub_level", 0.6f}, {"sub_oct", -1},
        {"flt_type", 1}, {"flt_cutoff", 1800}, {"flt_res", 0.3f}, {"flt_drive", 0.4f},
        {"slot1_src", F(Src::Lfo3)}, {"slot1_dst", F(Dest::AWarp)}, {"slot1_amt", 0.5f},
        {"slot2_src", F(Src::Lfo3)}, {"slot2_dst", F(Dest::Cutoff)}, {"slot2_amt", 0.2f},
        {"lfo3_rate", 3.0f}, {"lfo3_sync", 1}, {"lfo3_div", 2}, {"lfo3_shape", 1},
        {"mono", 1}, {"dist_on", 1}, {"dist_type", 2}, {"dist_drive", 0.35f}, {"dist_mix", 0.5f}}},
  };
  return presets;
}

inline void applyPreset(ParamSet& ps, const Preset& pr) {
  ps.reset();
  for (const auto& kv : pr.values) {
    const int i = findParam(kv.first);
    if (i >= 0) ps.v[i] = kv.second;
  }
}

}  // namespace plasma
