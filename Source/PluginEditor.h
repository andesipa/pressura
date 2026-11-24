#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Paleta medida sobre fotos del EP-1320 medieval
namespace Palette
{
    const juce::Colour bodyLight  { 0xffa38e57 };  // cuerpo dorado oliva (zona iluminada)
    const juce::Colour bodyDark   { 0xff8c7841 };  // cuerpo (zona en sombra)
    const juce::Colour ink        { 0xff2a1d14 };  // serigrafía sobre el cuerpo

    const juce::Colour panelGreen { 0xff2c3530 };  // panel superior verde
    const juce::Colour keyGreen   { 0xff1c2622 };  // teclas verde oscuro
    const juce::Colour keyBrown   { 0xff3c2521 };  // teclas marrón (numéricas)
    const juce::Colour keyCream   { 0xffe0d2c3 };  // teclas crema / pergamino
    const juce::Colour red        { 0xffbf0e06 };  // perilla y acentos rojos
    const juce::Colour keyRed     { 0xff9e150a };  // tecla roja

    const juce::Colour knobCream  { 0xffe6dccf };
    const juce::Colour knobBrown  { 0xff3d211c };

    const juce::Colour screen     { 0xff191413 };  // display negro-marrón
    const juce::Colour led        { 0xffe8321e };  // dígitos LED rojos
    const juce::Colour speaker    { 0xff332015 };
    const juce::Colour faderSlot  { 0xff1c170d };
    const juce::Colour faderCap   { 0xff1d2b26 };
}

class OptoLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos,
                           float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    int getSliderThumbRadius (juce::Slider&) override { return 12; }

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;

    // Menú de presets con los colores del visor
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                            bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                            const juce::String& shortcutKeyText, const juce::Drawable* icon, const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override { return gothic (17.0f); }

    static juce::Font gothic (float size);
    static juce::Font mono (float size, bool bold = true);
};

// Barra de presets: ‹ nombre › — click en el nombre abre la lista
class PresetBar : public juce::Component
{
public:
    std::function<void (int delta)> onStep;
    std::function<void()> onOpenMenu;
    void setPresetName (const juce::String& n) { if (n != name) { name = n; repaint(); } }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    juce::String name;
    static constexpr int arrowW = 16;
};

class Opto2Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Opto2Editor (Opto2Processor&);
    ~Opto2Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawTopPanel (juce::Graphics&);
    void drawDisplay  (juce::Graphics&, juce::Rectangle<float>);
    void setLimit (bool shouldLimit);
    void toggle (const char* paramId, bool isOn);
    bool visOn() const { return lastValues[5] > 0.5f; }
    bool extOn() const { return lastValues[6] > 0.5f; }

    Opto2Processor& proc;
    OptoLookAndFeel lnf;

    juce::Slider peakKnob, speedKnob, outputKnob, mixFader;
    juce::TextButton compKey { "compressio" }, limitKey { "limes" }, visKey { "vis" }, extKey { "alius" };
    PresetBar presetBar;
    void showPresetMenu();

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> peakAtt, speedAtt, outputAtt, mixAtt;

    juce::String valueText (int param) const;
    juce::String nameText  (int param) const;

    static constexpr int numTracked = 7;
    static constexpr const char* trackedIds[numTracked] = { "peak", "speed", "output", "mix", "limit", "vis", "ext" };
    float  lastValues[numTracked] {};
    int    activeParam  = -1;     // -1 = nada tocado todavía
    int    shownPreset  = 0, seenPresetLoads = 0;
    double presetChangeMs = -1.0e9;
    double lastChangeMs = 0.0;

    juce::Rectangle<float> displayArea;
    float shownGr = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Opto2Editor)
};
