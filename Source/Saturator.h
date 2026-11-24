#pragma once
// Saturación del modo vis. Sin dependencias de JUCE: se puede testear aparte.
//
//  - tanh con un poco de sesgo (armónicos pares, como un sampler que se pasa de rosca),
//    normalizado para que la señal chica salga con ganancia 1: la saturación ensucia, no sube el volumen.
//  - Cuánto empuja lo decide vis (la perilla roja): +0 dB en 0, +10 dB en 100. Suavizado 20 ms.
//  - El sesgo deja continua: filtro de 10 Hz.

#include <cmath>
#include <algorithm>

class Saturator
{
public:
    static constexpr float minDriveDb = 0.0f, maxDriveDb = 10.0f;

    void prepare (double sampleRate)
    {
        const float sr = (float) sampleRate;
        smCoef = std::exp (-1.0f / (0.02f * sr));                     // 20 ms
        dcCoef = std::exp (-2.0f * 3.14159265358979f * 10.0f / sr);   // 10 Hz
        driveDbSm = driveDb;
        for (int c = 0; c < 2; ++c) dcX[c] = dcY[c] = 0.0f;
    }

    // 0..100 (la perilla roja en modo vis)
    void setAmount (float amount)
    {
        driveDb = minDriveDb + (maxDriveDb - minDriveDb) * std::clamp (amount, 0.0f, 100.0f) * 0.01f;
    }

    void process (float& left, float& right)
    {
        driveDbSm = driveDb + smCoef * (driveDbSm - driveDb);
        const float drive = std::pow (10.0f, driveDbSm / 20.0f);
        left  = dcBlock (0, saturate (left,  drive));
        right = dcBlock (1, saturate (right, drive));
    }

private:
    static float saturate (float x, float drive)
    {
        constexpr float bias = 0.15f;
        const float tb = std::tanh (bias);
        return (std::tanh (drive * x + bias) - tb) / (drive * (1.0f - tb * tb));
    }

    float dcBlock (int c, float x)
    {
        dcY[c] = x - dcX[c] + dcCoef * dcY[c];
        dcX[c] = x;
        return dcY[c];
    }

    float smCoef = 0.0f, dcCoef = 0.0f;
    float driveDb = minDriveDb, driveDbSm = minDriveDb;
    float dcX[2] {}, dcY[2] {};
};
