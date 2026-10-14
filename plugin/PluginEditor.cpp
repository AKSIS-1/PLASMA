#include "PluginEditor.h"

using namespace plasma;
using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

// ===================================================================== look & feel
PlasmaLookAndFeel::PlasmaLookAndFeel() {
  setColour(juce::ResizableWindow::backgroundColourId, ui::bg);
  setColour(juce::Label::textColourId, ui::text);
  setColour(juce::Slider::textBoxTextColourId, ui::text);
  setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
  setColour(juce::ComboBox::textColourId, ui::text);
  setColour(juce::PopupMenu::backgroundColourId, ui::panel);
  setColour(juce::PopupMenu::textColourId, ui::text);
  setColour(juce::PopupMenu::highlightedBackgroundColourId, ui::violet);
  setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

juce::Font PlasmaLookAndFeel::getComboBoxFont(juce::ComboBox&) { return juce::Font(juce::FontOptions(13.f)); }
juce::Font PlasmaLookAndFeel::getTextButtonFont(juce::TextButton&, int) { return juce::Font(juce::FontOptions(14.f, juce::Font::bold)); }

juce::Label* PlasmaLookAndFeel::createSliderTextBox(juce::Slider& s) {
  auto* l = LookAndFeel_V4::createSliderTextBox(s);
  l->setFont(juce::Font(juce::FontOptions(11.f)));
  l->setColour(juce::Label::textColourId, ui::dim);
  l->setJustificationType(juce::Justification::centred);
  l->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
  l->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
  l->setColour(juce::Label::outlineWhenEditingColourId, ui::magenta);
  return l;
}

void PlasmaLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1,
                                         juce::Slider& s) {
  const auto b = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(4.f);
  const float d = std::min(b.getWidth(), b.getHeight());
  const auto c = b.getCentre();
  const float r = d * 0.5f;
  const float ang = a0 + pos * (a1 - a0);
  const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0 && std::abs(s.getMinimum() + s.getMaximum()) < 1e-6;
  const float zero = bipolar ? a0 + 0.5f * (a1 - a0) : a0;

  juce::Path track;
  track.addCentredArc(c.x, c.y, r - 2, r - 2, 0, a0, a1, true);
  g.setColour(ui::panelHi);
  g.strokePath(track, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

  juce::Path val;
  val.addCentredArc(c.x, c.y, r - 2, r - 2, 0, std::min(zero, ang), std::max(zero, ang), true);
  g.setGradientFill(juce::ColourGradient(ui::magenta, c.x - r, c.y, ui::cyan, c.x + r, c.y, false));
  g.strokePath(val, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

  g.setColour(ui::panelHi.brighter(0.1f));
  g.fillEllipse(c.x - r * 0.62f, c.y - r * 0.62f, r * 1.24f, r * 1.24f);
  juce::Path tick;
  tick.addRoundedRectangle(-1.5f, -r * 0.6f, 3.f, r * 0.38f, 1.5f);
  g.setColour(ui::text);
  g.fillPath(tick, juce::AffineTransform::rotation(ang).translated(c.x, c.y));
}

void PlasmaLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                         juce::Slider::SliderStyle, juce::Slider& s) {
  const float cy = y + h * 0.5f;
  g.setColour(ui::panelHi);
  g.fillRoundedRectangle((float)x, cy - 2.f, (float)w, 4.f, 2.f);
  const bool bipolar = s.getMinimum() < 0.0;
  const float zeroX = bipolar ? x + (float)s.valueToProportionOfLength(0.0) * w : (float)x;
  g.setGradientFill(juce::ColourGradient(ui::magenta, (float)x, cy, ui::cyan, (float)(x + w), cy, false));
  g.fillRoundedRectangle(std::min(zeroX, pos), cy - 2.f, std::abs(pos - zeroX), 4.f, 2.f);
  g.setColour(ui::text);
  g.fillEllipse(pos - 5.f, cy - 5.f, 10.f, 10.f);
}

void PlasmaLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool hi, bool) {
  const auto r = b.getLocalBounds().toFloat().reduced(1.f);
  const bool on = b.getToggleState();
  g.setColour(on ? ui::violet.withAlpha(0.35f) : ui::panelHi);
  g.fillRoundedRectangle(r, 5.f);
  g.setColour(on ? ui::magenta : (hi ? ui::dim : ui::panelHi.brighter(0.3f)));
  g.drawRoundedRectangle(r, 5.f, 1.2f);
  g.setColour(on ? ui::magenta : ui::dim);
  g.fillEllipse(r.getX() + 7.f, r.getCentreY() - 3.5f, 7.f, 7.f);
  g.setColour(on ? ui::text : ui::dim);
  g.setFont(juce::Font(juce::FontOptions(12.f, juce::Font::bold)));
  g.drawText(b.getButtonText(), r.withTrimmedLeft(18.f), juce::Justification::centredLeft);
}

void PlasmaLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) {
  const auto r = juce::Rectangle<float>(0.f, 0.f, (float)w, (float)h).reduced(0.5f);
  g.setColour(ui::panelHi);
  g.fillRoundedRectangle(r, 5.f);
  g.setColour(box.hasKeyboardFocus(false) ? ui::magenta : ui::panelHi.brighter(0.3f));
  g.drawRoundedRectangle(r, 5.f, 1.f);
  juce::Path arrow;
  arrow.addTriangle((float)w - 16.f, h * 0.5f - 2.f, (float)w - 8.f, h * 0.5f - 2.f, (float)w - 12.f, h * 0.5f + 3.f);
  g.setColour(ui::cyan);
  g.fillPath(arrow);
}

void PlasmaLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down) {
  const auto r = b.getLocalBounds().toFloat().reduced(1.f);
  const bool on = b.getToggleState();
  g.setColour(on ? ui::violet : (down ? ui::panelHi.brighter(0.2f) : hi ? ui::panelHi.brighter(0.1f) : ui::panelHi));
  g.fillRoundedRectangle(r, 6.f);
  if (on) { g.setColour(ui::magenta); g.drawRoundedRectangle(r, 6.f, 1.4f); }
}

// ===================================================================== value formatting
static juce::String formatValue(const ParamDef& d, double v) {
  const juce::String id(d.id);
  auto endsWith = [&](const char* s) { return id.endsWith(s); };
  if (d.type == ParamType::Int) return juce::String((int)std::lround(v));
  if (endsWith("_cutoff") || endsWith("_damp") && id.startsWith("dly"))
    return v >= 1000 ? juce::String(v / 1000.0, 1) + " kHz" : juce::String((int)std::lround(v)) + " Hz";
  if ((id.startsWith("env") && (endsWith("_a") || endsWith("_d") || endsWith("_r"))) || id == "glide" || endsWith("_fade"))
    return v < 1.0 ? juce::String((int)std::lround(v * 1000)) + " ms" : juce::String(v, 2) + " s";
  if (endsWith("_rate")) return juce::String(v, 2) + " Hz";
  if (id == "dly_time") return juce::String((int)std::lround(v)) + " ms";
  if (endsWith("_fine")) return juce::String(v, 0) + " ct";
  if (d.min < 0.f) return juce::String(v, 2);
  if (d.max <= 1.f) return juce::String((int)std::lround(v * 100)) + "%";
  return juce::String(v, 2);
}

// ===================================================================== widgets
namespace {

struct Knob : juce::Component {
  Knob(juce::AudioProcessorValueTreeState& apvts, P id, const juce::String& caption) : caption_(caption) {
    const ParamDef& def = paramDefs()[(int)id];
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 14);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxTextColourId, ui::dim);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    addAndMakeVisible(slider);
    att = std::make_unique<SliderAtt>(apvts, def.id, slider);
    slider.textFromValueFunction = [def](double v) { return formatValue(def, v); };
    slider.valueFromTextFunction = [](const juce::String& t) { return (double)t.getFloatValue(); };
    slider.setDoubleClickReturnValue(true, def.def);
    slider.updateText();
  }
  void resized() override { slider.setBounds(getLocalBounds().withTrimmedTop(13)); }
  void paint(juce::Graphics& g) override {
    g.setColour(ui::dim);
    g.setFont(juce::Font(juce::FontOptions(11.f, juce::Font::bold)));
    g.drawText(caption_.toUpperCase(), 0, 0, getWidth(), 13, juce::Justification::centred);
  }
  juce::Slider slider;
  std::unique_ptr<SliderAtt> att;
  juce::String caption_;
};

