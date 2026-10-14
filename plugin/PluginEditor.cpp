#include "PluginEditor.h"

#include <BinaryData.h>

#include <complex>
#include <cstring>

#include "Tooltips.h"

using namespace plasma;
using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

namespace {

const juce::Colour kKrypton{0xff8fe0b0};    // modulation
const juce::Colour kHydrogen{0xffff7a90};   // effects
const juce::Colour kNeutral{0xffc3c8d4};    // utility / output

// ===================================================================== fonts
struct Fonts {
  juce::Typeface::Ptr label, labelMed, mono, monoMed, round;
  static Fonts& get() {
    static Fonts f;
    return f;
  }
  Fonts() {
    label = juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowSemiCondensedSemiBold_ttf, BinaryData::BarlowSemiCondensedSemiBold_ttfSize);
    labelMed = juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowSemiCondensedMedium_ttf, BinaryData::BarlowSemiCondensedMedium_ttfSize);
    mono = juce::Typeface::createSystemTypefaceFor(BinaryData::IBMPlexMonoRegular_ttf, BinaryData::IBMPlexMonoRegular_ttfSize);
    monoMed = juce::Typeface::createSystemTypefaceFor(BinaryData::IBMPlexMonoMedium_ttf, BinaryData::IBMPlexMonoMedium_ttfSize);
    round = juce::Typeface::createSystemTypefaceFor(BinaryData::VarelaRoundRegular_ttf, BinaryData::VarelaRoundRegular_ttfSize);
  }
  static juce::Font make(const juce::Typeface::Ptr& t, float h, float kern = 0.f) {
    return juce::Font(juce::FontOptions(t).withHeight(h).withKerningFactor(kern));
  }
};
juce::Font labelFont(float h = 10.5f) { return Fonts::make(Fonts::get().label, h, 0.12f); }
juce::Font monoFont(float h = 10.5f) { return Fonts::make(Fonts::get().mono, h); }
juce::Font monoMedFont(float h = 10.5f) { return Fonts::make(Fonts::get().monoMed, h); }

// ===================================================================== value text
bool isTimeParam(const juce::String& id) {
  return (id.startsWith("env") && (id.endsWith("_a") || id.endsWith("_d") || id.endsWith("_r"))) || id == "glide" ||
         id.endsWith("_fade");
}

// Long form (used when typing a value)
juce::String formatValue(const ParamDef& d, double v) {
  const juce::String id(d.id);
  if (d.type == ParamType::Int) return juce::String((int)std::lround(v));
  if (id.endsWith("_cutoff") || (id.endsWith("_damp") && id.startsWith("dly")))
    return v >= 1000 ? juce::String(v / 1000.0, 2) + " kHz" : juce::String((int)std::lround(v)) + " Hz";
  if (isTimeParam(id)) return v < 1.0 ? juce::String((int)std::lround(v * 1000)) + " ms" : juce::String(v, 2) + " s";
  if (id.endsWith("_rate")) return juce::String(v, 2) + " Hz";
  if (id == "dly_time") return juce::String((int)std::lround(v)) + " ms";
  if (id.endsWith("_fine")) return juce::String(v, 0) + " ct";
  if (d.min < 0.f) return juce::String(v, 2);
  if (d.max <= 1.f) return juce::String((int)std::lround(v * 100)) + "%";
  return juce::String(v, 2);
}

// Short form (shown inside the rings)
juce::String shortText(const ParamDef& d, double v) {
  const juce::String id(d.id);
  if (d.type == ParamType::Int) {
    const int i = (int)std::lround(v);
    return (i > 0 && d.min < 0) ? "+" + juce::String(i) : juce::String(i);
  }
  if (id.endsWith("_cutoff") || (id.endsWith("_damp") && id.startsWith("dly")))
    return v >= 1000 ? juce::String(v / 1000.0, v >= 10000 ? 0 : 1) + "k" : juce::String((int)std::lround(v));
  if (isTimeParam(id)) return v < 1.0 ? juce::String((int)std::lround(v * 1000)) + "ms" : juce::String(v, v >= 10 ? 0 : 1) + "s";
  if (id.endsWith("_rate")) return juce::String(v, v >= 10 ? 1 : 2);
  if (id == "dly_time") return juce::String((int)std::lround(v)) + "ms";
  if (id.endsWith("_fine")) return (v > 0.5 ? "+" : "") + juce::String((int)std::lround(v)) + "ct";
  if (id.endsWith("_pan")) {
    if (std::abs(v) < 0.005) return "C";
    return juce::String(v < 0 ? "L" : "R") + juce::String((int)std::lround(std::abs(v) * 100));
  }
  if (d.min < 0.f) return (v > 0.004 ? "+" : "") + juce::String((int)std::lround(v * 100)) + "%";
  if (d.max <= 1.f) return juce::String((int)std::lround(v * 100)) + "%";
  return juce::String(v, 2);
}

// Parses what a person types into the "Type value" box
float parseValue(const ParamDef& d, const juce::String& textIn) {
  const juce::String id(d.id);
  const juce::String t = textIn.trim().toLowerCase();
  double v = t.getDoubleValue();
  if (t.contains("khz") || (t.endsWith("k") && !t.contains("ms"))) v *= 1000.0;
  else if (t.contains("ms") && id != "dly_time") v /= 1000.0;
  else if (t.contains("%")) v /= 100.0;
  else if (d.max <= 1.0f && d.type == ParamType::Float && std::abs(v) > 1.0) v /= 100.0;
  return (float)v;
}

void setReal(juce::AudioProcessorValueTreeState& apvts, const ParamDef& d, float v) {
  if (auto* p = apvts.getParameter(d.id)) p->setValueNotifyingHost(d.toNorm(std::min(d.maxValue(), std::max(d.min, v))));
}
void gesture(juce::AudioProcessorValueTreeState& apvts, const char* id, bool begin) {
  if (auto* p = apvts.getParameter(id)) { if (begin) p->beginChangeGesture(); else p->endChangeGesture(); }
}
float realOf(juce::AudioProcessorValueTreeState& apvts, const char* id) { return apvts.getRawParameterValue(id)->load(); }

}  // namespace

// ===================================================================== look & feel
SpectrumLookAndFeel::SpectrumLookAndFeel() {
  setColour(juce::ResizableWindow::backgroundColourId, ui::bg);
  setColour(juce::Label::textColourId, ui::text);
  setColour(juce::Slider::textBoxTextColourId, ui::dim);
  setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
  setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
  setColour(juce::ComboBox::textColourId, ui::text);
  setColour(juce::PopupMenu::backgroundColourId, ui::panel);
  setColour(juce::PopupMenu::textColourId, ui::text);
  setColour(juce::PopupMenu::highlightedBackgroundColourId, ui::line2);
  setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
  setColour(juce::AlertWindow::backgroundColourId, ui::panel);
  setColour(juce::AlertWindow::textColourId, ui::text);
  setColour(juce::AlertWindow::outlineColourId, ui::line2);
  setColour(juce::TextEditor::backgroundColourId, ui::bg);
  setColour(juce::TextEditor::textColourId, ui::text);
  setColour(juce::TextEditor::outlineColourId, ui::line2);
  setColour(juce::TextEditor::focusedOutlineColourId, ui::orange);
  setColour(juce::TextButton::buttonColourId, ui::line);
  setColour(juce::TextButton::textColourOffId, ui::text);
  setColour(juce::TextButton::textColourOnId, ui::text);
}

juce::Font SpectrumLookAndFeel::getComboBoxFont(juce::ComboBox&) { return monoFont(11.f); }
juce::Font SpectrumLookAndFeel::getPopupMenuFont() { return monoFont(12.f); }
juce::Font SpectrumLookAndFeel::getTextButtonFont(juce::TextButton&, int) { return labelFont(12.f); }
juce::Font SpectrumLookAndFeel::getAlertWindowMessageFont() { return monoFont(12.f); }

juce::Label* SpectrumLookAndFeel::createSliderTextBox(juce::Slider& s) {
  auto* l = LookAndFeel_V4::createSliderTextBox(s);
  l->setFont(monoFont(11.f));
  l->setJustificationType(juce::Justification::centredRight);
  return l;
}

void SpectrumLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label) {
  label.setBounds(7, 1, box.getWidth() - 24, box.getHeight() - 2);
  label.setFont(getComboBoxFont(box));
  label.setJustificationType(juce::Justification::centredLeft);
}

void SpectrumLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float, float a0, float a1,
                                           juce::Slider& s) {
  const auto b = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h);
  const auto c = b.getCentre();
  const float r = std::min(b.getWidth(), b.getHeight()) * 0.5f - 3.5f;
  const float pos = (float)s.valueToProportionOfLength(s.getValue());
  const float ang = a0 + pos * (a1 - a0);
  const juce::Colour col = s.findColour(juce::Slider::rotarySliderFillColourId);
  const bool bip = s.getMinimum() < 0.0 && s.getMaximum() > 0.0 && std::abs(s.getMinimum() + s.getMaximum()) < 1e-6;
  const float zero = bip ? a0 + 0.5f * (a1 - a0) : a0;
  const bool hot = s.isMouseOverOrDragging();

  juce::Path trk;
  trk.addCentredArc(c.x, c.y, r, r, 0.f, a0, a1, true);
  g.setColour(hot ? ui::track.brighter(0.25f) : ui::track);
  g.strokePath(trk, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

  if (std::abs(ang - zero) > 0.02f) {
    juce::Path val;
    val.addCentredArc(c.x, c.y, r, r, 0.f, std::min(zero, ang), std::max(zero, ang), true);
    g.setColour(col);
    g.strokePath(val, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  }
  const float dx = c.x + r * std::sin(ang), dy = c.y - r * std::cos(ang);
  g.setColour(col);
  g.fillEllipse(dx - 3.2f, dy - 3.2f, 6.4f, 6.4f);

  const int pid = (int)s.getProperties().getWithDefault("pid", 0);
  const auto txt = shortText(paramDefs()[(size_t)pid], s.getValue());
  g.setColour(hot ? juce::Colours::white : ui::text);
  g.setFont(monoFont(txt.length() > 5 ? 9.f : 10.5f));
  g.drawText(txt, juce::Rectangle<float>(c.x - r + 3.f, c.y - 8.f, 2.f * r - 6.f, 16.f), juce::Justification::centred, false);
}

void SpectrumLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                           juce::Slider::SliderStyle style, juce::Slider& s) {
  const juce::Colour col = s.findColour(juce::Slider::trackColourId);
  if (style == juce::Slider::LinearBar) {  // draggable "A 300ms" field
    const auto r = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(0.5f);
    g.setColour(juce::Colour(0xff0d1016));
    g.fillRect(r);
    g.setColour(s.isMouseOverOrDragging() ? ui::line2.brighter(0.3f) : ui::line2);
    g.drawRect(r, 1.f);
    const int pid = (int)s.getProperties().getWithDefault("pid", 0);
    const juce::String cap = s.getProperties().getWithDefault("cap", "").toString();
    g.setColour(ui::dim);
    g.setFont(monoFont(10.f));
    g.drawText(cap, r.reduced(6.f, 0.f), juce::Justification::centredLeft);
    g.setColour(ui::text);
    g.setFont(monoMedFont(10.f));
    g.drawText(shortText(paramDefs()[(size_t)pid], s.getValue()), r.reduced(6.f, 0.f), juce::Justification::centredRight);
    return;
  }
  const float cy = y + h * 0.5f;
  const bool scrub = s.getProperties().contains("scrub");
  g.setColour(ui::track);
  g.fillRect((float)x, cy - 1.f, (float)w, 2.f);
  const bool bip = s.getMinimum() < 0.0;
  const float zeroX = bip ? x + (float)s.valueToProportionOfLength(0.0) * w : (float)x;
  g.setColour(col);
  g.fillRect(std::min(zeroX, pos), cy - (scrub ? 1.f : 2.f), std::abs(pos - zeroX), scrub ? 2.f : 4.f);
  if (bip) { g.setColour(ui::faint); g.fillRect(zeroX - 1.f, cy - 5.f, 2.f, 10.f); }
  g.setColour(scrub ? col : ui::text);
  g.fillRect(pos - 1.5f, cy - 5.f, 3.f, 10.f);
}

void SpectrumLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool hi, bool) {
  const auto r = b.getLocalBounds().toFloat().reduced(0.5f);
  const bool on = b.getToggleState();
  const juce::Colour col = b.findColour(juce::ToggleButton::tickColourId);
  if (b.getProperties().contains("chip")) {  // bordered block (effects chain, Tips)
    g.setColour(on ? juce::Colour(0xff10161d) : juce::Colours::transparentBlack);
    g.fillRect(r);
    g.setColour(on ? col : (hi ? ui::faint : ui::track));
    g.drawRect(r, 1.f);
    g.setColour(on ? ui::text : ui::faint);
    g.setFont(monoFont(10.5f));
    g.drawText(b.getButtonText(), r, juce::Justification::centred);
    return;
  }
  g.setColour(on ? col : (hi ? ui::faint : ui::line2));
  g.drawRect(r, 1.f);
  const float bx = r.getX() + 8.f, by = r.getCentreY() - 3.f;
  g.setColour(on ? col : ui::track);
  g.fillRect(bx, by, 6.f, 6.f);
  g.setColour(on ? ui::text : ui::dim);
  g.setFont(labelFont(11.f));
  g.drawText(b.getButtonText().toUpperCase(), r.withTrimmedLeft(21.f), juce::Justification::centredLeft);
}

void SpectrumLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) {
  const auto r = juce::Rectangle<float>(0.f, 0.f, (float)w, (float)h).reduced(0.5f);
  g.setColour(juce::Colour(0xff0d1016));
  g.fillRect(r);
  g.setColour(box.isMouseOver() || box.isPopupActive() ? ui::faint : ui::line2);
  g.drawRect(r, 1.f);
  juce::Path arrow;
  arrow.addTriangle((float)w - 15.f, h * 0.5f - 2.f, (float)w - 8.f, h * 0.5f - 2.f, (float)w - 11.5f, h * 0.5f + 2.5f);
  g.setColour(ui::dim);
  g.fillPath(arrow);
}

void SpectrumLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool, bool) {
  if (b.getProperties().getWithDefault("kind", "") == "tab" && b.getToggleState()) {
    g.setColour(ui::orange);
    g.fillRect(2.f, (float)b.getHeight() - 2.f, (float)b.getWidth() - 4.f, 2.f);
  }
}

void SpectrumLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool hi, bool down) {
  const juce::String kind = b.getProperties().getWithDefault("kind", "").toString();
  const bool on = b.getToggleState();
  if (kind == "tab") {
    g.setFont(Fonts::make(Fonts::get().label, 12.5f, 0.2f));
    g.setColour(on ? ui::text : (hi ? ui::text.withAlpha(0.8f) : ui::dim));
  } else if (kind == "arrow") {
    g.setFont(monoFont(16.f));
    g.setColour(hi ? ui::text : ui::dim);
  } else {
    g.setFont(Fonts::make(Fonts::get().label, 11.5f, 0.16f));
    g.setColour(down ? ui::text : (hi ? ui::text : ui::dim));
  }
  g.drawText(kind == "tab" ? b.getButtonText() : b.getButtonText(), b.getLocalBounds().withTrimmedBottom(kind == "tab" ? 4 : 0),
             juce::Justification::centred);
}

juce::Rectangle<int> SpectrumLookAndFeel::getTooltipBounds(const juce::String& tip, juce::Point<int> pos, juce::Rectangle<int> area) {
  juce::AttributedString as;
  as.setJustification(juce::Justification::topLeft);
  as.append(tip, Fonts::make(Fonts::get().labelMed, 13.5f), ui::text);
  juce::TextLayout tl;
  tl.createLayoutWithBalancedLineLengths(as, 320.f);
  const int w = (int)std::ceil(tl.getWidth()) + 22, h = (int)std::ceil(tl.getHeight()) + 18;
  return juce::Rectangle<int>(pos.x > area.getCentreX() ? pos.x - w - 12 : pos.x + 14, pos.y + 18, w, h).constrainedWithin(area.reduced(4));
}

void SpectrumLookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& tip, int w, int h) {
  const auto r = juce::Rectangle<float>(0.f, 0.f, (float)w, (float)h).reduced(0.5f);
  g.setColour(juce::Colour(0xff0f1218));
  g.fillRect(r);
  g.setColour(ui::orange.withAlpha(0.8f));
  g.drawRect(r, 1.f);
  juce::AttributedString as;
  as.setJustification(juce::Justification::topLeft);
  as.append(tip, Fonts::make(Fonts::get().labelMed, 13.5f), ui::text);
  juce::TextLayout tl;
  tl.createLayoutWithBalancedLineLengths(as, 320.f);
  tl.draw(g, juce::Rectangle<float>(11.f, 9.f, (float)w - 22.f, (float)h - 18.f));
}

