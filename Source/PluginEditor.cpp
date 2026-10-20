#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

//==============================================================================
// Fuentes
//==============================================================================
Font OptoLookAndFeel::gothic (float size)
{
    static Typeface::Ptr tf = Typeface::createSystemTypefaceFor (BinaryData::UnifrakturMaguntia_ttf,
                                                                 BinaryData::UnifrakturMaguntia_ttfSize);
    return Font (FontOptions (tf).withHeight (size));
}

Font OptoLookAndFeel::mono (float size, bool bold)
{
    return Font (FontOptions (Font::getDefaultMonospacedFontName(), size,
                              bold ? Font::bold : Font::plain));
}

//==============================================================================
// Perillas: cilindro con collar oscuro, como las del EP-1320
//==============================================================================
void OptoLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, Slider& s)
{
    const auto area   = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto centre = area.getCentre();
    const float r     = jmin (area.getWidth(), area.getHeight()) * 0.5f - 10.0f;

    // Escala serigrafiada
    g.setColour (Palette::ink);
    for (int i = 0; i <= 10; ++i)
    {
        const float a  = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
        const bool big = (i % 5 == 0);
        const float r1 = r + 3.0f, r2 = r + (big ? 9.0f : 6.0f);
        g.drawLine (centre.x + r1 * std::sin (a), centre.y - r1 * std::cos (a),
                    centre.x + r2 * std::sin (a), centre.y - r2 * std::cos (a), big ? 1.6f : 1.0f);
    }

    // Sombra y collar
    g.setColour (Colours::black.withAlpha (0.28f));
    g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre (centre.translated (1.5f, 4.0f)));
    g.setColour (Palette::ink);
    g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre (centre));

    // Cara de la perilla
    const float faceR = r * 0.84f;
    const auto face   = s.findColour (Slider::thumbColourId);
    auto faceRect     = Rectangle<float> (faceR * 2, faceR * 2).withCentre (centre);
    g.setGradientFill (ColourGradient (face.brighter (0.18f), faceRect.getX(), faceRect.getY(),
                                       face.darker (0.25f), faceRect.getRight(), faceRect.getBottom(), false));
    g.fillEllipse (faceRect);

    // Hundimiento central (como la perilla roja "pulsus")
    g.setColour (face.darker (0.12f));
    g.drawEllipse (Rectangle<float> (faceR * 1.25f, faceR * 1.25f).withCentre (centre), 1.2f);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.drawEllipse (Rectangle<float> (faceR * 1.25f, faceR * 1.25f).withCentre (centre.translated (0.6f, 0.8f)), 0.8f);

    // Indicador
    const float a = startAngle + pos * (endAngle - startAngle);
    g.setColour (s.findColour (Slider::rotarySliderFillColourId));
    g.drawLine (centre.x + faceR * 0.45f * std::sin (a), centre.y - faceR * 0.45f * std::cos (a),
                centre.x + (faceR - 3.0f) * std::sin (a), centre.y - (faceR - 3.0f) * std::cos (a), 3.0f);
}

//==============================================================================
// Fader vertical: ranura oscura + tapa verde oscuro
//==============================================================================
void OptoLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float sliderPos,
                                        float, float, Slider::SliderStyle, Slider&)
{
    const float cx = (float) x + (float) w * 0.5f;

    auto slot = Rectangle<float> (cx - 4.0f, (float) y, 8.0f, (float) h);
    g.setColour (Palette::faderSlot);
    g.fillRoundedRectangle (slot, 4.0f);
    g.setColour (Colours::white.withAlpha (0.10f));
    g.drawLine (slot.getX() + 1, slot.getBottom() - 2, slot.getRight() - 1, slot.getBottom() - 2, 1.0f);

    // Tramo "wet" en rojo
    g.setColour (Palette::red.withAlpha (0.85f));
    g.fillRoundedRectangle (slot.withTop (sliderPos).reduced (2.0f, 1.0f), 2.0f);

    // Tapa
    auto cap = Rectangle<float> (30.0f, 18.0f).withCentre ({ cx, sliderPos });
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (cap.translated (1.0f, 3.0f), 3.0f);
    g.setGradientFill (ColourGradient (Palette::faderCap.brighter (0.25f), cap.getX(), cap.getY(),
                                       Palette::faderCap.darker (0.3f), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 3.0f);
    g.setColour (Palette::keyCream.withAlpha (0.8f));
    g.drawLine (cap.getX() + 5, cap.getCentreY(), cap.getRight() - 5, cap.getCentreY(), 1.5f);
}

//==============================================================================
// Teclas: bloque con bisel; activa = roja, inactiva = marrón
//==============================================================================
void OptoLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&,
                                            bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom (4.0f);

    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (r.translated (0.0f, down ? 1.5f : 4.0f), 4.0f);
    if (down) r = r.translated (0.0f, 2.5f);

    const auto base = b.getToggleState() ? Palette::keyRed : Palette::keyBrown;
    g.setColour (base.darker (0.35f));
    g.fillRoundedRectangle (r, 4.0f);
    auto top = r.reduced (3.0f).withTrimmedBottom (2.0f);
    g.setGradientFill (ColourGradient (base.brighter (highlighted ? 0.25f : 0.15f), top.getX(), top.getY(),
                                       base, top.getX(), top.getBottom(), false));
    g.fillRoundedRectangle (top, 3.0f);
}

void OptoLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool down)
{
    g.setFont (gothic (17.0f));
    g.setColour (Palette::keyCream);
    auto r = b.getLocalBounds().withTrimmedBottom (6);
    if (down) r = r.translated (0, 2);
    g.drawText (b.getButtonText(), r, Justification::centred);
}

//==============================================================================
// Editor
//==============================================================================
Opto2Editor::Opto2Editor (Opto2Processor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    for (auto* k : { &peakKnob, &attackKnob, &outputKnob })
    {
        k->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        k->setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        k->setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        addAndMakeVisible (*k);
    }

    // Mismas combinaciones que las perillas del aparato
    peakKnob.setColour   (Slider::thumbColourId, Palette::red);        // "pulsus"
    peakKnob.setColour   (Slider::rotarySliderFillColourId, Palette::keyCream);
    attackKnob.setColour (Slider::thumbColourId, Palette::knobBrown);  // "metrum"
    attackKnob.setColour (Slider::rotarySliderFillColourId, Palette::keyCream);
    outputKnob.setColour (Slider::thumbColourId, Palette::knobCream);  // "magnitudo"
    outputKnob.setColour (Slider::rotarySliderFillColourId, Palette::ink);

    mixFader.setSliderStyle (Slider::LinearVertical);
    mixFader.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (mixFader);

    peakAtt   = std::make_unique<SliderAttachment> (proc.apvts, "peak",   peakKnob);
    attackAtt = std::make_unique<SliderAttachment> (proc.apvts, "attack", attackKnob);
    outputAtt = std::make_unique<SliderAttachment> (proc.apvts, "output", outputKnob);
    mixAtt    = std::make_unique<SliderAttachment> (proc.apvts, "mix",    mixFader);

    peakKnob.setDoubleClickReturnValue   (true, 40.0);
    attackKnob.setDoubleClickReturnValue (true, 10.0);
    outputKnob.setDoubleClickReturnValue (true, 0.0);
    mixFader.setDoubleClickReturnValue   (true, 100.0);

    compKey.onClick  = [this] { setLimit (false); };
    limitKey.onClick = [this] { setLimit (true);  };
    addAndMakeVisible (compKey);
    addAndMakeVisible (limitKey);

    for (int i = 0; i < numTracked; ++i)
        lastValues[i] = proc.apvts.getRawParameterValue (trackedIds[i])->load();

    setSize (388, 508);
    startTimerHz (30);
}

Opto2Editor::~Opto2Editor()
{
    setLookAndFeel (nullptr);
}

void Opto2Editor::setLimit (bool shouldLimit)
{
    if (auto* param = proc.apvts.getParameter ("limit"))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (shouldLimit ? 1.0f : 0.0f);
        param->endChangeGesture();
    }
}

void Opto2Editor::timerCallback()
{
    const float gr = proc.meterGrDb.load();
    shownGr = gr > shownGr ? gr : shownGr * 0.88f + gr * 0.12f;

    // Detecta qué parámetro se movió (desde la UI o por automatización del host)
    for (int i = 0; i < numTracked; ++i)
    {
        const float v = proc.apvts.getRawParameterValue (trackedIds[i])->load();
        if (std::abs (v - lastValues[i]) > 1.0e-6f)
        {
            lastValues[i]   = v;
            activeParam     = i;
            lastChangeMs    = Time::getMillisecondCounterHiRes();
        }
    }

    const bool limit = lastValues[4] > 0.5f;
    compKey.setToggleState  (! limit, dontSendNotification);
    limitKey.setToggleState (limit,   dontSendNotification);

    repaint (displayArea.toNearestInt());
}

