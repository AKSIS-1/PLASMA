#pragma once
// Plain-language help text for every Plasma control (shown as tooltips).
#include <juce_core/juce_core.h>

namespace tips {

inline bool ends(const juce::String& s, const char* e) { return s.endsWith(e); }

inline juce::String warpList() {
  return "Off - plays the wavetable untouched\n"
         "Sync - hard sync: restarts the wave early for tearing, screaming leads\n"
         "Bend + / Bend - - skews the wave to one side, adds even harmonics\n"
         "Mirror - folds the phase back on itself\n"
         "Asym - squeezes one half of the cycle, PWM-like movement\n"
         "FM Self - the oscillator modulates its own pitch for metallic tones\n"
         "Quantize - steps the phase for lo-fi digital grit\n"
         "Fold - wavefolder, folds the wave back when it gets loud\n"
         "Clip - saturates and flattens the peaks\n"
         "Ring - ring-modulates the wave with a harmonic of itself";
}
inline juce::String tableList() {
  return "Basic Shapes - sine > triangle > saw > square > pulse\n"
         "PWM Sweep - pulse wave from 50% down to a thin pulse\n"
         "Harmonic Sweep - a saw that opens from sine to full brightness\n"
         "Vowels - formant filtered voice: A E I O U\n"
         "Digital FM - two-operator FM with rising index\n"
         "Hard Sync - sync sweep, 1x to 8x\n"
         "Organ - drawbar style harmonic mixes\n"
         "Glass - evolving inharmonic-feeling bell/glass spectra";
}
inline juce::String filterList() {
  return "LP 12 / LP 24 - low-pass, removes highs (24 is steeper)\n"
         "HP 12 / HP 24 - high-pass, removes lows\n"
         "BP - band-pass, keeps a band around the cutoff\n"
         "Notch - removes a band around the cutoff\n"
         "Ladder - warm, driven 24 dB low-pass with self-oscillation\n"
         "Comb - tuned resonant echo; cutoff sets the pitch, Reso the ring";
}
inline juce::String distList() {
  return "Tanh - smooth tube-like saturation\n"
         "Hard - hard clipping, aggressive\n"
         "Fold - wavefolding, metallic and bright\n"
         "Crush - bit and sample-rate reduction";
}
inline juce::String lfoShapeList() {
  return "Sine, Triangle - smooth cycles\n"
         "Saw Up / Saw Down - ramps\n"
         "Square - jumps between two values\n"
         "S&H - new random value every cycle (stepped)\n"
         "Smooth Rnd - random values glided between";
}
inline juce::String srcList() {
  return "Env 2 / Env 3 - envelopes, restart on every note\n"
         "LFO 1-4 - low frequency oscillators\n"
         "Velocity - how hard the key was hit\n"
         "Key - note pitch (low notes negative, high notes positive)\n"
         "Mod Wheel / Aftertouch - performance controls\n"
         "Macro 1-4 - the macro knobs\n"
         "Random - a new random value on every note";
}

inline juce::String forParam(const juce::String& id) {
  // ---- oscillators
  if (id.startsWith("a_") || id.startsWith("b_")) {
    const juce::String osc = id.startsWith("a_") ? "A" : "B";
    const juce::String k = id.substring(2);
    if (k == "on") return "Turns oscillator " + osc + " on or off.";
    if (k == "table") return "Wavetable for oscillator " + osc + ". Each table holds 64 frames that morph smoothly as you move Position.\n\n" + tableList();
    if (k == "pos") return "Position in the wavetable: which frame is played. Sweep it or modulate it with an envelope or LFO for evolving sounds.";
    if (k == "warp") return "Warp mode: bends the oscillator's phase or waveform to create new harmonics.\n\n" + warpList();
    if (k == "warpamt") return "How strongly the warp mode is applied. Does nothing while Warp is Off.";
    if (k == "level") return "Output level of oscillator " + osc + ".";
    if (k == "pan") return "Stereo position of oscillator " + osc + ".";
    if (k == "oct") return "Transpose in octaves.";
    if (k == "semi") return "Transpose in semitones.";
    if (k == "fine") return "Fine tune in cents (100 cents = 1 semitone).";
    if (k == "uni") return "Unison: number of stacked, slightly detuned copies of the oscillator. More voices = thicker, wider sound (and more CPU).";
    if (k == "detune") return "How far apart the unison voices are tuned. Small = chorus, large = supersaw.";
    if (k == "blend") return "Balance between the centre unison voice and the outer ones. Low = centre-heavy, high = all voices equal.";
    if (k == "stereo") return "How widely the unison voices are spread across the stereo field.";
    if (k == "phase") return "Starting phase of the wave on each note. Matters most with Rand at 0%.";
    if (k == "rand") return "How much the starting phase is randomised per note and unison voice. 100% = organic, 0% = identical attack every time.";
  }
  // ---- sub / noise / global
  if (id == "sub_on") return "Turns the sub oscillator on or off.";
  if (id == "sub_shape") return "Waveform of the sub oscillator.";
  if (id == "sub_oct") return "Sub oscillator octave relative to the played note.";
  if (id == "sub_level") return "Level of the sub oscillator.";
  if (id == "noise_level") return "Level of white noise mixed into the sound.";
  if (id == "master") return "Overall output volume.";
  if (id == "glide") return "Portamento time: how long pitch takes to slide from the previous note.";
  if (id == "mono") return "Play one note at a time (monophonic) instead of chords.";
  if (id == "legato") return "In Mono mode: overlapping notes glide without retriggering envelopes.";
  if (id == "bend_range") return "Pitch wheel range in semitones.";
  // ---- filter
  if (id == "flt_on") return "Turns the filter on or off.";
  if (id == "flt_type") return "Filter type.\n\n" + filterList();
  if (id == "flt_cutoff") return "Cutoff frequency: where the filter starts to act. Modulate it with an envelope or LFO for classic sweeps.";
  if (id == "flt_res") return "Resonance: emphasises frequencies at the cutoff. High values ring and can self-oscillate.";
  if (id == "flt_drive") return "Saturates the signal going into the filter for a thicker, dirtier tone.";
  if (id == "flt_key") return "Key tracking: moves the cutoff with the note pitch so high notes stay bright.";
  // ---- envelopes
  if (id.startsWith("env")) {
    const juce::String n = id.substring(3, 4);
    const juce::String who = n == "1" ? "Envelope 1 controls the volume of every note." : "Envelope " + n + " is a modulation source (use it in the Mod Matrix).";
    if (ends(id, "_a")) return "Attack: time to rise from silence to full level. " + who;
    if (ends(id, "_d")) return "Decay: time to fall from full level to the Sustain level. " + who;
    if (ends(id, "_s")) return "Sustain: level held while the key is down. " + who;
    if (ends(id, "_r")) return "Release: time to fade out after the key is released. " + who;
  }
  // ---- LFOs
  if (id.startsWith("lfo")) {
    const juce::String n = id.substring(3, 4);
    if (ends(id, "_shape")) return "Shape of LFO " + n + ".\n\n" + lfoShapeList();
    if (ends(id, "_rate")) return "Speed of LFO " + n + " in Hz (ignored while Sync is on).";
    if (ends(id, "_sync")) return "Lock LFO " + n + " to the host tempo using the Division setting.";
    if (ends(id, "_div")) return "Note length of one LFO cycle when Sync is on.";
    if (ends(id, "_retrig")) return "Restart the LFO at the start of every note. Off = free running with a random start.";
    if (ends(id, "_fade")) return "Fade-in time: the LFO starts at zero depth and reaches full strength after this time (delayed vibrato).";
  }
  if (id.startsWith("macro")) return "Macro knob: assign it as a source in the Mod Matrix to control several parameters with one knob. Automate this in FL Studio for big moves.";
  // ---- mod matrix
  if (id.startsWith("slot")) {
    if (ends(id, "_src")) return "Modulation source: what drives this slot.\n\n" + srcList();
    if (ends(id, "_dst")) return "Destination: the parameter this slot modulates. The modulation is added on top of the knob's own value. 'Pitch' moves +/-24 semitones at 100%.";
    if (ends(id, "_amt")) return "Amount of modulation. Negative values invert the source. Double-click to reset to 0.";
  }
  // ---- effects
  if (id == "dist_on") return "Turns distortion on or off.";
  if (id == "dist_type") return "Distortion style.\n\n" + distList();
  if (id == "dist_drive") return "How hard the signal is pushed into the distortion.";
  if (id == "dist_tone") return "Low-pass tone control after the distortion. Lower = darker.";
  if (id == "dist_mix") return "Dry/wet balance of the distortion.";
  if (id == "cho_on") return "Turns the chorus on or off.";
  if (id == "cho_rate") return "Speed of the chorus modulation.";
  if (id == "cho_depth") return "Depth of the chorus modulation. More = wobblier, wider.";
  if (id == "cho_mix") return "Dry/wet balance of the chorus.";
  if (id == "dly_on") return "Turns the delay on or off.";
  if (id == "dly_time") return "Delay time in milliseconds (ignored while Sync is on).";
  if (id == "dly_sync") return "Lock the delay time to the host tempo using the Division setting.";
  if (id == "dly_div") return "Note length of the delay when Sync is on.";
  if (id == "dly_fb") return "Feedback: how many repeats you hear. Very high values build up.";
  if (id == "dly_damp") return "Low-pass inside the feedback loop. Lower = repeats get darker.";
  if (id == "dly_pp") return "Ping-pong: repeats bounce between left and right.";
  if (id == "dly_mix") return "Level of the delayed signal.";
  if (id == "rev_on") return "Turns the reverb on or off.";
  if (id == "rev_size") return "Size of the virtual room: bigger = longer tail.";
  if (id == "rev_damp") return "High-frequency damping of the tail. Higher = darker, softer reverb.";
  if (id == "rev_width") return "Stereo width of the reverb.";
  if (id == "rev_mix") return "Dry/wet balance of the reverb.";
  return {};
}

}  // namespace tips
