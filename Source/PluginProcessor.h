#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "OptoCompressor.h"
#include "Saturator.h"
#include "Presets.h"

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

    // Presets de fábrica (Presets.h): el host los ve como programas
    int getNumPrograms() override { return numFactoryPresets; }
    int getCurrentProgram() override { return currentPreset.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    // Carga un preset aunque ya sea el actual (desde la UI, para volver a sus valores)
    void loadPreset (int index);

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> meterGrDb { 0.0f };
    std::atomic<int>   currentPreset { 0 };
    std::atomic<int>   presetLoads { 0 };   // sube en cada carga, aunque sea el mismo preset

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    OptoCompressor comp;
    Saturator sat;   // modo vis
    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outSmoothed;

    std::atomic<float>* peakParam  = nullptr;
    std::atomic<float>* speedParam  = nullptr;
    std::atomic<float>* visParam    = nullptr;
    std::atomic<float>* mixParam   = nullptr;
    std::atomic<float>* limitParam = nullptr;
    std::atomic<float>* outputParam = nullptr;
    std::atomic<float>* extParam    = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Opto2Processor)
};
