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
//  - Speed (0–100) mueve ataque, release rápido y detector juntos, como un
//    compresor de bus de una sola perilla. En 50 es exactamente el opto de arriba;
//    por debajo es más lento y suave; por encima se apaga la memoria óptica,
//    el detector pasa de RMS a pico y el compresor bombea.
//  - Dos modos de entrada:
//      clean: Peak Reduction baja el umbral y sube un makeup automático.
//      vis:   Drive empuja la señal contra un umbral fijo (más denso y más fuerte).

#include <cmath>
#include <algorithm>

class OptoCompressor
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        slowChgCoef  = timeToCoef (1200.0f);  // carga de la "memoria"
        slowRelCoef  = timeToCoef (2500.0f);  // etapa lenta del release
        peakRelCoef  = timeToCoef (3.0f);    // caída del detector de pico
        gainSmCoef   = timeToCoef (20.0f);    // suavizado de drive / makeup
        updateTimes();
        reset();
    }

    void reset()
    {
        rmsState = peakState = 0.0f;
        grFast = grSlow = 0.0f;
        inGainDbSm = inGainDb;
        makeupDbSm = makeupDb;
    }

    // 0..100: Peak Reduction en modo clean, Drive en modo vis
    void setPeakReduction (float pr)
    {
        peakReduction = std::clamp (pr, 0.0f, 100.0f);
        updateStatic();
    }

    // 0..100. 50 = el opto original (ataque 10 ms, release rápido 60 ms)
    void setSpeed (float s)
    {
        s = std::clamp (s, 0.0f, 100.0f) * 0.01f;
        if (std::abs (s - speed) < 1.0e-6f) return;
        speed = s;
        updateTimes();
    }

    void setLimitMode (bool shouldLimit)
    {
        ratio  = shouldLimit ? 10.0f : 3.0f;
        kneeDb = shouldLimit ? 3.0f  : 8.0f;
        updateStatic();
    }

    void setDriveMode (bool shouldDrive)
    {
        driveMode = shouldDrive;
        updateStatic();
    }

    // Recibe el sidechain (estéreo enlazado) y devuelve la ganancia lineal a aplicar.
    float process (float left, float right)
    {
        inGainDbSm = inGainDb + gainSmCoef * (inGainDbSm - inGainDb);
        makeupDbSm = makeupDb + gainSmCoef * (makeupDbSm - makeupDb);
        const float inGain2 = dbToPower (inGainDbSm);

        const float x2 = std::max (left * left, right * right) * inGain2;
        rmsState  = x2 + rmsCoef * (rmsState - x2);
        peakState = std::max (x2, peakRelCoef * peakState);

        const float rmsDb  = 10.0f * std::log10 (rmsState  + 1.0e-12f);
        const float peakDb = 10.0f * std::log10 (peakState + 1.0e-12f);
        const float levelDb = rmsDb + peakBlend * (peakDb - rmsDb);

        const float target = staticGainReductionDb (levelDb);

        // Etapa rápida: ataque y release según Speed
        if (target > grFast) grFast = target + attackCoef  * (grFast - target);
        else                 grFast = target + fastRelCoef * (grFast - target);

        // Etapa lenta: se carga despacio bajo compresión sostenida, suelta en segundos
        if (target > grSlow) grSlow = target + slowChgCoef * (grSlow - target);
        else                 grSlow = target + slowRelCoef * (grSlow - target);

        currentGrDb = std::max (grFast, slowWeight * grSlow);
        return dbToGain (inGainDbSm + makeupDbSm - currentGrDb);
    }

    float getGainReductionDb() const noexcept { return currentGrDb; }

private:
    void updateStatic()
    {
        if (driveMode)
        {
            inGainDb    = peakReduction * 0.24f;      // 0 .. +24 dB contra el umbral
            thresholdDb = -12.0f;
            makeupDb    = -inGainDb / ratio;          // lo fuerte queda igual, lo suave sube
        }
        else
        {
            inGainDb    = 0.0f;
            thresholdDb = -6.0f - peakReduction * 0.42f;   // -6 dB .. -48 dB
            makeupDb    = peakReduction * 0.09f;           // 0 .. +9 dB
        }
    }

    void updateTimes()
    {
        attackCoef  = timeToCoef (sweep (30.0f, 10.0f, 0.3f));
        fastRelCoef = timeToCoef (sweep (150.0f, 60.0f, 30.0f));
        rmsCoef     = timeToCoef (sweep (8.0f, 5.0f, 0.5f));
        slowWeight  = 0.5f * std::clamp (2.0f - 2.0f * speed, 0.0f, 1.0f);
        peakBlend   = std::clamp (2.0f * speed - 1.0f, 0.0f, 1.0f);
    }

    // Interpolación logarítmica: speed 0 -> slow, 0.5 -> mid, 1 -> fast
    float sweep (float slow, float mid, float fast) const
    {
        return speed < 0.5f ? slow * std::pow (mid / slow, 2.0f * speed)
                            : mid  * std::pow (fast / mid, 2.0f * speed - 1.0f);
    }

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
    static float dbToPower (float db) { return std::pow (10.0f, db / 10.0f); }

    float sr = 44100.0f;
    float peakReduction = 40.0f, thresholdDb = -22.8f, makeupDb = 3.6f, inGainDb = 0.0f;
    float inGainDbSm = 0.0f, makeupDbSm = 3.6f;
    bool  driveMode = false;
    float speed = 0.5f, slowWeight = 0.5f, peakBlend = 0.0f;
    float ratio = 3.0f, kneeDb = 8.0f;
    float attackCoef = 0.0f, rmsCoef = 0.0f, fastRelCoef = 0.0f, slowChgCoef = 0.0f, slowRelCoef = 0.0f;
    float peakRelCoef = 0.0f, gainSmCoef = 0.0f;
    float rmsState = 0.0f, peakState = 0.0f, grFast = 0.0f, grSlow = 0.0f, currentGrDb = 0.0f;
};