struct Combo : juce::ComboBox {
  Combo(juce::AudioProcessorValueTreeState& apvts, P id) {
    const ParamDef& def = paramDefs()[(int)id];
    int i = 1;
    for (const auto& s : def.choiceList()) addItem(s, i++);
    att = std::make_unique<ComboAtt>(apvts, def.id, *this);
  }
  std::unique_ptr<ComboAtt> att;
};

struct Toggle : juce::ToggleButton {
  Toggle(juce::AudioProcessorValueTreeState& apvts, P id, const juce::String& text) {
    setButtonText(text);
    att = std::make_unique<ButtonAtt>(apvts, paramDefs()[(int)id].id, *this);
  }
  std::unique_ptr<ButtonAtt> att;
};

// Live wavetable view: ghost stack of frames + the currently selected (interpolated) frame
struct WaveView : juce::Component, juce::Timer {
  WaveView(juce::AudioProcessorValueTreeState& a, P tbl, P pos, P warp, P amt) : apvts(a), pt(tbl), pp(pos), pw(warp), pa(amt) {
    startTimerHz(30);
  }
  void timerCallback() override {
    const float t = get(pt), p = get(pp), w = get(pw), a = get(pa);
    if (t != lt || p != lp || w != lw || a != la) { lt = t; lp = p; lw = w; la = a; repaint(); }
  }
  float get(P id) const { return apvts.getRawParameterValue(paramDefs()[(int)id].id)->load(); }
  void paint(juce::Graphics& g) override {
    auto r = getLocalBounds().toFloat();
    g.setColour(ui::bg);
    g.fillRoundedRectangle(r, 6.f);
    const auto& wt = WavetableBank::get().table((int)lt);
    r = r.reduced(6.f);
    const int N = (int)r.getWidth();
    const int stack = 8;
    for (int k = 0; k < stack; ++k) {
      const int f = k * (Wavetable::kFrames - 1) / (stack - 1);
      const float* d = wt.frame(f);
      const float yoff = r.getBottom() - r.getHeight() * 0.28f - (float)k * r.getHeight() * 0.045f;
      juce::Path path;
      for (int i = 0; i < N; ++i) {
        const float v = d[(i * wt.frameLength()) / N];
        const float y = yoff - v * r.getHeight() * 0.22f;
        if (i == 0) path.startNewSubPath(r.getX(), y); else path.lineTo(r.getX() + i, y);
      }
      g.setColour(ui::violet.withAlpha(0.12f + 0.02f * k));
      g.strokePath(path, juce::PathStrokeType(1.f));
    }
    const float fp = lp * (Wavetable::kFrames - 1);
    juce::Path cur;
    for (int i = 0; i <= N; ++i) {
      const float ph = (float)i / (float)N;
      const float v = wt.lookup(0, fp, std::min(ph, 0.9999f));
      const float y = r.getCentreY() - v * r.getHeight() * 0.42f;
      if (i == 0) cur.startNewSubPath(r.getX(), y); else cur.lineTo(r.getX() + i, y);
    }
    g.setColour(ui::magenta.withAlpha(0.25f));
    g.strokePath(cur, juce::PathStrokeType(5.f));
    g.setGradientFill(juce::ColourGradient(ui::magenta, r.getX(), 0.f, ui::cyan, r.getRight(), 0.f, false));
    g.strokePath(cur, juce::PathStrokeType(1.8f));
    g.setColour(ui::dim);
    g.setFont(juce::Font(juce::FontOptions(10.f)));
    g.drawText(juce::String(wt.name) + (lw > 0 ? "  +warp" : ""), r.reduced(2.f), juce::Justification::topRight);
  }
  juce::AudioProcessorValueTreeState& apvts;
  P pt, pp, pw, pa;
  float lt = -1, lp = -1, lw = -1, la = -1;
};

constexpr int kKnobW = 76, kKnobH = 74;

