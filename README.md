# OPTO-2

Compresor óptico estilo LA-2A con UI basada en el EP-1320 medieval (paleta medida de la foto oficial). AU + VST3 + Standalone (JUCE 8).

## Controles
- **reductio** (perilla roja) = Peak Reduction: umbral -6 a -48 dB + makeup automático 0 a +9 dB.
- **impetus** (perilla marrón) = Attack: 0.5–100 ms. Centro = 10 ms (el ataque fijo del LA-2A).
- **exitus** (perilla crema) = Output: -18 a +18 dB, después de la mezcla.
- **mixtura** (fader vertical) = Mix dry/wet para compresión paralela.
- **compressio / limes** (teclas) = ratio ~3:1 o ~10:1.
- Visor de 3 dígitos: muestra el valor mientras movés un control y, 2 s después de soltarlo, el nombre (RED, ATK, OUT, MIX). Las teclas muestran CMP / LMT.
- Doble click en cualquier control = valor por defecto.

Release: de dos etapas y dependiente del programa (~50% suelta en ~60 ms, el resto en 1–3 s).

## Compilar en tu Mac
Necesitás Xcode (o Command Line Tools) y CMake (`brew install cmake`).

```bash
cd OPTO2
cmake -B build -G Xcode
cmake --build build --config Release
```

El build copia solo los plugins a `~/Library/Audio/Plug-Ins/Components` (AU) y `~/Library/Audio/Plug-Ins/VST3`.
En Ableton: Preferencias → Plug-ins → "Rescan".

Si macOS lo bloquea: `xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/OPTO-2.vst3`

## Archivos
- `Source/OptoCompressor.h` — DSP (sin JUCE, testeable aparte)
- `Source/PluginProcessor.*` — parámetros y procesamiento
- `Source/PluginEditor.*` — UI dibujada a mano (colores en `Palette`)

Fuente gótica: UnifrakturMaguntia (SIL Open Font License, ver `Resources/OFL.txt`), embebida en el plugin.