void Opto2Editor::resized()
{
    displayArea = { 0.0f, 120.0f, 388.0f, 96.0f };

    const int knobY = 254, knobS = 100;
    peakKnob.setBounds   (86,  knobY, knobS, knobS);
    attackKnob.setBounds (188, knobY, knobS, knobS);
    outputKnob.setBounds (285, knobY, knobS, knobS);

    mixFader.setBounds (10, 258, 50, 190);

    compKey.setBounds  (96,  382, 136, 62);
    limitKey.setBounds (244, 382, 136, 62);
}

void Opto2Editor::drawTopPanel (Graphics& g)
{
    // Panel verde con título, de borde a borde
    auto panel = Rectangle<float> (0.0f, 0.0f, 274.0f, 120.0f);
    g.setColour (Palette::panelGreen);
    g.fillRect (panel);

    g.setFont (OptoLookAndFeel::gothic (44.0f));
    g.setColour (Palette::red);
    g.drawText ("O", 14, 12, 40, 50, Justification::centredLeft);
    g.setColour (Palette::keyCream);
    g.drawText ("pto-2", 44, 12, 180, 50, Justification::centredLeft);

    g.setFont (OptoLookAndFeel::gothic (18.0f));
    g.drawText ("amplificator opticus", 14, 84, 250, 24, Justification::centredLeft);

    // Rejilla del parlante
    auto grille = Rectangle<float> (274.0f, 0.0f, 114.0f, 120.0f);
    g.setColour (Palette::speaker);
    g.fillRect (grille);
    g.setColour (Colours::black.withAlpha (0.6f));
    const float step = 7.0f;
    for (float yy = grille.getY() + 6.0f; yy < grille.getBottom() - 3.0f; yy += step)
        for (float xx = grille.getX() + 6.0f; xx < grille.getRight() - 3.0f; xx += step)
            g.fillEllipse (xx - 2.0f, yy - 2.0f, 4.0f, 4.0f);
}

void Opto2Editor::paint (Graphics& g)
{
    // Chasis dorado oliva, sin marco
    auto b = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (Palette::bodyLight, 0.0f, 0.0f,
                                       Palette::bodyDark, 0.0f, b.getBottom(), false));
    g.fillAll();

    drawTopPanel (g);
    drawDisplay (g, displayArea);

    // Etiquetas de perillas (serigrafía gótica)
    g.setColour (Palette::ink);
    g.setFont (OptoLookAndFeel::gothic (17.0f));
    auto labelOver = [&] (const Slider& s, const char* txt)
    {
        g.drawText (txt, s.getX(), s.getY() - 22, s.getWidth(), 20, Justification::centred);
    };
    labelOver (peakKnob,   "reductio");
    labelOver (attackKnob, "impetus");
    labelOver (outputKnob, "exitus");

    // Fader
    g.drawText ("mixtura", 0, 232, 72, 20, Justification::centred);
    g.setFont (OptoLookAndFeel::gothic (15.0f));
    g.drawText ("siccus", 0, 448, 72, 16, Justification::centred);
    const float top = (float) mixFader.getY() + 12.0f, bot = (float) mixFader.getBottom() - 12.0f;
    for (int i = 0; i <= 10; ++i)
    {
        const float yy = top + (bot - top) * (float) i / 10.0f;
        const float len = (i % 5 == 0) ? 9.0f : 5.0f;
        g.drawLine (4.0f, yy, 4.0f + len, yy, 1.0f);
        g.drawLine (66.0f - len, yy, 66.0f, yy, 1.0f);
    }

    // Puntitos bajo las teclas, como en el aparato
    g.setColour (Palette::red);
    g.fillEllipse (100.0f, 452.0f, 5.0f, 5.0f);
    g.fillEllipse (248.0f, 452.0f, 5.0f, 5.0f);
    g.setColour (Palette::ink);
    g.setFont (OptoLookAndFeel::gothic (13.0f));
    g.drawText ("ratio tres",  110, 446, 120, 16, Justification::centredLeft);
    g.drawText ("ratio decem", 258, 446, 120, 16, Justification::centredLeft);

    // Pie
    g.setColour (Palette::ink.withAlpha (0.25f));
    g.drawLine (0.0f, 472.0f, b.getRight(), 472.0f, 1.0f);
    g.setColour (Palette::ink);
    g.setFont (OptoLookAndFeel::gothic (15.0f));
    g.drawText ("instrumentalis electronicum", 10, 480, 240, 20, Justification::centredLeft);
    g.drawText ("@gueibrielon", 230, 480, 148, 20, Justification::centredRight);
}