// ===================================================================== right-click menu
// The host's own parameter menu (FL Studio: "Create automation clip", "Link to controller", ...)
// followed by Reset and, for numeric parameters, "Type value...".
namespace {
struct ParamContextMenu : juce::MouseListener {
  ParamContextMenu(juce::AudioProcessorValueTreeState& a, const juce::String& id, juce::Component& c)
      : apvts(a), paramId(id), comp(c) {
    comp.addMouseListener(this, true);
  }
  ~ParamContextMenu() override { comp.removeMouseListener(this); }

  void mouseDown(const juce::MouseEvent& e) override {
    if (!e.mods.isPopupMenu()) return;
    auto* param = apvts.getParameter(paramId);
    if (param == nullptr) return;
    auto* editor = comp.findParentComponentOfClass<juce::AudioProcessorEditor>();
    const int idx = findParam(paramId.toStdString());
    const ParamDef def = paramDefs()[(size_t)idx];

    std::shared_ptr<juce::HostProvidedContextMenu> hostMenu;
    juce::PopupMenu menu;
    if (editor != nullptr)
      if (auto* ctx = editor->getHostContext())
        if (auto m = ctx->getContextMenuForParameter(param)) {
          hostMenu = std::move(m);
          menu = hostMenu->getEquivalentPopupMenu();
        }
    if (menu.getNumItems() > 0) menu.addSeparator();
    menu.addItem("Reset to default", [param] {
      param->beginChangeGesture();
      param->setValueNotifyingHost(param->getDefaultValue());
      param->endChangeGesture();
    });
    if (def.type == ParamType::Float || def.type == ParamType::Int) {
      auto& av = apvts;
      menu.addItem("Type value...", [editor, def, param, &av] {
        auto* w = new juce::AlertWindow("Enter value", juce::String(def.name), juce::MessageBoxIconType::NoIcon, editor);
        w->addTextEditor("v", formatValue(def, realOf(av, def.id)));
        w->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
        w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        w->enterModalState(true, juce::ModalCallbackFunction::create([w, def, param](int r) {
          if (r == 1) {
            param->beginChangeGesture();
            param->setValueNotifyingHost(def.toNorm(std::min(def.maxValue(), std::max(def.min, parseValue(def, w->getTextEditorContents("v"))))));
            param->endChangeGesture();
          }
        }), true);
      });
    }
    menu.setLookAndFeel(&comp.getLookAndFeel());
    auto opts = juce::PopupMenu::Options()
                    .withTargetScreenArea(juce::Rectangle<int>(e.getScreenPosition(), e.getScreenPosition()).expanded(1))
                    .withParentComponent(editor);
    menu.showMenuAsync(opts, [hostMenu](int) {});  // hostMenu stays alive while the menu is open
  }

  juce::AudioProcessorValueTreeState& apvts;
  juce::String paramId;
  juce::Component& comp;
};

// ===================================================================== basic widgets
struct RingKnob : juce::Component, juce::SettableTooltipClient {
  RingKnob(juce::AudioProcessorValueTreeState& apvts, P id, const juce::String& caption, juce::Colour colour) : caption_(caption) {
    const ParamDef& def = paramDefs()[(size_t)id];
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setColour(juce::Slider::rotarySliderFillColourId, colour);
    slider.getProperties().set("pid", (int)id);
    slider.setMouseDragSensitivity(220);
    addAndMakeVisible(slider);
    att = std::make_unique<SliderAtt>(apvts, def.id, slider);
    slider.setDoubleClickReturnValue(true, def.def);
    const auto tip = tips::forParam(def.id);
    slider.setTooltip(tip);
    setTooltip(tip);
    menu = std::make_unique<ParamContextMenu>(apvts, def.id, *this);
  }
  void resized() override { slider.setBounds((getWidth() - 52) / 2, 0, 52, 52); }
  void paint(juce::Graphics& g) override {
    g.setColour(ui::dim);
    g.setFont(labelFont(10.5f));
    g.drawText(caption_.toUpperCase(), 0, 54, getWidth(), 14, juce::Justification::centred);
  }
  juce::Slider slider;
  std::unique_ptr<SliderAtt> att;
  std::unique_ptr<ParamContextMenu> menu;
  juce::String caption_;
};

struct Combo : juce::ComboBox {
  Combo(juce::AudioProcessorValueTreeState& apvts, P id) {
    const ParamDef& def = paramDefs()[(size_t)id];
    int i = 1;
    for (const auto& s : def.choiceList()) addItem(s, i++);
    att = std::make_unique<ComboAtt>(apvts, def.id, *this);
    setTooltip(tips::forParam(def.id));
    menu = std::make_unique<ParamContextMenu>(apvts, def.id, *this);
  }
  std::unique_ptr<ComboAtt> att;
  std::unique_ptr<ParamContextMenu> menu;
};

struct Toggle : juce::ToggleButton {
  Toggle(juce::AudioProcessorValueTreeState& apvts, P id, const juce::String& text, juce::Colour colour, bool chip = false) {
    setButtonText(text);
    setColour(juce::ToggleButton::tickColourId, colour);
    if (chip) getProperties().set("chip", true);
    const char* pid = paramDefs()[(size_t)id].id;
    att = std::make_unique<ButtonAtt>(apvts, pid, *this);
    setTooltip(tips::forParam(pid));
    menu = std::make_unique<ParamContextMenu>(apvts, pid, *this);
  }
  std::unique_ptr<ButtonAtt> att;
  std::unique_ptr<ParamContextMenu> menu;
};

// A draggable "A 300ms" field
struct ValueField : juce::Slider {
  ValueField(juce::AudioProcessorValueTreeState& apvts, P id, const juce::String& cap) {
    const ParamDef& def = paramDefs()[(size_t)id];
    setSliderStyle(juce::Slider::LinearBar);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    getProperties().set("pid", (int)id);
    getProperties().set("cap", cap);
    setMouseDragSensitivity(240);
    att = std::make_unique<SliderAtt>(apvts, def.id, *this);
    setDoubleClickReturnValue(true, def.def);
    setTooltip(tips::forParam(def.id));
    menu = std::make_unique<ParamContextMenu>(apvts, def.id, *this);
  }
  std::unique_ptr<SliderAtt> att;
  std::unique_ptr<ParamContextMenu> menu;
};

// ===================================================================== displays
juce::Colour ridgeGrey(float t) { return juce::Colour::fromFloatRGBA(0.59f, 0.65f, 0.75f, 0.22f + 0.40f * t); }

struct RidgeView : juce::Component, juce::Timer {
  RidgeView(juce::AudioProcessorValueTreeState& a, P tbl, P pos, juce::Colour c) : apvts(a), pt(tbl), pp(pos), col(c) { startTimerHz(30); }
  void timerCallback() override {
    const float t = realOf(apvts, paramDefs()[(size_t)pt].id), p = realOf(apvts, paramDefs()[(size_t)pp].id);
    if (t != lt || p != lp) { lt = t; lp = p; repaint(); }
  }
  void paint(juce::Graphics& g) override {
    const auto bounds = getLocalBounds().toFloat();
    g.setColour(ui::bg);
    g.fillRect(bounds);
    g.setColour(juce::Colour(0xff1a1f2a));
    g.drawRect(bounds, 1.f);
    const auto& wt = WavetableBank::get().table((int)lt);
    const float W = bounds.getWidth() - 8.f, H = bounds.getHeight();
    const int N = 22, M = 94;
    const float top = H * 0.30f, step = (H * 0.62f) / (float)(N - 1), amp = H * 0.085f;
    const int sel = (int)std::lround(lp * (N - 1));
    for (int k = 0; k < N; ++k) {
      const float t = (float)k / (float)(N - 1), base = top + step * (float)k;
      juce::Path ridge;
      for (int i = 0; i <= M; ++i) {
        const float ph = (float)i / (float)M;
        float v;
        if (k == sel) v = wt.lookup(0, lp * (float)(Wavetable::kFrames - 1), std::min(ph, 0.9999f));
        else v = wt.frame((int)std::lround(t * (Wavetable::kFrames - 1)))[std::min(Wavetable::kFrameLen - 1, (int)(ph * (float)Wavetable::kFrameLen))];
        const float x = 4.f + ph * W, y = base - v * amp;
        if (i == 0) ridge.startNewSubPath(x, y); else ridge.lineTo(x, y);
      }
      juce::Path fill(ridge);
      fill.lineTo(4.f + W, H - 1.f);
      fill.lineTo(4.f, H - 1.f);
      fill.closeSubPath();
      g.setColour(ui::bg);
      g.fillPath(fill);
      if (k == sel) {
        g.setColour(col.withAlpha(0.22f));
        g.strokePath(ridge, juce::PathStrokeType(4.f, juce::PathStrokeType::curved));
        g.setColour(col);
        g.strokePath(ridge, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved));
      } else {
        g.setColour(ridgeGrey(t));
        g.strokePath(ridge, juce::PathStrokeType(1.f, juce::PathStrokeType::curved));
      }
    }
    g.setColour(ui::faint);
    g.setFont(monoFont(9.5f));
    g.drawText(juce::String(wt.name).toUpperCase(), bounds.reduced(8.f, 6.f), juce::Justification::topRight);
  }
  juce::AudioProcessorValueTreeState& apvts;
  P pt, pp;
  juce::Colour col;
  float lt = -1.f, lp = -1.f;
};

