// Runs the real processor offline with a sidechain input and checks the routing and EQ.
// cmake --build build --target OPTO2SidechainTest && build/OPTO2SidechainTest_artefacts/Release/OPTO2SidechainTest
#include "../Source/PluginProcessor.h"

namespace
{
    constexpr double sr = 48000.0;
    constexpr int    block = 512;

    void set (Opto2Processor& p, const char* id, float value)
    {
        auto* param = p.apvts.getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 (value));
    }

    // 3 s of: main = quiet 220 Hz tone, sidechain = kick (60 Hz) or hats (8 kHz) or nothing.
    // Returns the largest gain reduction (dB) seen after the first 0.5 s.
    float run (Opto2Processor& p, int sidechain /* 0 none, 1 kick, 2 hats */)
    {
        p.prepareToPlay (sr, block);
        juce::AudioBuffer<float> buf (4, block);   // main L/R + sidechain L/R
        juce::MidiBuffer midi;
        float maxGr = 0.0f;
        for (int start = 0; start < (int) (sr * 3); start += block)
        {
            for (int i = 0; i < block; ++i)
            {
                const double t = (start + i) / sr, tk = std::fmod (t, 0.5), th = std::fmod (t, 0.125);
                const float main = 0.01f * (float) std::sin (2 * juce::MathConstants<double>::pi * 220 * t);
                float sc = 0.0f;
                if (sidechain == 1 && tk < 0.15) sc = 0.7f * (float) (std::exp (-tk * 20) * std::sin (2 * juce::MathConstants<double>::pi * 60 * tk));
                if (sidechain == 2 && th < 0.03) sc = 0.7f * (float) (std::exp (-th * 120) * std::sin (2 * juce::MathConstants<double>::pi * 8000 * th));
                buf.setSample (0, i, main); buf.setSample (1, i, main);
                buf.setSample (2, i, sc);   buf.setSample (3, i, sc);
            }
            p.processBlock (buf, midi);
            if (start > sr * 0.5) maxGr = juce::jmax (maxGr, p.meterGrDb.load());
        }
        return maxGr;
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    int failures = 0;
    auto check = [&] (const char* what, float gr, bool expectCompression)
    {
        const bool ok = expectCompression ? gr > 3.0f : gr < 0.5f;
        std::printf ("%-55s GR %5.1f dB  %s\n", what, gr, ok ? "ok" : "FAIL");
        failures += ok ? 0 : 1;
    };

    Opto2Processor p;
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses  = { juce::AudioChannelSet::stereo(), juce::AudioChannelSet::stereo() };
    layout.outputBuses = { juce::AudioChannelSet::stereo() };
    if (! p.setBusesLayout (layout)) { std::printf ("sidechain layout rejected\nFAIL\n"); return 1; }
    set (p, "peak", 60.0f);

    set (p, "ext", 0.0f);
    check ("alius off, kick on sidechain, quiet main: ignores kick", run (p, 1), false);

    set (p, "ext", 1.0f);
    check ("alius on, kick on sidechain: compresses", run (p, 1), true);
    check ("alius on, nothing routed: no compression", run (p, 0), false);
    check ("alius on, hats on sidechain, EQ flat: compresses", run (p, 2), true);

    set (p, "low", 6.0f); set (p, "mid", -24.0f); set (p, "high", -24.0f);
    check ("alius on, hats, gravis +6 / medius+acutus OFF: ignores hats", run (p, 2), false);
    check ("alius on, kick, gravis +6 / medius+acutus OFF: compresses", run (p, 1), true);

    std::printf (failures == 0 ? "ALL PASSED\n" : "%d FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
