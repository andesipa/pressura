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
    driveParam  = apvts.getRawParameterValue ("vis");
    mixParam    = apvts.getRawParameterValue ("mix");
    limitParam  = apvts.getRawParameterValue ("limit");
    outputParam = apvts.getRawParameterValue ("output");
    extParam    = apvts.getRawParameterValue ("ext");
    lowParam    = apvts.getRawParameterValue ("low");
    midParam    = apvts.getRawParameterValue ("mid");
    highParam   = apvts.getRawParameterValue ("high");
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

    // Peak Reduction pasa a ser Drive contra un umbral fijo
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { "vis", 1 }, "Drive Mode", false));

    // El detector escucha la entrada de sidechain (otra pista) en vez de la propia
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { "ext", 1 }, "External Sidechain", false));

    // EQ de la señal del detector: cuánto dispara la compresión cada banda (en el mínimo, apagada)
    NormalisableRange<float> eqRange (SidechainEQ::minDb, SidechainEQ::maxDb, 0.1f);
    eqRange.setSkewForCentre (0.0f);   // 0 dB con la perilla derecha
    for (auto [id, name] : { std::pair { "low", "Sidechain Low" }, { "mid", "Sidechain Mid" }, { "high", "Sidechain High" } })
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, eqRange, 0.0f,
            AudioParameterFloatAttributes().withLabel ("dB")));

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
    comp.setDriveMode (driveParam->load() > 0.5f);
    comp.prepare (sampleRate);
    scEq.setGainsDb (lowParam->load(), midParam->load(), highParam->load());
    scEq.prepare (sampleRate);
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

    comp.setPeakReduction (peakParam->load());
    comp.setSpeed (speedParam->load());
    comp.setDriveMode (driveParam->load() > 0.5f);
    comp.setLimitMode (limitParam->load() > 0.5f);
    mixSmoothed.setTargetValue (mixParam->load() * 0.01f);
    outSmoothed.setTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));
    scEq.setGainsDb (lowParam->load(), midParam->load(), highParam->load());

    auto main = getBusBuffer (buffer, false, 0);
    float* l = main.getWritePointer (0);
    float* r = main.getNumChannels() > 1 ? main.getWritePointer (1) : nullptr;

    // Detector: la propia pista, o la de sidechain si "alius" está prendida y el host la conectó
    const float* scL = l;
    const float* scR = r;
    if (extParam->load() > 0.5f && getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto side = getBusBuffer (buffer, true, 1);
        if (side.getNumChannels() > 0)
        {
            scL = side.getReadPointer (0);
            scR = side.getNumChannels() > 1 ? side.getReadPointer (1) : nullptr;
        }
    }

    float blockMaxGr = 0.0f;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float dryL = l[i];
        const float dryR = r != nullptr ? r[i] : dryL;

        // El detector lee antes de escribir la salida: con sidechain propio, scL apunta a l
        float detL = scL[i];
        float detR = scR != nullptr ? scR[i] : detL;
        scEq.process (detL, detR);

        const float g   = comp.process (detL, detR);
        const float mix = mixSmoothed.getNextValue();
        const float out = outSmoothed.getNextValue();

        l[i] = out * (dryL + mix * (dryL * g - dryL));
        if (r != nullptr)
            r[i] = out * (dryR + mix * (dryR * g - dryR));

        blockMaxGr = juce::jmax (blockMaxGr, comp.getGainReductionDb());
    }

    meterGrDb.store (blockMaxGr);
}

juce::AudioProcessorEditor* Opto2Processor::createEditor()
{
    return new Opto2Editor (*this);
}

void Opto2Processor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void Opto2Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Opto2Processor();
}
