# Pressura

Compresor óptico estilo LA-2A con UI basada en el EP-1320 medieval (paleta medida de la foto oficial). AU + VST3 + Standalone (JUCE 8).

## Controles
- **reductio** (perilla roja) = Peak Reduction: umbral -6 a -48 dB + makeup automático 0 a +9 dB.
- **impetus** (perilla marrón) = Speed 0–100: mueve ataque, release y detector juntos. 50 = el opto original (ataque 10 ms). Abajo: más lento y suave (ataque ~30 ms). Arriba: se apaga la memoria óptica, el detector pasa a pico y bombea (ataque ~0.3 ms, release ~30 ms).
- **exitus** (perilla crema) = Output: -18 a +18 dB, después de la mezcla.
- **mixtura** (fader vertical) = Mix dry/wet para compresión paralela.
- **compressio / limes** (teclas) = ratio ~3:1 o ~10:1.
- **vis** (tecla en el panel verde) = modo de entrada. Apagada (CLN): la perilla roja es Peak Reduction. Encendida (VIS): la perilla roja pasa a **vis** = Drive, 0 a +24 dB contra un umbral fijo de -12 dB; lo fuerte queda igual y lo suave sube.
- Visor de 3 dígitos: muestra el valor mientras movés un control y, 2 s después de soltarlo, el nombre (RED o DRV, SPD, OUT, MIX). Las teclas muestran CMP / LMT y VIS / CLN.
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

Si macOS lo bloquea: `xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Pressura.vst3`

## Archivos
- `Source/OptoCompressor.h` — DSP (sin JUCE, testeable aparte)
- `Source/PluginProcessor.*` — parámetros y procesamiento
- `Source/PluginEditor.*` — UI dibujada a mano (colores en `Palette`)
- `Tests/Snapshot.cpp` — renderiza la UI a PNG sin abrir un DAW: `cmake --build build --target OPTO2Snapshot`, después `build/OPTO2Snapshot_artefacts/Release/OPTO2Snapshot ui.png [vis] [limit]`

Fuente gótica: UnifrakturMaguntia (SIL Open Font License, ver `Resources/OFL.txt`), embebida en el plugin.
