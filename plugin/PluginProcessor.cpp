#include "PluginProcessor.h"

#include "PluginEditor.h"

using namespace plasma;

juce::AudioProcessorValueTreeState::ParameterLayout PlasmaProcessor::createLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;
  for (const auto& d : paramDefs()) {
    const juce::ParameterID pid{d.id, 1};
    switch (d.type) {
      case ParamType::Bool:
        layout.add(std::make_unique<juce::AudioParameterBool>(pid, d.name, d.def > 0.5f));
        break;
      case ParamType::Int:
        layout.add(std::make_unique<juce::AudioParameterInt>(pid, d.name, (int)d.min, (int)d.max, (int)d.def));
        break;
      case ParamType::Choice: {
        juce::StringArray items;
        for (const auto& s : d.choiceList()) items.add(s);
        layout.add(std::make_unique<juce::AudioParameterChoice>(pid, d.name, items, (int)d.def));
        break;
      }
      case ParamType::Float: {
        const ParamDef def = d;
        juce::NormalisableRange<float> range(
            def.min, def.max,
            [def](float, float, float n) { return def.fromNorm(n); },
            [def](float, float, float v) { return def.toNorm(v); },
            [](float, float, float v) { return v; });
        layout.add(std::make_unique<juce::AudioParameterFloat>(pid, d.name, range, d.def));
        break;
      }
    }
  }
  return layout;
}

PlasmaProcessor::PlasmaProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PLASMA", createLayout()) {
  for (int i = 0; i < kNumParams; ++i) raw_[i] = apvts.getRawParameterValue(paramDefs()[i].id);
  WavetableBank::get();  // build tables up front, off the audio thread
}

void PlasmaProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  synth_.prepare((float)sampleRate, std::max(samplesPerBlock, 1));
}

bool PlasmaProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
  return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PlasmaProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
  juce::ScopedNoDenormals noDenormals;
  const int n = buffer.getNumSamples();
  buffer.clear();

  for (int i = 0; i < kNumParams; ++i) values_[i] = raw_[i]->load();
  synth_.setParams(values_);

  if (auto* ph = getPlayHead())
    if (auto pos = ph->getPosition())
      if (auto bpm = pos->getBpm()) synth_.setTempo(*bpm);

  float* L = buffer.getWritePointer(0);
  float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

  int cursor = 0;
  auto renderTo = [&](int end) {
    if (end > cursor) { synth_.render(L + cursor, R + cursor, end - cursor); cursor = end; }
  };
  for (const auto meta : midi) {
    const auto msg = meta.getMessage();
    renderTo(std::min(meta.samplePosition, n));
    if (msg.isNoteOn()) synth_.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
    else if (msg.isNoteOff()) synth_.noteOff(msg.getNoteNumber());
    else if (msg.isAllNotesOff()) synth_.allNotesOff();
    else if (msg.isAllSoundOff()) synth_.allNotesOff(true);
    else if (msg.isController()) synth_.controller(msg.getControllerNumber(), msg.getControllerValue());
    else if (msg.isPitchWheel()) synth_.pitchWheel((msg.getPitchWheelValue() - 8192) / 8192.f);
    else if (msg.isChannelPressure()) synth_.aftertouch(msg.getChannelPressureValue() / 127.f);
  }
  renderTo(n);
  voices_.store(synth_.activeVoices());
}

void PlasmaProcessor::setCurrentProgram(int index) {
  const auto& presets = factoryPresets();
  if (index < 0 || index >= (int)presets.size()) return;
  currentProgram_ = index;
  ParamSet ps;
  applyPreset(ps, presets[(size_t)index]);
  for (int i = 0; i < kNumParams; ++i)
    if (auto* p = apvts.getParameter(paramDefs()[i].id))
      p->setValueNotifyingHost(p->convertTo0to1(ps.v[i]));
}

const juce::String PlasmaProcessor::getProgramName(int index) {
  const auto& presets = factoryPresets();
  return index >= 0 && index < (int)presets.size() ? juce::String(presets[(size_t)index].name) : juce::String();
}

void PlasmaProcessor::getStateInformation(juce::MemoryBlock& dest) {
  auto state = apvts.copyState();
  state.setProperty("program", currentProgram_, nullptr);
  if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}

void PlasmaProcessor::setStateInformation(const void* data, int size) {
  if (auto xml = getXmlFromBinary(data, size))
    if (xml->hasTagName(apvts.state.getType())) {
      auto st = juce::ValueTree::fromXml(*xml);
      currentProgram_ = st.getProperty("program", 0);
      apvts.replaceState(st);
    }
}

juce::AudioProcessorEditor* PlasmaProcessor::createEditor() { return new PlasmaEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PlasmaProcessor(); }