// A titled panel: header controls on the first row, optional extra component (e.g. wave view), then a knob grid.
struct Panel : juce::Component {
  Panel(PlasmaProcessor& p, const juce::String& t) : proc(p), title(t) {}
  Knob& knob(P id, const juce::String& cap) {
    knobs.push_back(std::make_unique<Knob>(proc.apvts, id, cap));
    addAndMakeVisible(*knobs.back());
    return *knobs.back();
  }
  Combo& combo(P id, int w) {
    combos.push_back(std::make_unique<Combo>(proc.apvts, id));
    header.push_back({combos.back().get(), w});
    addAndMakeVisible(*combos.back());
    return *combos.back();
  }
  Toggle& toggle(P id, const juce::String& text, int w = 62) {
    toggles.push_back(std::make_unique<Toggle>(proc.apvts, id, text));
    header.push_back({toggles.back().get(), w});
    addAndMakeVisible(*toggles.back());
    return *toggles.back();
  }
  void setExtra(std::unique_ptr<juce::Component> c, int h) {
    extra = std::move(c);
    extraH = h;
    addAndMakeVisible(*extra);
  }
  void paint(juce::Graphics& g) override {
    auto r = getLocalBounds().toFloat().reduced(1.f);
    g.setColour(ui::panel);
    g.fillRoundedRectangle(r, 10.f);
    g.setColour(ui::panelHi);
    g.drawRoundedRectangle(r, 10.f, 1.f);
    g.setColour(ui::magenta);
    g.fillRoundedRectangle(r.getX() + 12.f, r.getY() + 10.f, 3.f, 14.f, 1.5f);
    g.setColour(ui::text);
    g.setFont(juce::Font(juce::FontOptions(14.f, juce::Font::bold)));
    g.drawText(title, 22, 8, 90, 18, juce::Justification::centredLeft);
  }
  void resized() override {
    int x = 22 + titleW, y = 6;
    for (auto& h : header) {
      if (x + h.second > getWidth() - 8 && x > 22 + titleW) { x = 14; y += 26; }
      h.first->setBounds(x, y, h.second, 22);
      x += h.second + 6;
    }
    int top = y + 28;
    if (extra) { extra->setBounds(10, top, getWidth() - 20, extraH); top += extraH + 4; }
    const int cols = std::max(1, (getWidth() - 10) / knobW);
    for (size_t i = 0; i < knobs.size(); ++i)
      knobs[i]->setBounds(8 + (int)(i % cols) * knobW, top + (int)(i / cols) * kKnobH, knobW - 2, kKnobH - 2);
  }
  PlasmaProcessor& proc;
  juce::String title;
  int titleW = 64, extraH = 0, knobW = kKnobW;
  std::vector<std::unique_ptr<Knob>> knobs;
  std::vector<std::unique_ptr<Combo>> combos;
  std::vector<std::unique_ptr<Toggle>> toggles;
  std::vector<std::pair<juce::Component*, int>> header;
  std::unique_ptr<juce::Component> extra;
};

std::unique_ptr<Panel> makeOsc(PlasmaProcessor& pr, const char* title, P base) {
  auto p = std::make_unique<Panel>(pr, title);
  p->titleW = 26;
  p->toggle(base + 0, "On", 50);
  p->combo(base + 1, 118);
  p->combo(base + 3, 82);
  p->setExtra(std::make_unique<WaveView>(pr.apvts, base + 1, base + 2, base + 3, base + 4), 92);
  p->knob(base + 2, "Pos");
  p->knob(base + 4, "Warp");
  p->knob(base + 5, "Level");
  p->knob(base + 6, "Pan");
  p->knob(base + 7, "Octave");
  p->knob(base + 8, "Semi");
  p->knob(base + 9, "Fine");
  p->knob(base + 10, "Unison");
  p->knob(base + 11, "Detune");
  p->knob(base + 12, "Blend");
  p->knob(base + 13, "Stereo");
  p->knob(base + 14, "Phase");
  p->knob(base + 15, "Rand");
  return p;
}

std::unique_ptr<Panel> makeEnv(PlasmaProcessor& pr, const char* title, P a) {
  auto p = std::make_unique<Panel>(pr, title);
  p->knobW = 58;
  p->knob(a + 0, "Attack");
  p->knob(a + 1, "Decay");
  p->knob(a + 2, "Sustain");
  p->knob(a + 3, "Release");
  return p;
}

std::unique_ptr<Panel> makeLfo(PlasmaProcessor& pr, const char* title, P l) {
  auto p = std::make_unique<Panel>(pr, title);
  p->titleW = 46;
  p->combo(l + 0, 96);
  p->toggle(l + 4, "Retrig", 70);
  p->toggle(l + 2, "Sync", 58);
  p->combo(l + 3, 70);
  p->knob(l + 1, "Rate");
  p->knob(l + 5, "Fade In");
  return p;
}

