#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "OptoCompressor.h"
#include "SidechainEQ.h"

class Opto2Processor : public juce::AudioProcessor
{
public:
    Opto2Processor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Pressura"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> meterGrDb { 0.0f };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    OptoCompressor comp;
    SidechainEQ scEq;
    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outSmoothed;

    std::atomic<float>* peakParam  = nullptr;
    std::atomic<float>* speedParam  = nullptr;
    std::atomic<float>* driveParam  = nullptr;
    std::atomic<float>* mixParam   = nullptr;
    std::atomic<float>* limitParam = nullptr;
    std::atomic<float>* outputParam = nullptr;
    std::atomic<float>* extParam    = nullptr;
    std::atomic<float>* lowParam    = nullptr;
    std::atomic<float>* midParam    = nullptr;
    std::atomic<float>* highParam   = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Opto2Processor)
};
