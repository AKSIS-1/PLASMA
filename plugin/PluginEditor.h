#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

namespace ui {
// Spectrum palette: near-black ground, each module keyed to a gas-discharge emission colour.
const juce::Colour bg{0xff0a0c10}, panel{0xff10131a}, line{0xff1f2430}, line2{0xff2c3342}, track{0xff252b38},
    text{0xffe6e8ee}, dim{0xff7d8596}, faint{0xff586074}, orange{0xffff6a3d}, violet{0xffa58bff}, ice{0xff8fd3ff},
    yellow{0xffffd86b};
constexpr int kBaseW = 1280, kBaseH = 900;
}  // namespace ui

class SpectrumLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  SpectrumLookAndFeel();
  void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle,
                        juce::Slider&) override;
  void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                        juce::Slider::SliderStyle, juce::Slider&) override;
  void drawLinearSliderBackground(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle,
                                  juce::Slider&) override {}
  void drawLinearSliderThumb(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle,
                             juce::Slider&) override {}
  void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
  void drawComboBox(juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
  void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
  void drawButtonText(juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
  void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
  juce::Font getComboBoxFont(juce::ComboBox&) override;
  juce::Font getPopupMenuFont() override;
  juce::Font getTextButtonFont(juce::TextButton&, int) override;
  juce::Font getAlertWindowMessageFont() override;
  juce::Label* createSliderTextBox(juce::Slider&) override;
  juce::Rectangle<int> getTooltipBounds(const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
  void drawTooltip(juce::Graphics&, const juce::String&, int w, int h) override;
};

class PlasmaEditor : public juce::AudioProcessorEditor, private juce::Timer {
 public:
  explicit PlasmaEditor(PlasmaProcessor&);
  ~PlasmaEditor() override;
  void paint(juce::Graphics&) override;
  void resized() override;

 private:
  void timerCallback() override;
  void setTipsEnabled(bool);

  PlasmaProcessor& proc_;
  SpectrumLookAndFeel lnf_;
  struct Root;
  std::unique_ptr<Root> root_;
  juce::TooltipWindow tooltips_{this, 450};
  std::unique_ptr<juce::ComponentBoundsConstrainer> constrainer_;
};
