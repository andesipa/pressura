// Runs the real processor offline: sidechain routing, vis saturation and presets.
// cmake --build build --target OPTO2SidechainTest && build/OPTO2SidechainTest_artefacts/Release/OPTO2SidechainTest
#include "../Source/PluginProcessor.h"

namespace
{
    constexpr double sr = 48000.0;
    constexpr int    block = 512;
    const double twoPi = juce::MathConstants<double>::twoPi;

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
                const float main = 0.01f * (float) std::sin (twoPi * 220 * t);
                float sc = 0.0f;
                if (sidechain == 1 && tk < 0.15) sc = 0.7f * (float) (std::exp (-tk * 20) * std::sin (twoPi * 60 * tk));
                if (sidechain == 2 && th < 0.03) sc = 0.7f * (float) (std::exp (-th * 120) * std::sin (twoPi * 8000 * th));
                buf.setSample (0, i, main); buf.setSample (1, i, main);
                buf.setSample (2, i, sc);   buf.setSample (3, i, sc);
            }
            p.processBlock (buf, midi);
            if (start > sr * 0.5) maxGr = juce::jmax (maxGr, p.meterGrDb.load());
        }
        return maxGr;
    }

    // Steady -12 dBFS 1 kHz tone for 3 s, nothing on the sidechain. Measures the last second:
    // output level relative to the input (dB) and THD (%).
    struct Tone { float levelDb, thd; };
    Tone tone (Opto2Processor& p)
    {
        p.prepareToPlay (sr, block);
        juce::AudioBuffer<float> buf (4, block);
        juce::MidiBuffer midi;
        std::vector<double> y;
        for (int start = 0; start < (int) (sr * 3); start += block)
        {
            for (int i = 0; i < block; ++i)
            {
                const float x = 0.25f * (float) std::sin (twoPi * 1000 * (start + i) / sr);
                buf.setSample (0, i, x); buf.setSample (1, i, x); buf.setSample (2, i, 0.0f); buf.setSample (3, i, 0.0f);
            }
            p.processBlock (buf, midi);
            if (start >= sr * 2) for (int i = 0; i < block; ++i) y.push_back (buf.getSample (0, i));
        }
        auto mag = [&] (double f)
        {
            double c = 0, s = 0;
            for (size_t i = 0; i < y.size(); ++i) { c += y[i] * std::cos (twoPi * f * i / sr); s += y[i] * std::sin (twoPi * f * i / sr); }
            return std::sqrt (c * c + s * s) * 2.0 / (double) y.size();
        };
        const double f1 = mag (1000);
        double h = 0;
        for (int k = 2; k < 12; ++k) h += std::pow (mag (1000.0 * k), 2);
        return { (float) (20.0 * std::log10 (f1 / 0.25)), (float) (100.0 * std::sqrt (h) / f1) };
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    int failures = 0;
    auto expect = [&] (const juce::String& what, bool ok) { std::printf ("%-62s %s\n", what.toRawUTF8(), ok ? "ok" : "FAIL"); failures += ok ? 0 : 1; };
    auto check = [&] (const char* what, float gr, bool expectCompression)
    {
        expect (juce::String (what) + juce::String::formatted ("  GR %5.1f dB", gr), expectCompression ? gr > 3.0f : gr < 0.5f);
    };

    Opto2Processor p;
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses  = { juce::AudioChannelSet::stereo(), juce::AudioChannelSet::stereo() };
    layout.outputBuses = { juce::AudioChannelSet::stereo() };
    if (! p.setBusesLayout (layout)) { std::printf ("sidechain layout rejected\nFAIL\n"); return 1; }

    // ---- Sidechain ----
    set (p, "peak", 60.0f);
    set (p, "ext", 0.0f);
    check ("alius off, kick on sidechain, quiet main: ignores", run (p, 1), false);
    set (p, "ext", 1.0f);
    check ("alius on, kick on sidechain: compresses", run (p, 1), true);
    check ("alius on, hats on sidechain: compresses", run (p, 2), true);
    check ("alius on, nothing routed: no compression", run (p, 0), false);
    {
        const auto t = tone (p);   // ducking: no makeup, the track only goes down
        expect (juce::String::formatted ("alius on, clean, nothing on sidechain: unity  %+.2f dB", t.levelDb), std::abs (t.levelDb) < 0.2f);
    }
    set (p, "vis", 1.0f);
    check ("vis + alius, kick on sidechain: ducks", run (p, 1), true);
    set (p, "ext", 0.0f);

    // ---- vis = drive + saturation; clean stays clean ----
    set (p, "vis", 0.0f); set (p, "peak", 0.0f);
    {
        const auto t = tone (p);
        expect (juce::String::formatted ("clean, reductio 0: no distortion  THD %.2f %%", t.thd), t.thd < 0.1f);
    }
    float lastThd = 0.0f;
    for (float amount : { 0.0f, 50.0f, 100.0f })
    {
        set (p, "vis", 1.0f); set (p, "peak", amount);
        const auto t = tone (p);
        expect (juce::String::formatted ("vis %3.0f: saturates  THD %4.1f %%  level %+.1f dB", amount, t.thd, t.levelDb),
                t.thd > 1.0f && t.thd < 15.0f && t.thd > lastThd && std::abs (t.levelDb) < 6.0f);
        lastThd = t.thd;
    }
    set (p, "vis", 0.0f); set (p, "peak", 40.0f);

    // ---- Presets ----
    auto value = [&] (const char* id) { auto* q = p.apvts.getParameter (id); return q->convertFrom0to1 (q->getValue()); };

    expect ("10 programs, names visible to the host", p.getNumPrograms() == 10 && numFactoryPresets == 10
                                                      && p.getProgramName (9) == "Bass Sidechain");
    bool allExact = true;
    for (int i = 0; i < numFactoryPresets; ++i)
    {
        p.loadPreset (i);
        const auto& f = factoryPresets[i];
        const float want[] = { f.peak, f.speed, f.output, f.mix, f.limit ? 1.f : 0.f, f.vis ? 1.f : 0.f, f.ext ? 1.f : 0.f };
        const char* ids[] = { "peak", "speed", "output", "mix", "limit", "vis", "ext" };
        for (int k = 0; k < 7; ++k)
            if (std::abs (value (ids[k]) - want[k]) > 0.051f) { allExact = false; std::printf ("  %s: %s = %.2f, want %.2f\n", f.name, ids[k], value (ids[k]), want[k]); }
    }
    expect ("every preset sets all 7 parameters exactly", allExact);

    p.loadPreset (4); set (p, "peak", 33.0f);
    p.setCurrentProgram (4);
    expect ("host re-selecting the current preset keeps user tweaks", std::abs (value ("peak") - 33.0f) < 0.06f);

    juce::MemoryBlock state; p.getStateInformation (state);
    Opto2Processor q; q.setStateInformation (state.getData(), (int) state.getSize());
    expect ("session save/restore keeps preset index and tweaks",
            q.getCurrentProgram() == 4 && std::abs (q.apvts.getParameter ("peak")->convertFrom0to1 (q.apvts.getParameter ("peak")->getValue()) - 33.0f) < 0.06f);

    p.loadPreset (0);
    expect ("Default preset = factory defaults", std::abs (value ("peak") - 40.0f) < 0.06f && value ("ext") < 0.5f && value ("vis") < 0.5f);

    p.loadPreset (9);   // Bass Sidechain
    check ("Bass Sidechain preset: kick on sidechain ducks", run (p, 1), true);

    std::printf (failures == 0 ? "ALL PASSED\n" : "%d FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
