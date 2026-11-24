#include "PluginProcessor.h"
#include "PluginEditor.h"

Opto2Processor::Opto2Processor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
                        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    peakParam   = apvts.getRawParameterValue ("peak");
    speedParam  = apvts.getRawParameterValue ("speed");
    visParam    = apvts.getRawParameterValue ("vis");
    mixParam    = apvts.getRawParameterValue ("mix");
    limitParam  = apvts.getRawParameterValue ("limit");
    outputParam = apvts.getRawParameterValue ("output");
    extParam    = apvts.getRawParameterValue ("ext");
}

juce::AudioProcessorValueTreeState::ParameterLayout Opto2Processor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "peak", 1 }, "Peak Reduction",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 40.0f,
        AudioParameterFloatAttributes().withLabel ("")));

    // 50 = el opto original; arriba bombea
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "speed", 1 }, "Speed",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f,
        AudioParameterFloatAttributes().withLabel ("")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "output", 1 }, "Output",
        NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { "limit", 1 }, "Limit", false));

    // Peak Reduction pasa a ser Drive contra un umbral fijo + saturación
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { "vis", 1 }, "Vis (Drive + Saturation)", false));

    // El detector escucha la entrada de sidechain (otra pista) en vez de la propia
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { "ext", 1 }, "External Sidechain", false));


    return layout;
}

bool Opto2Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    if (out != layouts.getMainInputChannelSet())
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo())
            return false;
    }
    return true;
}

void Opto2Processor::prepareToPlay (double sampleRate, int)
{
    // Parámetros antes de prepare() para que drive/makeup no arranquen con una rampa
    comp.setPeakReduction (peakParam->load());
    comp.setSpeed (speedParam->load());
    comp.setLimitMode (limitParam->load() > 0.5f);
    comp.setDriveMode (visParam->load() > 0.5f);
    comp.prepare (sampleRate);
    sat.setAmount (peakParam->load());
    sat.prepare (sampleRate);
    mixSmoothed.reset (sampleRate, 0.03);
    mixSmoothed.setCurrentAndTargetValue (mixParam->load() * 0.01f);
    outSmoothed.reset (sampleRate, 0.03);
    outSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));
}

void Opto2Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto ch = getMainBusNumInputChannels(); ch < getMainBusNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const bool vis = visParam->load() > 0.5f;
    comp.setPeakReduction (peakParam->load());
    comp.setSpeed (speedParam->load());
    comp.setDriveMode (vis);
    comp.setLimitMode (limitParam->load() > 0.5f);
    sat.setAmount (peakParam->load());
    mixSmoothed.setTargetValue (mixParam->load() * 0.01f);
    outSmoothed.setTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));

    auto main = getBusBuffer (buffer, false, 0);
    float* l = main.getWritePointer (0);
    float* r = main.getNumChannels() > 1 ? main.getWritePointer (1) : nullptr;

    // Detector: la propia pista, o la de sidechain si "alius" está prendida y el host la conectó
    const float* scL = l;
    const float* scR = r;
    bool external = false;
    if (extParam->load() > 0.5f && getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto side = getBusBuffer (buffer, true, 1);
        if (side.getNumChannels() > 0)
        {
            scL = side.getReadPointer (0);
            scR = side.getNumChannels() > 1 ? side.getReadPointer (1) : nullptr;
            external = true;
        }
    }

    float blockMaxGr = 0.0f;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float dryL = l[i];
        const float dryR = r != nullptr ? r[i] : dryL;

        // El detector lee antes de escribir la salida: con sidechain propio, scL apunta a l
        const float detL = scL[i];
        const float detR = scR != nullptr ? scR[i] : detL;

        // Con sidechain externo es ducking: solo la reducción, sin drive ni makeup
        const float gDb = comp.processDb (detL, detR);
        const float g   = std::pow (10.0f, (external ? -comp.getGainReductionDb() : gDb) / 20.0f);

        float wetL = dryL * g, wetR = dryR * g;
        if (vis)
            sat.process (wetL, wetR);

        const float mix = mixSmoothed.getNextValue();
        const float out = outSmoothed.getNextValue();
        l[i] = out * (dryL + mix * (wetL - dryL));
        if (r != nullptr)
            r[i] = out * (dryR + mix * (wetR - dryR));
        blockMaxGr = juce::jmax (blockMaxGr, comp.getGainReductionDb());
    }

    meterGrDb.store (blockMaxGr);
}

void Opto2Processor::loadPreset (int index)
{
    if (index < 0 || index >= numFactoryPresets)
        return;

    const auto& p = factoryPresets[index];
    auto set = [this] (const char* id, float value)
    {
        if (auto* param = apvts.getParameter (id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 (value));
            param->endChangeGesture();
        }
    };
    set ("peak", p.peak);    set ("speed", p.speed);  set ("output", p.output); set ("mix", p.mix);
    set ("limit", p.limit ? 1.0f : 0.0f);
    set ("vis",   p.vis   ? 1.0f : 0.0f);
    set ("ext",   p.ext   ? 1.0f : 0.0f);
    currentPreset = index;
    ++presetLoads;
}

void Opto2Processor::setCurrentProgram (int index)
{
    // Algunos hosts vuelven a llamar al programa actual al restaurar una sesión:
    // en ese caso no hay que pisar los ajustes que el usuario hizo encima del preset.
    if (index != currentPreset.load())
        loadPreset (index);
}

const juce::String Opto2Processor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, numFactoryPresets) ? juce::String (factoryPresets[index].name) : juce::String();
}

juce::AudioProcessorEditor* Opto2Processor::createEditor()
{
    return new Opto2Editor (*this);
}

void Opto2Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset.load(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void Opto2Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentPreset = juce::jlimit (0, numFactoryPresets - 1, (int) state.getProperty ("preset", 0));
            apvts.replaceState (state);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Opto2Processor();
}