struct MatrixPanel : Panel {
  struct Row {
    std::unique_ptr<Combo> src, dst;
    juce::Slider amt;
    std::unique_ptr<SliderAtt> att;
  };
  MatrixPanel(PlasmaProcessor& p) : Panel(p, "MOD MATRIX") {
    titleW = 40;
    for (int i = 0; i < kNumSlots; ++i) {
      auto r = std::make_unique<Row>();
      const P b = P::slot1_src + i * (P::slot2_src - P::slot1_src);
      r->src = std::make_unique<Combo>(p.apvts, b);
      r->dst = std::make_unique<Combo>(p.apvts, b + 1);
      r->amt.setSliderStyle(juce::Slider::LinearHorizontal);
      r->amt.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 20);
      r->amt.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
      r->amt.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
      addAndMakeVisible(*r->src);
      addAndMakeVisible(*r->dst);
      addAndMakeVisible(r->amt);
      r->att = std::make_unique<SliderAtt>(p.apvts, paramDefs()[(int)(b + 2)].id, r->amt);
      r->amt.textFromValueFunction = [](double v) { return juce::String((int)std::lround(v * 100)) + "%"; };
      r->amt.valueFromTextFunction = [](const juce::String& t) { return t.getDoubleValue() / 100.0; };
      r->amt.setDoubleClickReturnValue(true, 0.0);
      r->amt.updateText();
      rows.push_back(std::move(r));
    }
  }
  void resized() override {
    const int w = getWidth();
    for (size_t i = 0; i < rows.size(); ++i) {
      const int y = 40 + (int)i * 31;
      rows[i]->src->setBounds(16, y, 130, 24);
      rows[i]->dst->setBounds(156, y, 150, 24);
      rows[i]->amt.setBounds(316, y, w - 332, 24);
    }
  }
  void paint(juce::Graphics& g) override {
    Panel::paint(g);
    g.setColour(ui::dim);
    g.setFont(juce::Font(juce::FontOptions(10.f, juce::Font::bold)));
    g.drawText("SOURCE", 18, 24, 100, 12, juce::Justification::left);
    g.drawText("DESTINATION", 158, 24, 100, 12, juce::Justification::left);
    g.drawText("AMOUNT", 318, 24, 100, 12, juce::Justification::left);
  }
  std::vector<std::unique_ptr<Row>> rows;
};

}  // namespace

// ===================================================================== pages
struct PlasmaEditor::Pages {
  juce::Component sound, mod, fx;
  std::vector<std::unique_ptr<Panel>> owned;

  template <class T>
  T* add(juce::Component& parent, std::unique_ptr<T> p, juce::Rectangle<int> r) {
    auto* raw = p.get();
    parent.addAndMakeVisible(*raw);
    raw->setBounds(r);
    owned.push_back(std::move(p));
    return raw;
  }

