# Plasma

An original wavetable synthesizer (VST3) in the spirit of modern hybrid synths. Everything here -
the wavetable generators, DSP and UI - is written from scratch; no third-party synth code,
samples or presets are used.

## Install on Windows 11 (FL Studio)

1. Open the **Actions** tab on GitHub, pick the latest green *Build Plasma* run and download the
   **Plasma-VST3-Windows-x64** artifact (a zip).
2. Unzip it and copy the whole **`Plasma.vst3`** folder to  
   `C:\Program Files\Common Files\VST3\`
3. In FL Studio: *Options -> Manage plugins -> Find installed plugins* (enable "Verify plugins" if
   asked). Plasma appears under *Plasma* in the plugin picker (Add -> Plasma).

If Windows SmartScreen / Defender complains about the unsigned file, that is expected for a
home-built plug-in.

## Features (v0.1)

* 2 wavetable oscillators (8 procedural wavetables x 64 morphable frames, band-limited mip-maps),
  each with 11 warp modes, up to 8-voice unison (detune, blend, stereo), phase & random phase
* Sub oscillator (4 shapes) + noise
* Multimode filter: LP12/LP24/HP12/HP24/BP/Notch, nonlinear ZDF Ladder, tuned Comb; drive and key tracking
* 3 ADSR envelopes, 4 LFOs (7 shapes, tempo sync, fade-in, retrigger), 4 macros
* 10-slot mod matrix: sources (Env2/3, LFO1-4, Velocity, Key, Mod wheel, Aftertouch, Macros, Random)
  to 24 destinations
* FX: distortion (4 types), chorus, ping-pong/tempo-synced delay, reverb
* Poly (16 voices) / mono / legato / glide, pitch bend, mod wheel, sustain pedal
* 10 factory presets, save/load `.plasma` presets

## Building locally

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Plasma_VST3
```

Headless DSP tests (no GUI toolkit needed):

```
cmake -S . -B build-core -DPLASMA_BUILD_PLUGIN=OFF && cmake --build build-core && ./build-core/plasma_tests
```

## Layout

* `core/plasma/` - framework-independent C++17 DSP engine (header only)
* `plugin/` - JUCE wrapper (parameters, MIDI, editor)
* `tests/` - headless engine tests
* `.github/workflows/build.yml` - CI that builds the Windows VST3

## Licensing note

The plug-in links JUCE (AGPLv3 / commercial). That is fine for personal use. Before distributing
binaries, either publish the source under AGPL or switch to a permissive framework / JUCE licence;
the DSP core is framework-independent so that is a small change.
