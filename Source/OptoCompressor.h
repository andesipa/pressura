#pragma once
// OPTO-2 — modelo simplificado de compresor óptico estilo LA-2A.
// Sin dependencias de JUCE: se puede testear aparte.
//
// Rasgos del modelo:
//  - Detector RMS corto (la celda óptica responde a energía, no a picos).
//  - Curva con knee suave; modo COMP ~3:1, modo LIMIT ~10:1.
//  - Release de dos etapas, dependiente del programa: ~50% de la reducción
//    se libera rápido (~60 ms) y el resto lento (1–3 s), más lento cuanto
//    más tiempo estuvo comprimiendo (la "memoria" de la celda T4).
//  - Ataque variable (en el LA-2A original es fijo en ~10 ms).
//  - Makeup automático ligado a Peak Reduction.

#include <cmath>
#include <algorithm>

class OptoCompressor
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        rmsCoef      = timeToCoef (5.0f);     // ventana del detector
        fastRelCoef  = timeToCoef (60.0f);    // etapa rápida del release
        slowChgCoef  = timeToCoef (1200.0f);  // carga de la "memoria"
        slowRelCoef  = timeToCoef (2500.0f);  // etapa lenta del release
        setAttackMs (attackMs);
        reset();
    }

    void reset()
    {
        rmsState = 0.0f;
        grFast = grSlow = 0.0f;
    }

    // 0..100, como el knob del LA-2A
    void setPeakReduction (float pr)
    {
        peakReduction = std::clamp (pr, 0.0f, 100.0f);
        thresholdDb   = -6.0f - peakReduction * 0.42f;   // -6 dB .. -48 dB
        makeupDb      = peakReduction * 0.09f;           // 0 .. +9 dB
    }

    void setAttackMs (float ms)
    {
        attackMs  = std::clamp (ms, 0.5f, 100.0f);
        attackCoef = timeToCoef (attackMs);
    }

    void setLimitMode (bool shouldLimit)
    {
        ratio  = shouldLimit ? 10.0f : 3.0f;
        kneeDb = shouldLimit ? 3.0f  : 8.0f;
    }

    // Recibe el sidechain (estéreo enlazado) y devuelve la ganancia lineal a aplicar.
    float process (float left, float right)
    {
        const float x2 = std::max (left * left, right * right);
        rmsState = x2 + rmsCoef * (rmsState - x2);
        const float levelDb = 10.0f * std::log10 (rmsState + 1.0e-12f);

        const float target = staticGainReductionDb (levelDb);

        // Etapa rápida: ataque variable, release ~60 ms
        if (target > grFast) grFast = target + attackCoef  * (grFast - target);
        else                 grFast = target + fastRelCoef * (grFast - target);

        // Etapa lenta: se carga despacio bajo compresión sostenida, suelta en segundos
        if (target > grSlow) grSlow = target + slowChgCoef * (grSlow - target);
        else                 grSlow = target + slowRelCoef * (grSlow - target);

        currentGrDb = std::max (grFast, 0.5f * grSlow);
        return dbToGain (makeupDb - currentGrDb);
    }

    float getGainReductionDb() const noexcept { return currentGrDb; }

private:
    float staticGainReductionDb (float levelDb) const
    {
        const float over = levelDb - thresholdDb;
        const float slope = 1.0f - 1.0f / ratio;
        if (2.0f * over <= -kneeDb) return 0.0f;
        if (2.0f * over >=  kneeDb) return slope * over;
        const float k = over + kneeDb * 0.5f;
        return slope * k * k / (2.0f * kneeDb);
    }

    float timeToCoef (float ms) const { return std::exp (-1.0f / (0.001f * ms * sr)); }
    static float dbToGain (float db)  { return std::pow (10.0f, db / 20.0f); }

    float sr = 44100.0f;
    float peakReduction = 40.0f, thresholdDb = -22.8f, makeupDb = 3.6f;
    float attackMs = 10.0f, attackCoef = 0.0f;
    float ratio = 3.0f, kneeDb = 8.0f;
    float rmsCoef = 0.0f, fastRelCoef = 0.0f, slowChgCoef = 0.0f, slowRelCoef = 0.0f;
    float rmsState = 0.0f, grFast = 0.0f, grSlow = 0.0f, currentGrDb = 0.0f;
};