struct FrameBar : juce::Component {
  FrameBar(juce::AudioProcessorValueTreeState& apvts, P pos, juce::Colour c) {
    const ParamDef& def = paramDefs()[(size_t)pos];
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour(juce::Slider::trackColourId, c);
    slider.getProperties().set("scrub", true);
    addAndMakeVisible(slider);
    att = std::make_unique<SliderAtt>(apvts, def.id, slider);
    slider.setDoubleClickReturnValue(true, def.def);
    slider.setTooltip("Drag to scan through the wavetable frames. Same control as the Pos knob.");
  }
  void resized() override { slider.setBounds(54, 0, getWidth() - 54 - 64, getHeight()); }
  void paint(juce::Graphics& g) override {
    g.setColour(ui::dim);
    g.setFont(monoFont(10.f));
    g.drawText("FRAME", 4, 0, 50, getHeight(), juce::Justification::centredLeft);
    g.setColour(ui::text);
    g.drawText(juce::String(1 + (int)std::lround(slider.getValue() * (Wavetable::kFrames - 1))) + " / " + juce::String(Wavetable::kFrames),
               getWidth() - 62, 0, 62, getHeight(), juce::Justification::centredRight);
  }
  juce::Slider slider;
  std::unique_ptr<SliderAtt> att;
};

// Frequency response of the filter; drag to move cutoff (left/right) and resonance (up/down)
struct FilterView : juce::Component, juce::Timer {
  FilterView(juce::AudioProcessorValueTreeState& a, juce::Colour c) : apvts(a), col(c) { startTimerHz(30); }
  float readP(P id) { return realOf(apvts, paramDefs()[(size_t)id].id); }
  void timerCallback() override {
    const float t = readP(P::flt_type), c = readP(P::flt_cutoff), r = readP(P::flt_res), o = readP(P::flt_on);
    if (t != lt || c != lc || r != lr || o != lo) { lt = t; lc = c; lr = r; lo = o; repaint(); }
  }
  static double magnitude(int type, double f, double fc, double res) {
    const double x = f / fc, k = 2.0 * std::exp(-res * 4.60517);
    auto two = [&](double kk, int mode) {
      const double d = std::sqrt((1 - x * x) * (1 - x * x) + kk * kk * x * x);
      return mode == 0 ? 1.0 / d : mode == 1 ? x * x / d : mode == 2 ? kk * x / d : std::abs(1 - x * x) / d;
    };
    switch (type) {
      case 0: return two(k, 0);
      case 1: { const double m = two(k * 1.4, 0); return m * m; }
      case 2: return two(k, 1);
      case 3: { const double m = two(k * 1.4, 1); return m * m; }
      case 4: return two(k, 2);
      case 5: return two(k, 3);
      case 6: {  // 4-pole ladder with feedback
        const std::complex<double> s(0.0, x), one(1.0, 0.0);
        const auto den = std::pow(one + s, 4) + 3.9 * res;
        return std::abs(1.0 / den) * (1.0 + 0.4 * res);
      }
      default: {  // comb
        const double fb = res * 0.96;
        return 1.0 / std::sqrt(1 + fb * fb - 2 * fb * std::cos(2 * kPi * x));
      }
    }
  }
  void paint(juce::Graphics& g) override {
    const auto b = getLocalBounds().toFloat();
    const float W = b.getWidth(), H = b.getHeight();
    g.setColour(ui::bg);
    g.fillRect(b);
    g.setColour(juce::Colour(0xff1a1f2a));
    g.drawRect(b, 1.f);
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine((int)(H * i / 4.f), 1.f, W - 1.f);
    for (float fx : {100.f, 1000.f, 10000.f}) g.drawVerticalLine((int)(std::log10(fx / 20.f) / 3.f * W), 1.f, H - 1.f);
    g.setColour(ui::faint);
    g.setFont(monoFont(8.5f));
    g.drawText("100", (int)(std::log10(100.f / 20.f) / 3.f * W) + 3, (int)H - 14, 30, 12, juce::Justification::left);
    g.drawText("1k", (int)(std::log10(1000.f / 20.f) / 3.f * W) + 3, (int)H - 14, 30, 12, juce::Justification::left);
    g.drawText("10k", (int)(std::log10(10000.f / 20.f) / 3.f * W) + 3, (int)H - 14, 30, 12, juce::Justification::left);
    const bool on = lo > 0.5f;
    juce::Path line;
    const int type = (int)lt;
    for (int i = 0; i <= 150; ++i) {
      const double x = (double)i / 150.0, f = 20.0 * std::pow(1000.0, x);
      const double db = 20.0 * std::log10(std::max(magnitude(type, f, lc, lr), 1e-4));
      const float y = std::min(H - 3.f, std::max(3.f, H * 0.68f - (float)db * H * 0.019f));
      if (i == 0) line.startNewSubPath((float)x * W, y); else line.lineTo((float)x * W, y);
    }
    juce::Path area(line);
    area.lineTo(W, H);
    area.lineTo(0.f, H);
    area.closeSubPath();
    const auto c = on ? col : ui::faint;
    g.setGradientFill(juce::ColourGradient(c.withAlpha(0.32f), 0.f, 0.f, c.withAlpha(0.f), 0.f, H, false));
    g.fillPath(area);
    g.setColour(c);
    g.strokePath(line, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved));
    // cutoff marker
    const float cx = (float)(std::log10(lc / 20.f) / 3.0) * W;
    g.setColour(c.withAlpha(0.5f));
    g.drawVerticalLine((int)cx, 2.f, H - 2.f);
  }
  void drag(const juce::MouseEvent& e) {
    const float x = juce::jlimit(0.f, 1.f, e.position.x / (float)getWidth()), y = juce::jlimit(0.f, 1.f, e.position.y / (float)getHeight());
    setReal(apvts, paramDefs()[(size_t)P::flt_cutoff], 20.f * std::pow(1000.f, x));
    setReal(apvts, paramDefs()[(size_t)P::flt_res], 1.f - y);
  }
  void mouseDown(const juce::MouseEvent& e) override {
    if (e.mods.isPopupMenu()) return;
    gesture(apvts, "flt_cutoff", true);
    gesture(apvts, "flt_res", true);
    drag(e);
  }
  void mouseDrag(const juce::MouseEvent& e) override { if (!e.mods.isPopupMenu()) drag(e); }
  void mouseUp(const juce::MouseEvent& e) override {
    if (e.mods.isPopupMenu()) return;
    gesture(apvts, "flt_cutoff", false);
    gesture(apvts, "flt_res", false);
  }
  juce::AudioProcessorValueTreeState& apvts;
  juce::Colour col;
  float lt = -1, lc = -1, lr = -1, lo = -1;
};