  explicit Pages(PlasmaProcessor& pr) {
    // ---- SOUND
    add(sound, makeOsc(pr, "A", P::a_on), {10, 8, 410, 372});
    add(sound, makeOsc(pr, "B", P::b_on), {428, 8, 410, 372});
    {
      auto p = std::make_unique<Panel>(pr, "SUB");
      p->titleW = 30;
      p->toggle(P::sub_on, "On", 48);
      p->combo(P::sub_shape, 78);
      p->knob(P::sub_oct, "Octave");
      p->knob(P::sub_level, "Level");
      p->knob(P::noise_level, "Noise");
      add(sound, std::move(p), {846, 8, 244, 124});
    }
    {
      auto p = std::make_unique<Panel>(pr, "GLOBAL");
      p->titleW = 62;
      p->toggle(P::mono, "Mono", 62);
      p->toggle(P::legato, "Legato", 74);
      p->knob(P::master, "Master");
      p->knob(P::glide, "Glide");
      p->knob(P::bend_range, "Bend");
      add(sound, std::move(p), {846, 140, 244, 124});
    }
    {
      auto p = std::make_unique<Panel>(pr, "MACROS");
      p->titleW = 60;
      p->knobW = 58;
      p->knob(P::macro1, "M1");
      p->knob(P::macro2, "M2");
      p->knob(P::macro3, "M3");
      p->knob(P::macro4, "M4");
      add(sound, std::move(p), {846, 272, 244, 108});
    }
    {
      auto p = std::make_unique<Panel>(pr, "FILTER");
      p->titleW = 54;
      p->toggle(P::flt_on, "On", 46);
      p->combo(P::flt_type, 84);
      p->knob(P::flt_cutoff, "Cutoff");
      p->knob(P::flt_res, "Reso");
      p->knob(P::flt_drive, "Drive");
      p->knob(P::flt_key, "Key Trk");
      add(sound, std::move(p), {10, 388, 330, 134});
    }
    add(sound, makeEnv(pr, "ENV 1 (AMP)", P::env1_a), {348, 388, 242, 134});
    add(sound, makeEnv(pr, "ENV 2", P::env2_a), {598, 388, 242, 134});
    add(sound, makeEnv(pr, "ENV 3", P::env3_a), {848, 388, 242, 134});

    // ---- MOD
    add(mod, makeLfo(pr, "LFO 1", P::lfo1_shape), {10, 8, 262, 150});
    add(mod, makeLfo(pr, "LFO 2", P::lfo2_shape), {280, 8, 262, 150});
    add(mod, makeLfo(pr, "LFO 3", P::lfo3_shape), {550, 8, 262, 150});
    add(mod, makeLfo(pr, "LFO 4", P::lfo4_shape), {820, 8, 270, 150});
    {
      auto p = std::make_unique<Panel>(pr, "MACROS");
      p->titleW = 70;
      p->knob(P::macro1, "Macro 1");
      p->knob(P::macro2, "Macro 2");
      p->knob(P::macro3, "Macro 3");
      p->knob(P::macro4, "Macro 4");
      add(mod, std::move(p), {10, 166, 170, 356});
    }
    add(mod, std::make_unique<MatrixPanel>(pr), {188, 166, 902, 356});

    // ---- FX
    {
      auto p = std::make_unique<Panel>(pr, "DIST");
      p->titleW = 40;
      p->toggle(P::dist_on, "On", 46);
      p->combo(P::dist_type, 80);
      p->knob(P::dist_drive, "Drive");
      p->knob(P::dist_tone, "Tone");
      p->knob(P::dist_mix, "Mix");
      add(fx, std::move(p), {10, 8, 262, 150});
    }
    {
      auto p = std::make_unique<Panel>(pr, "CHORUS");
      p->titleW = 66;
      p->toggle(P::cho_on, "On", 46);
      p->knob(P::cho_rate, "Rate");
      p->knob(P::cho_depth, "Depth");
      p->knob(P::cho_mix, "Mix");
      add(fx, std::move(p), {280, 8, 262, 150});
    }
    {
      auto p = std::make_unique<Panel>(pr, "DELAY");
      p->titleW = 46;
      p->toggle(P::dly_on, "On", 46);
      p->toggle(P::dly_sync, "Sync", 58);
      p->toggle(P::dly_pp, "Ping", 54);
      p->knob(P::dly_time, "Time");
      p->combo(P::dly_div, 64);
      p->knob(P::dly_fb, "Feedback");
      p->knob(P::dly_damp, "Damp");
      p->knob(P::dly_mix, "Mix");
      add(fx, std::move(p), {550, 8, 340, 210});
    }
    {
      auto p = std::make_unique<Panel>(pr, "REVERB");
      p->titleW = 68;
      p->toggle(P::rev_on, "On", 46);
      p->knob(P::rev_size, "Size");
      p->knob(P::rev_damp, "Damp");
      p->knob(P::rev_width, "Width");
      p->knob(P::rev_mix, "Mix");
      add(fx, std::move(p), {898, 8, 192, 210});
    }
  }
};