//==============================================================================
// Visor de 3 dígitos de 7 segmentos
//==============================================================================
namespace
{
    // bits: a=1 b=2 c=4 d=8 e=16 f=32 g=64
    int glyphFor (juce_wchar c)
    {
        switch (CharacterFunctions::toUpperCase (c))
        {
            case '0': return 63;  case '1': return 6;   case '2': return 91;  case '3': return 79;
            case '4': return 102; case '5': return 109; case '6': return 125; case '7': return 7;
            case '8': return 127; case '9': return 111; case '-': return 64;
            case 'A': return 119; case 'C': return 57;  case 'D': return 94;  case 'E': return 121;
            case 'I': return 6;   case 'K': return 117; case 'L': return 56;  case 'M': return 55;
            case 'O': return 63;  case 'P': return 115; case 'R': return 80;  case 'T': return 120;
            case 'U': return 62;  case 'X': return 118; case 'S': return 109;
            default:  return 0;
        }
    }

    void drawSegment (Graphics& g, Point<float> p1, Point<float> p2, float t)
    {
        const bool horiz = std::abs (p2.y - p1.y) < 0.01f;
        Path s;
        const float h = t * 0.5f;
        if (horiz)
        {
            s.startNewSubPath (p1.x, p1.y);
            s.lineTo (p1.x + h, p1.y - h); s.lineTo (p2.x - h, p2.y - h);
            s.lineTo (p2.x, p2.y);
            s.lineTo (p2.x - h, p2.y + h); s.lineTo (p1.x + h, p1.y + h);
        }
        else
        {
            s.startNewSubPath (p1.x, p1.y);
            s.lineTo (p1.x + h, p1.y + h); s.lineTo (p2.x + h, p2.y - h);
            s.lineTo (p2.x, p2.y);
            s.lineTo (p2.x - h, p2.y - h); s.lineTo (p1.x - h, p1.y + h);
        }
        s.closeSubPath();
        g.fillPath (s);
    }

    void drawDigit (Graphics& g, Rectangle<float> r, int mask, bool dp)
    {
        const float t  = r.getWidth() * 0.2f;
        const float gp = 1.6f;
        const float x0 = r.getX() + t * 0.5f, x1 = r.getRight() - t * 0.5f;
        const float y0 = r.getY() + t * 0.5f, ym = r.getCentreY(), y1 = r.getBottom() - t * 0.5f;

        const Point<float> seg[7][2] = {
            { { x0 + gp, y0 }, { x1 - gp, y0 } },   // a
            { { x1, y0 + gp }, { x1, ym - gp } },   // b
            { { x1, ym + gp }, { x1, y1 - gp } },   // c
            { { x0 + gp, y1 }, { x1 - gp, y1 } },   // d
            { { x0, ym + gp }, { x0, y1 - gp } },   // e
            { { x0, y0 + gp }, { x0, ym - gp } },   // f
            { { x0 + gp, ym }, { x1 - gp, ym } },   // g
        };

        for (int i = 0; i < 7; ++i)
        {
            const bool on = (mask >> i) & 1;
            if (on)
            {
                g.setColour (Palette::led.withAlpha (0.25f));        // halo
                drawSegment (g, seg[i][0], seg[i][1], t + 3.0f);
            }
            g.setColour (on ? Palette::led : Palette::led.withAlpha (0.07f));
            drawSegment (g, seg[i][0], seg[i][1], t);
        }

        g.setColour (dp ? Palette::led : Palette::led.withAlpha (0.07f));
        g.fillRect (r.getRight() + 3.0f, r.getBottom() - t, t, t);
    }
}