// ADSR curve with draggable breakpoints
struct EnvView : juce::Component, juce::Timer, juce::SettableTooltipClient {
  EnvView(juce::AudioProcessorValueTreeState& a, P base, juce::Colour c) : apvts(a), b(base), col(c) {
    setTooltip("Drag the dots: the first sets Attack, the second sets Decay (left/right) and Sustain (up/down), the third sets Release.");
    startTimerHz(30);
  }
  float rd(int i) { return realOf(apvts, paramDefs()[(size_t)(b + i)].id); }
  void timerCallback() override {
    const float v[4] = {rd(0), rd(1), rd(2), rd(3)};
    if (std::memcmp(v, last, sizeof(v)) != 0) { std::memcpy(last, v, sizeof(v)); repaint(); }
  }
  static float tFrac(float t) { return std::log(std::max(t, 0.001f) / 0.001f) / std::log(10000.f); }
  static float fracT(float f) { return 0.001f * std::pow(10000.f, juce::jlimit(0.f, 1.f, f)); }
  static constexpr float kSeg = 84.f, kSus = 60.f;
  juce::Point<float> pts(int i) {
    const float H = (float)getHeight(), L = 10.f;
    const float a = rd(0), d = rd(1), s = rd(2), r = rd(3);
    const float x1 = L + 6.f + tFrac(a) * kSeg, x2 = x1 + 6.f + tFrac(d) * kSeg, x3 = x2 + kSus, x4 = x3 + 6.f + tFrac(r) * kSeg;
    const float top = 8.f, bot = H - 8.f, ys = bot - (bot - top) * s;
    if (i == 0) return {x1, top};
    if (i == 1) return {x2, ys};
    if (i == 2) return {x3, ys};
    return {x4, bot};
  }
  void paint(juce::Graphics& g) override {
    const auto bb = getLocalBounds().toFloat();
    const float H = bb.getHeight();
    g.setColour(ui::bg);
    g.fillRect(bb);
    g.setColour(juce::Colour(0xff1a1f2a));
    g.drawRect(bb, 1.f);
    const auto p1 = pts(0), p2 = pts(1), p3 = pts(2), p4 = pts(3);
    juce::Path line;
    line.startNewSubPath(10.f, H - 8.f);
    line.lineTo(p1);
    line.lineTo(p2);
    line.lineTo(p3);
    line.lineTo(p4);
    juce::Path area(line);
    area.closeSubPath();
    g.setColour(col.withAlpha(0.10f));
    g.fillPath(area);
    g.setColour(col);
    g.strokePath(line, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved));
    for (auto p : {p1, p2, p4}) {
      g.setColour(ui::bg);
      g.fillEllipse(p.x - 4.f, p.y - 4.f, 8.f, 8.f);
      g.setColour(col);
      g.drawEllipse(p.x - 4.f, p.y - 4.f, 8.f, 8.f, 1.6f);
    }
  }
  void mouseDown(const juce::MouseEvent& e) override {
    if (e.mods.isPopupMenu()) return;
    drag_ = -1;
    float best = 14.f;
    const int map[3] = {0, 1, 3};
    for (int i = 0; i < 3; ++i) {
      const float dist = pts(map[i]).getDistanceFrom(e.position);
      if (dist < best) { best = dist; drag_ = i; }
    }
    if (drag_ < 0) return;
    ids_[0] = paramDefs()[(size_t)b].id;
    ids_[1] = paramDefs()[(size_t)(b + 1)].id;
    ids_[2] = paramDefs()[(size_t)(b + 2)].id;
    ids_[3] = paramDefs()[(size_t)(b + 3)].id;
    if (drag_ == 0) gesture(apvts, ids_[0], true);
    if (drag_ == 1) { gesture(apvts, ids_[1], true); gesture(apvts, ids_[2], true); }
    if (drag_ == 2) gesture(apvts, ids_[3], true);
  }
  void mouseDrag(const juce::MouseEvent& e) override {
    if (drag_ < 0 || e.mods.isPopupMenu()) return;
    const float H = (float)getHeight(), L = 10.f;
    const auto& defs = paramDefs();
    const float x1 = pts(0).x, x2 = pts(1).x, x3 = pts(2).x;
    if (drag_ == 0) setReal(apvts, defs[(size_t)b], fracT((e.position.x - L - 6.f) / kSeg));
    if (drag_ == 1) {
      setReal(apvts, defs[(size_t)(b + 1)], fracT((e.position.x - x1 - 6.f) / kSeg));
      setReal(apvts, defs[(size_t)(b + 2)], juce::jlimit(0.f, 1.f, (H - 8.f - e.position.y) / (H - 16.f)));
    }
    if (drag_ == 2) setReal(apvts, defs[(size_t)(b + 3)], fracT((e.position.x - x3 - 6.f) / kSeg));
    juce::ignoreUnused(x2);
  }
  void mouseUp(const juce::MouseEvent& e) override {
    if (drag_ < 0 || e.mods.isPopupMenu()) return;
    if (drag_ == 0) gesture(apvts, ids_[0], false);
    if (drag_ == 1) { gesture(apvts, ids_[1], false); gesture(apvts, ids_[2], false); }
    if (drag_ == 2) gesture(apvts, ids_[3], false);
    drag_ = -1;
  }
  juce::AudioProcessorValueTreeState& apvts;
  P b;
  juce::Colour col;
  float last[4] = {-1, -1, -1, -1};
  int drag_ = -1;
  const char* ids_[4] = {};
};

struct LfoView : juce::Component, juce::Timer {
  LfoView(juce::AudioProcessorValueTreeState& a, P shape, juce::Colour c) : apvts(a), sp(shape), col(c) { startTimerHz(15); }
  void timerCallback() override {
    const float s = realOf(apvts, paramDefs()[(size_t)sp].id);
    if (s != last) { last = s; repaint(); }
  }
  void paint(juce::Graphics& g) override {
    const auto bb = getLocalBounds().toFloat();
    const float W = bb.getWidth(), H = bb.getHeight();
    g.setColour(ui::bg);
    g.fillRect(bb);
    g.setColour(juce::Colour(0xff1a1f2a));
    g.drawRect(bb, 1.f);
    g.drawHorizontalLine((int)(H * 0.5f), 1.f, W - 1.f);
    juce::Path p;
    const int shape = (int)last;
    const float hold[4] = {.6f, -.4f, .2f, -.8f};
    for (int i = 0; i <= 120; ++i) {
      const float ph = std::fmod((float)i / 120.f * 2.f, 1.f);
      float y;
      switch (shape) {
        case 0: y = std::sin(2.f * (float)kPi * ph); break;
        case 1: y = 4.f * std::fabs(ph - .5f) - 1.f; break;
        case 2: y = 2.f * ph - 1.f; break;
        case 3: y = 1.f - 2.f * ph; break;
        case 4: y = ph < .5f ? 1.f : -1.f; break;
        case 5: y = hold[std::min(3, (int)((float)i / 120.f * 4.f))]; break;
        default: y = 0.6f * std::sin(2.f * (float)kPi * ph) + 0.3f * std::sin(2.f * (float)kPi * ph * 2.2f); break;
      }
      const float x = 8.f + (float)i / 120.f * (W - 16.f), yy = H * 0.5f - y * (H * 0.36f);
      if (i == 0) p.startNewSubPath(x, yy); else p.lineTo(x, yy);
    }
    g.setColour(col);
    g.strokePath(p, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved));
  }
  juce::AudioProcessorValueTreeState& apvts;
  P sp;
  juce::Colour col;
  float last = -1;
};