// ===================================================================== editor
PlasmaEditor::PlasmaEditor(PlasmaProcessor& p) : AudioProcessorEditor(&p), proc_(p) {
  setLookAndFeel(&lnf_);

  pages_ = std::make_unique<Pages>(p);
  for (auto* c : {&pages_->sound, &pages_->mod, &pages_->fx}) {
    addChildComponent(*c);
  }
  const char* names[3] = {"SOUND", "MOD", "FX"};
  for (int i = 0; i < 3; ++i) {
    tabs_[i].setButtonText(names[i]);
    tabs_[i].setClickingTogglesState(false);
    tabs_[i].onClick = [this, i] { showPage(i); };
    addAndMakeVisible(tabs_[i]);
  }
  const auto& presets = factoryPresets();
  for (size_t i = 0; i < presets.size(); ++i) presetBox_.addItem(presets[i].name, (int)i + 1);
  presetBox_.setSelectedId(p.getCurrentProgram() + 1, juce::dontSendNotification);
  presetBox_.onChange = [this] { proc_.setCurrentProgram(presetBox_.getSelectedId() - 1); };
  addAndMakeVisible(presetBox_);
  prevBtn_.onClick = [this] {
    const int n = proc_.getNumPrograms();
    presetBox_.setSelectedId(((presetBox_.getSelectedId() - 2 + n) % n) + 1, juce::sendNotification);
  };
  nextBtn_.onClick = [this] {
    const int n = proc_.getNumPrograms();
    presetBox_.setSelectedId((presetBox_.getSelectedId() % n) + 1, juce::sendNotification);
  };
  saveBtn_.onClick = [this] { savePreset(); };
  loadBtn_.onClick = [this] { loadPreset(); };
  for (auto* b : {&prevBtn_, &nextBtn_, &saveBtn_, &loadBtn_}) addAndMakeVisible(*b);
  voices_.setJustificationType(juce::Justification::centredRight);
  voices_.setColour(juce::Label::textColourId, ui::dim);
  addAndMakeVisible(voices_);
  showPage(0);
  setSize(1100, 610);
  startTimerHz(10);
}

PlasmaEditor::~PlasmaEditor() { setLookAndFeel(nullptr); }

void PlasmaEditor::showPage(int i) {
  page_ = i;
  pages_->sound.setVisible(i == 0);
  pages_->mod.setVisible(i == 1);
  pages_->fx.setVisible(i == 2);
  for (int t = 0; t < 3; ++t) tabs_[t].setToggleState(t == i, juce::dontSendNotification);
}

void PlasmaEditor::timerCallback() {
  voices_.setText("Voices: " + juce::String(proc_.activeVoices()), juce::dontSendNotification);
}

void PlasmaEditor::paint(juce::Graphics& g) {
  g.fillAll(ui::bg);
  g.setGradientFill(juce::ColourGradient(ui::violet.withAlpha(0.18f), 0.f, 0.f, ui::bg, 0.f, 70.f, false));
  g.fillRect(0, 0, getWidth(), 70);
  g.setGradientFill(juce::ColourGradient(ui::magenta, 18.f, 0.f, ui::cyan, 150.f, 0.f, false));
  g.setFont(juce::Font(juce::FontOptions(30.f, juce::Font::bold)));
  g.drawText("PLASMA", 18, 6, 160, 42, juce::Justification::centredLeft);
}

void PlasmaEditor::resized() {
  if (!pages_) return;
  for (int i = 0; i < 3; ++i) tabs_[i].setBounds(190 + i * 86, 14, 80, 30);
  prevBtn_.setBounds(470, 14, 28, 30);
  presetBox_.setBounds(502, 14, 220, 30);
  nextBtn_.setBounds(726, 14, 28, 30);
  saveBtn_.setBounds(766, 14, 60, 30);
  loadBtn_.setBounds(832, 14, 60, 30);
  voices_.setBounds(getWidth() - 140, 14, 124, 30);
  const auto content = juce::Rectangle<int>(0, 56, getWidth(), getHeight() - 56);
  for (auto* c : {&pages_->sound, &pages_->mod, &pages_->fx}) c->setBounds(content);
}

void PlasmaEditor::savePreset() {
  chooser_ = std::make_unique<juce::FileChooser>("Save Plasma preset",
                                                 juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.plasma");
  chooser_->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                            juce::FileBrowserComponent::warnAboutOverwriting,
                        [this](const juce::FileChooser& fc) {
                          auto f = fc.getResult();
                          if (f == juce::File()) return;
                          if (auto xml = proc_.apvts.copyState().createXml()) xml->writeTo(f.withFileExtension("plasma"));
                        });
}

void PlasmaEditor::loadPreset() {
  chooser_ = std::make_unique<juce::FileChooser>("Load Plasma preset",
                                                 juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.plasma");
  chooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                        [this](const juce::FileChooser& fc) {
                          auto f = fc.getResult();
                          if (!f.existsAsFile()) return;
                          if (auto xml = juce::XmlDocument::parse(f))
                            if (xml->hasTagName(proc_.apvts.state.getType()))
                              proc_.apvts.replaceState(juce::ValueTree::fromXml(*xml));
                        });
}
