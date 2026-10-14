#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "plasma/Presets.h"
#include "plasma/Synth.h"

class PlasmaProcessor : public juce::AudioProcessor {
 public:
  PlasmaProcessor();
  ~PlasmaProcessor() override = default;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return "Plasma"; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 4.0; }

  int getNumPrograms() override { return (int)plasma::factoryPresets().size(); }
  int getCurrentProgram() override { return currentProgram_; }
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState apvts;
  int activeVoices() const { return voices_.load(); }

 private:
  static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

  plasma::Synth synth_;
  std::atomic<float>* raw_[plasma::kNumParams] = {};
  float values_[plasma::kNumParams] = {};
  int currentProgram_ = 0;
  std::atomic<int> voices_{0};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlasmaProcessor)
};
