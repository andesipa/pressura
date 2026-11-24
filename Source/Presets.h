#pragma once
// Presets de fábrica. El índice 0 es el estado por defecto de todos los parámetros.
// Valores en las unidades de cada parámetro (ver createLayout): peak/speed/mix 0–100, output en dB.
// reductio calibrado para material a ~-6 dBFS de pico: leveler 3–5 dB de reducción promedio, glue 1–2 dB,
// limiter solo picos, sidechain ~8 dB por golpe. vis = drive + saturación. Después se ajusta a oído.
// Drum Bus Glue, Mix Bus Glue, Parallel Smash y Drum Bus Pump: calibrados con tools/bench/drum_bench.cpp contra
// modelos de 1176 / SSL de bus, igualados en loudness (ver CLAUDE.md).

struct Preset
{
    const char* name;
    float peak, speed, output, mix;
    bool  limit, vis, ext;
};

static constexpr Preset factoryPresets[] =
{
    //  nombre               peak speed  out    mix   limit  vis    ext
    { "Default",             40,  50,   0.0f,  100,  false, false, false },
    { "Vocal Leveler",       45,  45,   0.0f,  100,  false, false, false },
    { "Bass Leveler",        35,  40,   0.0f,  100,  false, false, false },
    { "Drum Bus Glue",       25,  80,   1.5f,  100,  false, false, false },
    { "Mix Bus Glue",        20,  80,  -1.0f,  100,  false, false, false },
    { "Parallel Smash",      40,  70,   2.0f,   30,  true,  false, false },
    { "Peak Limiter",        25,  95,   0.0f,  100,  true,  false, false },
    { "Drum Bus Pump",       60,  70,  -0.5f,  100,  false, true,  false },
    { "Crunch",              60,  50,  -2.0f,  100,  false, true,  false },
    { "Bass Sidechain",      30,  85,   0.0f,  100,  false, false, true  },
};

static constexpr int numFactoryPresets = (int) (sizeof (factoryPresets) / sizeof (factoryPresets[0]));