// Animated organic logo mark: three intertwining filaments that pinch at both ends
struct LogoView : juce::Component, juce::Timer {
  LogoView() { startTimerHz(30); setInterceptsMouseClicks(false, false); }
  void timerCallback() override { t += 0.035f; repaint(); }
  void paint(juce::Graphics& g) override {
    const float W = (float)getWidth(), H = (float)getHeight(), mid = H * 0.5f;
    struct Strand { float amp, phase, speed, width, alpha; };
    const Strand strands[3] = {{0.46f, 0.f, 1.f, 3.0f, 0.95f}, {0.46f, 3.14159f, 1.f, 2.4f, 0.85f}, {0.30f, 1.57f, -1.3f, 1.7f, 0.75f}};
    for (const auto& s : strands) {
      juce::Path p;
      for (int i = 0; i <= 48; ++i) {
        const float u = (float)i / 48.f, env = std::pow(std::sin(u * 3.14159f), 0.75f);
        const float y = mid + H * s.amp * env * std::sin(u * 6.9f + s.phase + t * s.speed + 0.6f * std::sin(t * 0.6f + u * 3.f));
        const float x = 2.f + u * (W - 4.f);
        if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
      }
      juce::ColourGradient grad(ui::orange, 0.f, 0.f, ui::violet, W, 0.f, false);
      grad.addColour(0.35, ui::yellow);
      grad.addColour(0.68, ui::ice);
      grad.multiplyOpacity(s.alpha);
      g.setGradientFill(grad);
      g.strokePath(p, juce::PathStrokeType(s.width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
      juce::ColourGradient glow(grad);
      glow.multiplyOpacity(0.18f);
      g.setGradientFill(glow);
      g.strokePath(p, juce::PathStrokeType(s.width + 4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
  }
  float t = 0.f;
};

// ===================================================================== panels
constexpr int kKnobW = 74, kKnobH = 74;

struct Panel : juce::Component {
  Panel(PlasmaProcessor& p, const juce::String& t, juce::Colour c, bool header = true) : proc(p), title(t), colour(c), showHeader(header) {}
  RingKnob& knob(P id, const juce::String& cap) {
    knobs.push_back(std::make_unique<RingKnob>(proc.apvts, id, cap, colour));
    addAndMakeVisible(*knobs.back());
    return *knobs.back();
  }
  Combo& combo(P id, int w) {
    combos.push_back(std::make_unique<Combo>(proc.apvts, id));
    header.push_back({combos.back().get(), w});
    addAndMakeVisible(*combos.back());
    return *combos.back();
  }
  Toggle& toggle(P id, const juce::String& text, int w, bool chip = false) {
    toggles.push_back(std::make_unique<Toggle>(proc.apvts, id, text, colour, chip));
    header.push_back({toggles.back().get(), w});
    addAndMakeVisible(*toggles.back());
    return *toggles.back();
  }
  template <class T>
  T* block(std::unique_ptr<T> c, int h) {
    auto* raw = c.get();
    addAndMakeVisible(*raw);
    blocks.push_back({raw, h});
    owned.push_back(std::move(c));
    return raw;
  }
  void paint(juce::Graphics& g) override {
    const auto r = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(ui::panel);
    g.fillRect(r);
    g.setColour(ui::line);
    g.drawRect(r, 1.f);
    if (!showHeader) return;
    g.setColour(colour);
    g.fillRect(14.f, 15.f, 9.f, 9.f);
    g.setColour(ui::text);
    g.setFont(Fonts::make(Fonts::get().label, 12.5f, 0.2f));
    g.drawText(title, 30, 10, 200, 20, juce::Justification::centredLeft);
  }
  void resized() override {
    int y = 8;
    if (showHeader) {
      int total = 0;
      for (auto& h : header) total += h.second + 8;
      total = std::max(0, total - 8);
      const int avail = getWidth() - 24 - titleW;
      if (total <= avail) {
        int x = getWidth() - 12 - total;
        for (auto& h : header) { h.first->setBounds(x, 8, h.second, 24); x += h.second + 8; }
        y = 44;
      } else {
        int x = 12, row = 0;
        for (auto& h : header) {
          if (x + h.second > getWidth() - 12) { x = 12; ++row; }
          h.first->setBounds(x, 38 + row * 30, h.second, 24);
          x += h.second + 8;
        }
        y = 38 + (row + 1) * 30 + 6;
      }
    }
    for (auto& bl : blocks) { bl.first->setBounds(12, y, getWidth() - 24, bl.second); y += bl.second + 8; }
    const int cols = std::max(1, (getWidth() - 8) / kKnobW);
    const int n = (int)knobs.size();
    for (int i = 0; i < n; ++i) {
      int x = 6 + (i % cols) * kKnobW;
      if (spread && n <= cols && n > 1) x = 12 + i * ((getWidth() - 24 - kKnobW) / (n - 1));
      knobs[(size_t)i]->setBounds(x, y + (i / cols) * kKnobH, kKnobW, kKnobH);
    }
  }
  PlasmaProcessor& proc;
  juce::String title;
  juce::Colour colour;
  bool showHeader, spread = false;
  int titleW = 70;
  std::vector<std::unique_ptr<RingKnob>> knobs;
  std::vector<std::unique_ptr<Combo>> combos;
  std::vector<std::unique_ptr<Toggle>> toggles;
  std::vector<std::pair<juce::Component*, int>> header, blocks;
  std::vector<std::unique_ptr<juce::Component>> owned;
};

// rows of  [source] -> [destination] --- amount
struct MatrixRows : juce::Component {
  struct Row {
    std::unique_ptr<Combo> src, dst;
    juce::Slider amt;
    std::unique_ptr<SliderAtt> att;
    std::unique_ptr<ParamContextMenu> menu;
  };
  MatrixRows(PlasmaProcessor& p, int first, int count, int rowH, juce::Colour c) : rowH_(rowH) {
    for (int i = 0; i < count; ++i) {
      auto r = std::make_unique<Row>();
      const P b = P::slot1_src + (first + i) * (P::slot2_src - P::slot1_src);
      r->src = std::make_unique<Combo>(p.apvts, b);
      r->dst = std::make_unique<Combo>(p.apvts, b + 1);
      r->amt.setSliderStyle(juce::Slider::LinearHorizontal);
      r->amt.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
      r->amt.setColour(juce::Slider::trackColourId, c);
      r->amt.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
      r->amt.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
      r->amt.setColour(juce::Slider::textBoxTextColourId, ui::text);
      addAndMakeVisible(*r->src);
      addAndMakeVisible(*r->dst);
      addAndMakeVisible(r->amt);
      const char* amtId = paramDefs()[(size_t)(b + 2)].id;
      r->att = std::make_unique<SliderAtt>(p.apvts, amtId, r->amt);
      r->amt.textFromValueFunction = [](double v) { return (v > 0.004 ? juce::String("+") : juce::String()) + juce::String((int)std::lround(v * 100)) + "%"; };
      r->amt.valueFromTextFunction = [](const juce::String& t) { return t.getDoubleValue() / 100.0; };
      r->amt.setDoubleClickReturnValue(true, 0.0);
      r->amt.updateText();
      r->amt.setTooltip(tips::forParam(amtId));
      r->menu = std::make_unique<ParamContextMenu>(p.apvts, amtId, r->amt);
      rows.push_back(std::move(r));
    }
  }
  void resized() override {
    for (size_t i = 0; i < rows.size(); ++i) {
      const int y = (int)i * (rowH_ + 2);
      rows[i]->src->setBounds(0, y, 104, rowH_);
      rows[i]->dst->setBounds(128, y, 124, rowH_);
      rows[i]->amt.setBounds(266, y, getWidth() - 266, rowH_);
    }
  }
  void paint(juce::Graphics& g) override {
    g.setColour(ui::faint);
    g.setFont(monoFont(12.f));
    for (size_t i = 0; i < rows.size(); ++i) g.drawText(juce::CharPointer_UTF8("\xe2\x86\x92"), 104, (int)i * (rowH_ + 2), 24, rowH_, juce::Justification::centred);
  }
  int rowH_;
  std::vector<std::unique_ptr<Row>> rows;
};

struct EnvValues : juce::Component {
  EnvValues(PlasmaProcessor& p, P base) {
    const char* caps[4] = {"A", "D", "S", "R"};
    for (int i = 0; i < 4; ++i) {
      fields.push_back(std::make_unique<ValueField>(p.apvts, base + i, caps[i]));
      addAndMakeVisible(*fields.back());
    }
  }
  void resized() override {
    const int w = (getWidth() - 3 * 6) / 4;
    for (int i = 0; i < 4; ++i) fields[(size_t)i]->setBounds(i * (w + 6), 0, w, getHeight());
  }
  std::vector<std::unique_ptr<ValueField>> fields;
};

struct ChainBlock : juce::Component {
  ChainBlock(PlasmaProcessor& p) {
    const P ons[4] = {P::dist_on, P::cho_on, P::dly_on, P::rev_on};
    const char* names[4] = {"DIST", "CHORUS", "DELAY", "REVERB"};
    for (int i = 0; i < 4; ++i) {
      chips.push_back(std::make_unique<Toggle>(p.apvts, ons[i], names[i], ui::ice, true));
      addAndMakeVisible(*chips.back());
    }
  }
  void resized() override {
    const int w = 76, gap = 22;
    for (int i = 0; i < 4; ++i) chips[(size_t)i]->setBounds(i * (w + gap), 0, w, 30);
  }
  void paint(juce::Graphics& g) override {
    g.setColour(ui::faint);
    g.setFont(monoFont(14.f));
    for (int i = 0; i < 3; ++i) g.drawText(juce::CharPointer_UTF8("\xe2\x80\xba"), 76 + i * 98, 0, 22, 30, juce::Justification::centred);
    g.setFont(monoFont(10.f));
    g.drawText("Click a block to switch it on or off. Details are on the FX tab.", 0, 38, getWidth(), 28, juce::Justification::topLeft);
  }
  std::vector<std::unique_ptr<Toggle>> chips;
};

// Sources & output strip (no header): sub, noise, glide, bend, mono/legato, master, macros
struct StripPanel : Panel {
  StripPanel(PlasmaProcessor& p) : Panel(p, "", kNeutral, false) {
    sub = &toggle(P::sub_on, "Sub", 70);
    shape = &combo(P::sub_shape, 84);
    mono = &toggle(P::mono, "Mono", 76);
    legato = &toggle(P::legato, "Legato", 76);
    const std::pair<P, const char*> ks[] = {{P::sub_oct, "Sub oct"}, {P::sub_level, "Sub lvl"}, {P::noise_level, "Noise"}, {P::glide, "Glide"},
                                            {P::bend_range, "Bend"}, {P::master, "Master"}, {P::macro1, "Macro 1"}, {P::macro2, "Macro 2"},
                                            {P::macro3, "Macro 3"}, {P::macro4, "Macro 4"}};
    for (auto& k : ks) knob(k.first, k.second);
  }
  void resized() override {
    sub->setBounds(18, 14, 84, 26);
    shape->setBounds(18, 46, 84, 26);
    const int kx[] = {116, 192, 268, 380, 456, 668, 776, 852, 928, 1004};
    for (size_t i = 0; i < knobs.size(); ++i) knobs[i]->setBounds(kx[i], 8, kKnobW, kKnobH);
    mono->setBounds(548, 14, 84, 26);
    legato->setBounds(548, 46, 84, 26);
  }
  void paint(juce::Graphics& g) override {
    Panel::paint(g);
    g.setColour(ui::line);
    for (int x : {352, 644, 752}) g.fillRect(x, 14, 1, getHeight() - 28);
    g.setColour(ui::faint);
    g.setFont(labelFont(9.5f));
    g.drawText("SUB / NOISE", 18, 78, 110, 12, juce::Justification::left);
    g.drawText("MACROS", 776, getHeight() - 11, 100, 10, juce::Justification::left);
  }
  Toggle *sub, *mono, *legato;
  Combo* shape;
};

// ===================================================================== pages
struct Page : juce::Component {
  template <class T>
  T* place(std::unique_ptr<T> p, juce::Rectangle<int> r) {
    auto* raw = p.get();
    addAndMakeVisible(*raw);
    raw->setBounds(r);
    owned.push_back(std::move(p));
    return raw;
  }
  std::vector<std::unique_ptr<juce::Component>> owned;
};

std::unique_ptr<Panel> makeOsc(PlasmaProcessor& pr, const char* title, P base, juce::Colour c) {
  auto p = std::make_unique<Panel>(pr, title, c);
  p->titleW = 40;
  p->toggle(base + 0, "On", 56);
  p->combo(base + 1, 124);
  p->combo(base + 3, 100);
  p->block(std::make_unique<RidgeView>(pr.apvts, base + 1, base + 2, c), 188);
  p->block(std::make_unique<FrameBar>(pr.apvts, base + 2, c), 22);
  const std::pair<int, const char*> ks[] = {{2, "Pos"},    {4, "Warp"},  {5, "Level"},  {6, "Pan"},  {7, "Octave"}, {8, "Semi"}, {9, "Fine"},
                                            {10, "Unison"}, {11, "Detune"}, {12, "Blend"}, {13, "Stereo"}, {14, "Phase"}, {15, "Rand"}};
  for (auto& k : ks) p->knob(base + k.first, k.second);
  return p;
}

std::unique_ptr<Panel> makeEnv(PlasmaProcessor& pr, const char* title, P a, const char* role) {
  auto p = std::make_unique<Panel>(pr, title, ui::yellow);
  p->titleW = 70;
  p->block(std::make_unique<EnvView>(pr.apvts, a, ui::yellow), 58);
  p->block(std::make_unique<EnvValues>(pr, a), 22);
  juce::ignoreUnused(role);
  return p;
}

std::unique_ptr<Panel> makeLfo(PlasmaProcessor& pr, const char* title, P l) {
  auto p = std::make_unique<Panel>(pr, title, kKrypton);
  p->titleW = 70;
  p->combo(l + 0, 100);
  p->toggle(l + 4, "Retrig", 76, true);
  p->toggle(l + 2, "Sync", 62, true);
  p->combo(l + 3, 66);
  p->block(std::make_unique<LfoView>(pr.apvts, l + 0, kKrypton), 84);
  p->knob(l + 1, "Rate");
  p->knob(l + 5, "Fade in");
  return p;
}

std::unique_ptr<Panel> makeFx(PlasmaProcessor& pr, const char* title, std::vector<std::pair<P, const char*>> knobs, P on, P typeOrNone, P extraToggle1,
                              const char* extraName1, P extraToggle2, const char* extraName2, P combo2) {
  auto p = std::make_unique<Panel>(pr, title, kHydrogen);
  p->titleW = 70;
  p->toggle(on, "On", 56);
  if (typeOrNone != P::Count) p->combo(typeOrNone, 84);
  if (extraToggle1 != P::Count) p->toggle(extraToggle1, extraName1, 66);
  if (extraToggle2 != P::Count) p->toggle(extraToggle2, extraName2, 84);
  if (combo2 != P::Count) p->combo(combo2, 70);
  for (auto& k : knobs) p->knob(k.first, k.second);
  return p;
}

}  // namespace

// ===================================================================== root
struct PlasmaEditor::Root : juce::Component, juce::Timer {
  Root(PlasmaEditor& ed, PlasmaProcessor& p) : editor(ed), proc(p) {
    logo = std::make_unique<LogoView>();
    addAndMakeVisible(*logo);

    // ---- pages
    for (auto* c : {&sound, &mod, &fx}) addChildComponent(*c);

    // SOUND
    sound.place(makeOsc(p, "OSC A", P::a_on, ui::orange), {24, 84, 400, 528});
    sound.place(makeOsc(p, "OSC B", P::b_on, ui::violet), {440, 84, 400, 528});
    {
      auto f = std::make_unique<Panel>(p, "FILTER", ui::ice);
      f->titleW = 60;
      f->toggle(P::flt_on, "On", 56);
      f->combo(P::flt_type, 96);
      f->block(std::make_unique<FilterView>(p.apvts, ui::ice), 92);
      f->spread = true;
      f->knob(P::flt_cutoff, "Cutoff");
      f->knob(P::flt_res, "Reso");
      f->knob(P::flt_drive, "Drive");
      f->knob(P::flt_key, "Key trk");
      sound.place(std::move(f), {856, 84, 400, 230});
    }
    sound.place(makeEnv(p, "ENV 1  AMP", P::env1_a, "AMPLITUDE"), {856, 330, 400, 133});
    sound.place(makeEnv(p, "ENV 2  MOD", P::env2_a, "MOD SOURCE"), {856, 479, 400, 133});
    sound.place(std::make_unique<StripPanel>(p), {24, 628, 1232, 96});
    {
      auto m = std::make_unique<Panel>(p, "MODULATION", kKrypton);
      m->titleW = 100;
      m->block(std::make_unique<MatrixRows>(p, 0, 4, 22, kKrypton), 96);
      sound.place(std::move(m), {24, 740, 816, 136});
    }
    {
      auto c = std::make_unique<Panel>(p, "EFFECTS CHAIN", kHydrogen);
      c->titleW = 100;
      c->block(std::make_unique<ChainBlock>(p), 70);
      sound.place(std::move(c), {856, 740, 400, 136});
    }

    // MOD
    mod.place(makeLfo(p, "LFO 1", P::lfo1_shape), {24, 84, 299, 300});
    mod.place(makeLfo(p, "LFO 2", P::lfo2_shape), {335, 84, 299, 300});
    mod.place(makeLfo(p, "LFO 3", P::lfo3_shape), {646, 84, 299, 300});
    mod.place(makeLfo(p, "LFO 4", P::lfo4_shape), {957, 84, 299, 300});
    {
      auto m = std::make_unique<Panel>(p, "MOD MATRIX", kKrypton);
      m->titleW = 100;
      m->block(std::make_unique<MatrixRows>(p, 0, kNumSlots, 26, kKrypton), 10 * 28);
      mod.place(std::move(m), {24, 400, 816, 340});
    }
    mod.place(makeEnv(p, "ENV 3", P::env3_a, "MOD SOURCE"), {856, 400, 400, 133});
    {
      auto m = std::make_unique<Panel>(p, "MACROS", kKrypton);
      m->titleW = 70;
      m->spread = true;
      for (int i = 0; i < 4; ++i) m->knob(P::macro1 + i, ("Macro " + juce::String(i + 1)).toRawUTF8());
      mod.place(std::move(m), {856, 549, 400, 130});
    }

    // FX
    fx.place(makeFx(p, "DISTORTION", {{P::dist_drive, "Drive"}, {P::dist_tone, "Tone"}, {P::dist_mix, "Mix"}}, P::dist_on, P::dist_type, P::Count, "", P::Count, "", P::Count),
             {24, 84, 280, 190});
    fx.place(makeFx(p, "CHORUS", {{P::cho_rate, "Rate"}, {P::cho_depth, "Depth"}, {P::cho_mix, "Mix"}}, P::cho_on, P::Count, P::Count, "", P::Count, "", P::Count),
             {320, 84, 260, 190});
    fx.place(makeFx(p, "DELAY", {{P::dly_time, "Time"}, {P::dly_fb, "Feedback"}, {P::dly_damp, "Damp"}, {P::dly_mix, "Mix"}}, P::dly_on, P::Count, P::dly_sync, "Sync", P::dly_pp, "Ping-pong", P::dly_div),
             {596, 84, 346, 250});
    fx.place(makeFx(p, "REVERB", {{P::rev_size, "Size"}, {P::rev_damp, "Damp"}, {P::rev_width, "Width"}, {P::rev_mix, "Mix"}}, P::rev_on, P::Count, P::Count, "", P::Count, "", P::Count),
             {958, 84, 298, 250});

    // ---- header
    const char* names[3] = {"SOUND", "MOD", "FX"};
    for (int i = 0; i < 3; ++i) {
      tabs[i].setButtonText(names[i]);
      tabs[i].getProperties().set("kind", "tab");
      tabs[i].onClick = [this, i] { showPage(i); };
      addAndMakeVisible(tabs[i]);
    }
    const auto& presets = factoryPresets();
    for (size_t i = 0; i < presets.size(); ++i) presetBox.addItem(juce::String(presets[i].name).toUpperCase(), (int)i + 1);
    presetBox.setSelectedId(p.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram(presetBox.getSelectedId() - 1); };
    addAndMakeVisible(presetBox);
    prevBtn.setButtonText(juce::CharPointer_UTF8("\xe2\x80\xb9"));
    nextBtn.setButtonText(juce::CharPointer_UTF8("\xe2\x80\xba"));
    for (auto* b : {&prevBtn, &nextBtn}) b->getProperties().set("kind", "arrow");
    prevBtn.onClick = [this] { const int n = proc.getNumPrograms(); presetBox.setSelectedId(((presetBox.getSelectedId() - 2 + n) % n) + 1, juce::sendNotification); };
    nextBtn.onClick = [this] { const int n = proc.getNumPrograms(); presetBox.setSelectedId((presetBox.getSelectedId() % n) + 1, juce::sendNotification); };
    saveBtn.setButtonText("SAVE");
    loadBtn.setButtonText("LOAD");
    saveBtn.onClick = [this] { savePreset(); };
    loadBtn.onClick = [this] { loadPreset(); };
    for (auto* b : {&prevBtn, &nextBtn, &saveBtn, &loadBtn}) addAndMakeVisible(*b);
    voices.setFont(monoFont(11.f));
    voices.setColour(juce::Label::textColourId, ui::dim);
    voices.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(voices);
    tipsBtn.setButtonText("TIPS");
    tipsBtn.setColour(juce::ToggleButton::tickColourId, ui::orange);
    tipsBtn.getProperties().set("chip", true);
    tipsBtn.setToggleState(true, juce::dontSendNotification);
    tipsBtn.setTooltip("Show or hide these help tooltips.");
    tipsBtn.onClick = [this] { editor.setTipsEnabled(tipsBtn.getToggleState()); };
    addAndMakeVisible(tipsBtn);
    showPage(0);
    startTimerHz(8);
  }

  void timerCallback() override { voices.setText("VOICES " + juce::String(proc.activeVoices()), juce::dontSendNotification); }

  void showPage(int i) {
    sound.setVisible(i == 0);
    mod.setVisible(i == 1);
    fx.setVisible(i == 2);
    for (int t = 0; t < 3; ++t) tabs[t].setToggleState(t == i, juce::dontSendNotification);
  }

  void paint(juce::Graphics& g) override {
    g.fillAll(ui::bg);
    g.setGradientFill(juce::ColourGradient(ui::violet.withAlpha(0.06f), 0.f, 0.f, ui::bg, 0.f, 90.f, false));
    g.fillRect(0, 0, getWidth(), 90);
    // wordmark
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff4f1ee), 100.f, 0.f, juce::Colour(0xffd9e6ff), 240.f, 0.f, false));
    g.setFont(Fonts::make(Fonts::get().round, 34.f, 0.05f));
    g.drawText("plasma", 104, 20, 200, 48, juce::Justification::centredLeft);
  }

  void resized() override {
    logo->setBounds(24, 24, 70, 44);
    prevBtn.setBounds(420, 26, 24, 30);
    presetBox.setBounds(448, 28, 240, 26);
    nextBtn.setBounds(692, 26, 24, 30);
    saveBtn.setBounds(736, 26, 52, 30);
    loadBtn.setBounds(790, 26, 52, 30);
    tabs[0].setBounds(884, 26, 76, 30);
    tabs[1].setBounds(966, 26, 54, 30);
    tabs[2].setBounds(1026, 26, 40, 30);
    tipsBtn.setBounds(1090, 28, 58, 26);
    voices.setBounds(1156, 26, 100, 30);
    for (auto* c : {&sound, &mod, &fx}) c->setBounds(0, 0, ui::kBaseW, ui::kBaseH);
  }

  void savePreset() {
    chooser = std::make_unique<juce::FileChooser>("Save Plasma preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.plasma");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                         [this](const juce::FileChooser& fc) {
                           auto f = fc.getResult();
                           if (f == juce::File()) return;
                           if (auto xml = proc.apvts.copyState().createXml()) xml->writeTo(f.withFileExtension("plasma"));
                         });
  }
  void loadPreset() {
    chooser = std::make_unique<juce::FileChooser>("Load Plasma preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.plasma");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) {
      auto f = fc.getResult();
      if (!f.existsAsFile()) return;
      if (auto xml = juce::XmlDocument::parse(f))
        if (xml->hasTagName(proc.apvts.state.getType())) proc.apvts.replaceState(juce::ValueTree::fromXml(*xml));
    });
  }

  PlasmaEditor& editor;
  PlasmaProcessor& proc;
  std::unique_ptr<LogoView> logo;
  Page sound, mod, fx;
  juce::TextButton tabs[3], prevBtn, nextBtn, saveBtn, loadBtn;
  juce::ComboBox presetBox;
  juce::Label voices;
  juce::ToggleButton tipsBtn;
  std::unique_ptr<juce::FileChooser> chooser;
};

// ===================================================================== editor
PlasmaEditor::PlasmaEditor(PlasmaProcessor& p) : AudioProcessorEditor(&p), proc_(p) {
  setLookAndFeel(&lnf_);
  root_ = std::make_unique<Root>(*this, p);
  addAndMakeVisible(*root_);
  setResizable(true, true);
  setResizeLimits(800, 562, 2560, 1800);
  getConstrainer()->setFixedAspectRatio((double)ui::kBaseW / (double)ui::kBaseH);
  setSize(1152, 810);
  startTimerHz(1);
}

PlasmaEditor::~PlasmaEditor() { setLookAndFeel(nullptr); }

void PlasmaEditor::setTipsEnabled(bool on) { tooltips_.setMillisecondsBeforeTipAppears(on ? 450 : 3600000); }
void PlasmaEditor::timerCallback() {}

void PlasmaEditor::paint(juce::Graphics& g) { g.fillAll(ui::bg); }

void PlasmaEditor::resized() {
  if (!root_) return;
  root_->setBounds(0, 0, ui::kBaseW, ui::kBaseH);
  root_->setTransform(juce::AffineTransform::scale((float)getWidth() / (float)ui::kBaseW));
}