String Opto2Editor::valueText (int param) const
{
    const float v = lastValues[param];
    switch (param)
    {
        case 0:  return String (roundToInt (v));                                   // reductio 0–100
        case 1:  return v >= 99.95f ? String ("100") : String (v, 1);              // impetus ms
        case 2:  if (std::abs (v) < 0.05f) return "0.0";                           // exitus dB
                 return std::abs (v) < 9.95f ? String (v, 1) : String (roundToInt (v));
        case 3:  return String (roundToInt (v));                                   // mixtura %
        default: return lastValues[4] > 0.5f ? "LMT" : "CMP";
    }
}

String Opto2Editor::nameText (int param) const
{
    static const char* names[] = { "RED", "ATK", "OUT", "MIX" };
    if (param < 0)  return "OPT";
    if (param == 4) return lastValues[4] > 0.5f ? "LMT" : "CMP";
    return names[param];
}

void Opto2Editor::drawDisplay (Graphics& g, Rectangle<float> r)
{
    g.setColour (Palette::screen);
    g.fillRect (r);

    // Mientras se mueve: dígitos. 2 s después de soltar: letras del parámetro.
    const bool moving = activeParam >= 0 && activeParam != 4
                        && Time::getMillisecondCounterHiRes() - lastChangeMs < 2000.0;
    const String txt = moving ? valueText (activeParam) : nameText (activeParam);

    struct Cell { juce_wchar c = ' '; bool dp = false; };
    Cell cells[3];
    Array<Cell> parsed;
    for (auto ch : txt)
    {
        if (ch == '.' && ! parsed.isEmpty()) parsed.getReference (parsed.size() - 1).dp = true;
        else parsed.add ({ ch, false });
    }
    const int n = jmin (3, parsed.size());
    for (int i = 0; i < n; ++i)
        cells[3 - n + i] = parsed[parsed.size() - n + i];

    const float dw = 38.0f, dh = 64.0f, gap = 14.0f;
    float x = r.getX() + 22.0f;
    const float y = r.getCentreY() - dh * 0.5f;
    for (auto& c : cells)
    {
        drawDigit (g, { x, y, dw, dh }, glyphFor (c.c), c.dp);
        x += dw + gap;
    }

    // Medidor de reducción: escalera de LEDs a la derecha
    auto meter = Rectangle<float> (r.getX() + 196.0f, r.getY() + 30.0f, 172.0f, 12.0f);
    g.setFont (OptoLookAndFeel::mono (10.0f));
    g.setColour (Palette::led.withAlpha (0.8f));
    g.drawText ("GR", Rectangle<float> (meter.getX(), r.getY() + 12.0f, 40.0f, 14.0f).toNearestInt(), Justification::centredLeft);

    const int segs = 20;
    const float segW = meter.getWidth() / (float) segs;
    for (int i = 0; i < segs; ++i)
    {
        auto seg = Rectangle<float> (meter.getX() + segW * (float) i, meter.getY(), segW - 2.0f, meter.getHeight());
        g.setColour (shownGr >= (float) i + 0.5f ? Palette::led : Palette::led.withAlpha (0.08f));
        g.fillRect (seg);
    }
    g.setColour (Palette::led.withAlpha (0.55f));
    g.setFont (OptoLookAndFeel::mono (9.0f, false));
    for (int db : { 0, 5, 10, 20 })
    {
        const float xx = meter.getX() + segW * (float) db;
        g.drawText (String (db), Rectangle<float> (xx - 10.0f, meter.getBottom() + 3.0f, 20.0f, 12.0f).toNearestInt(),
                    db == 0 ? Justification::centredLeft : (db == 20 ? Justification::centredRight : Justification::centred));
    }

    // Indicadores de modo
    const bool limit = lastValues[4] > 0.5f;
    auto modeRow = Rectangle<float> (meter.getX(), r.getBottom() - 26.0f, meter.getWidth(), 14.0f);
    g.setFont (OptoLookAndFeel::gothic (14.0f));
    g.setColour (limit ? Palette::led.withAlpha (0.18f) : Palette::led);
    g.drawText ("compressio", modeRow.toNearestInt(), Justification::centredLeft);
    g.setColour (limit ? Palette::led : Palette::led.withAlpha (0.18f));
    g.drawText ("limes", modeRow.toNearestInt(), Justification::centredRight);
}
