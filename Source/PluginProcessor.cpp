#include "PluginProcessor.h"
#include "PluginEditor.h"

Opto2Processor::Opto2Processor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    peakParam   = apvts.getRawParameterValue ("peak");
    speedParam  = apvts.getRawParameterValue ("speed");
    driveParam  = apvts.getRawParameterValue ("vis");
    mixParam    = apvts.getRawParameterValue ("mix");
    limitParam  = apvts.getRawParameterValue ("limit");
    outputParam = apvts.getRawParameterValue ("output");
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

    return layout;
}

bool Opto2Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void Opto2Processor::prepareToPlay (double sampleRate, int)
{
    // Parámetros antes de prepare() para que drive/makeup no arranquen con una rampa
    comp.setPeakReduction (peakParam->load());
    comp.setSpeed (speedParam->load());
    comp.setLimitMode (limitParam->load() > 0.5f);
    comp.setDriveMode (driveParam->load() > 0.5f);
    comp.prepare (sampleRate);
    mixSmoothed.reset (sampleRate, 0.03);
    mixSmoothed.setCurrentAndTargetValue (mixParam->load() * 0.01f);
    outSmoothed.reset (sampleRate, 0.03);
    outSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));
}

void Opto2Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    comp.setPeakReduction (peakParam->load());
    comp.setSpeed (speedParam->load());
    comp.setDriveMode (driveParam->load() > 0.5f);
    comp.setLimitMode (limitParam->load() > 0.5f);
    mixSmoothed.setTargetValue (mixParam->load() * 0.01f);
    outSmoothed.setTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));

    const int numCh = juce::jmin (2, buffer.getNumChannels());
    float* l = buffer.getWritePointer (0);
    float* r = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    float blockMaxGr = 0.0f;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float dryL = l[i];
        const float dryR = r != nullptr ? r[i] : dryL;

        const float g   = comp.process (dryL, dryR);
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
