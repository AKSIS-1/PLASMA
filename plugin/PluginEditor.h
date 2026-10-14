#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

namespace ui {
const juce::Colour bg{0xff0d0a14}, panel{0xff181226}, panelHi{0xff231a38}, text{0xffe9e4f5}, dim{0xff8b819f},
    magenta{0xffff2bd6}, cyan{0xff29d9ff}, violet{0xff8a5bff};
}

class PlasmaLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  PlasmaLookAndFeel();
  void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle,
                        juce::Slider&) override;
  void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                        juce::Slider::SliderStyle, juce::Slider&) override;
  void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
  void drawComboBox(juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
  void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
  juce::Font getComboBoxFont(juce::ComboBox&) override;
  juce::Font getTextButtonFont(juce::TextButton&, int) override;
  juce::Label* createSliderTextBox(juce::Slider&) override;
};

class PlasmaEditor : public juce::AudioProcessorEditor, private juce::Timer {
 public:
  explicit PlasmaEditor(PlasmaProcessor&);
  ~PlasmaEditor() override;
  void paint(juce::Graphics&) override;
  void resized() override;

 private:
  void timerCallback() override;
  void showPage(int);
  void savePreset();
  void loadPreset();

  PlasmaProcessor& proc_;
  PlasmaLookAndFeel lnf_;
  struct Pages;
  std::unique_ptr<Pages> pages_;
  juce::TextButton tabs_[3];
  juce::TextButton saveBtn_{"Save"}, loadBtn_{"Load"}, prevBtn_{"<"}, nextBtn_{">"};
  juce::ComboBox presetBox_;
  juce::Label voices_;
  std::unique_ptr<juce::FileChooser> chooser_;
  int page_ = 0;
};
