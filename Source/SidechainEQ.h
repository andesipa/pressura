#pragma once
// EQ de tres bandas sobre la señal que escucha el detector (no sobre el audio).
// Sin dependencias de JUCE: se puede testear aparte.
//
//  - Cortes Linkwitz-Riley de 4.º orden en 200 Hz y 1 kHz:
//      gravis < 200 Hz · medius 200 Hz – 1 kHz · acutus > 1 kHz
//  - La banda grave pasa por un allpass en 1 kHz para quedar en fase con las otras dos,
//    así con las tres en 0 dB la suma es plana.
//  - Con las tres en 0 dB la señal sale sin tocar (el compresor suena igual que sin EQ).
//  - Ganancia por banda -24 .. +12 dB; en -24 la banda se apaga.

#include <cmath>
#include <algorithm>

class SidechainEQ
{
public:
    static constexpr float minDb = -24.0f, maxDb = 12.0f;

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        for (auto& c : ch)
        {
            setPair (c.lp200, lowpass  (200.0f));
            setPair (c.hp200, highpass (200.0f));
            setPair (c.lp1k,  lowpass  (1000.0f));
            setPair (c.hp1k,  highpass (1000.0f));
            setPair (c.apLp,  lowpass  (1000.0f));
            setPair (c.apHp,  highpass (1000.0f));
        }
        smCoef = std::exp (-1.0f / (0.02f * sr));   // 20 ms
        for (int b = 0; b < 3; ++b) gain[b] = target[b];
    }

    void setGainsDb (float lowDb, float midDb, float highDb)
    {
        const float db[3] = { lowDb, midDb, highDb };
        flat = true;
        for (int b = 0; b < 3; ++b)
        {
            target[b] = db[b] <= minDb + 0.05f ? 0.0f : std::pow (10.0f, std::clamp (db[b], minDb, maxDb) / 20.0f);
            flat = flat && std::abs (db[b]) < 1.0e-3f;
        }
    }

    // Filtra el par estéreo del detector en el lugar
    void process (float& left, float& right)
    {
        bool settled = true;
        for (int b = 0; b < 3; ++b)
        {
            gain[b] = target[b] + smCoef * (gain[b] - target[b]);
            settled = settled && std::abs (gain[b] - target[b]) < 1.0e-4f;
        }

        // Los filtros corren siempre para no arrancar con estado viejo al mover una perilla
        const float l = split (ch[0], left), r = split (ch[1], right);
        if (flat && settled) return;
        left = l;
        right = r;
    }

private:
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        float process (float x)
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };
    struct Lr4  // dos Butterworth de 2.º orden en cascada = Linkwitz-Riley de 4.º
    {
        Biquad a, b;
        float process (float x) { return b.process (a.process (x)); }
    };
    struct Channel { Lr4 lp200, hp200, lp1k, hp1k, apLp, apHp; };

    float split (Channel& c, float x)
    {
        const float lowRaw = c.lp200.process (x);
        const float rest   = c.hp200.process (x);
        const float low    = c.apLp.process (lowRaw) + c.apHp.process (lowRaw);   // allpass en 1 kHz
        const float mid    = c.lp1k.process (rest);
        const float high   = c.hp1k.process (rest);
        return gain[0] * low + gain[1] * mid + gain[2] * high;
    }

    // Coeficientes RBJ, Q = 1/√2
    Biquad lowpass (float fc) const  { return rbj (fc, true); }
    Biquad highpass (float fc) const { return rbj (fc, false); }
    Biquad rbj (float fc, bool lp) const
    {
        const float w = 2.0f * 3.14159265358979f * fc / sr;
        const float cw = std::cos (w), alpha = std::sin (w) / (2.0f * 0.70710678f);
        const float a0 = 1.0f + alpha;
        Biquad q;
        q.b0 = (lp ? (1.0f - cw) : (1.0f + cw)) * 0.5f / a0;
        q.b1 = (lp ? (1.0f - cw) : -(1.0f + cw)) / a0;
        q.b2 = q.b0;
        q.a1 = -2.0f * cw / a0;
        q.a2 = (1.0f - alpha) / a0;
        return q;
    }
    static void setPair (Lr4& f, const Biquad& q) { f.a = q; f.b = q; }

    float sr = 44100.0f, smCoef = 0.0f;
    float target[3] { 1.0f, 1.0f, 1.0f }, gain[3] { 1.0f, 1.0f, 1.0f };
    bool  flat = true;
    Channel ch[2];
};
