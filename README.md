# Pressura

*amplificator opticus* — an optical-style compressor with a one-knob speed control and a drive mode.
VST3 · AU · Standalone · Windows 10/11 (64-bit) · macOS 11+ (Intel and Apple Silicon)

[Leer en español](README.es.md)

<p align="center"><img src="docs/pressura.png" width="388" alt="Pressura panel"></p>

Pressura starts from the smooth, program-dependent behaviour of a classic optical leveller (think LA-2A)
and lets you push it somewhere else: turn **impetus** up and it becomes a fast, pumping bus compressor;
press **vis** and the red knob stops lowering a threshold and starts *driving* the signal into one.

---

## Installing

Download the latest installer from the **Releases** page.

### Windows

1. Download **`Pressura-<version>-Windows-Setup.exe`** and double-click it.
2. Windows will say *"Windows protected your PC"*, because Pressura isn't signed with a paid certificate.
   Click **More info** → **Run anyway**. This only happens once.
3. Follow the installer. It puts the VST3 in `C:\Program Files\Common Files\VST3` and the Standalone app in
   `C:\Program Files\Pressura`, with a Start menu shortcut.
4. Rescan plugins in your DAW. It appears as **Pressura** by **gueibrielon**.

**Uninstall:** *Settings → Apps → Pressura → Uninstall*.

### macOS

1. Download **`Pressura-<version>.pkg`** and double-click it.
2. macOS will say it can't verify the developer, because Pressura isn't signed with a paid Apple account.
   This only happens once:
   - **macOS 15 Sequoia or newer:** click **Done**, open **System Settings → Privacy & Security**, scroll down
     to *"Pressura-….pkg was blocked"* and click **Open Anyway**.
   - **macOS 14 or older:** right-click (or Control-click) the .pkg, choose **Open**, then **Open** again.
3. Follow the installer. It puts the plugins here (choose **Customize** to leave any out):
   - Audio Unit → `/Library/Audio/Plug-Ins/Components` (Logic, GarageBand, Ableton…)
   - VST3 → `/Library/Audio/Plug-Ins/VST3` (Ableton, Reaper, Bitwig, Cubase…)
   - Standalone app → `/Applications`
4. Rescan plugins in your DAW (Ableton: *Settings → Plug-Ins → Rescan*). It appears as **Pressura** by **gueibrielon**.

No Terminal needed: plugins installed by the .pkg are not blocked by macOS.

**Uninstall:** delete `Pressura.component`, `Pressura.vst3` and `Pressura.app` from the folders above.

---

## The panel

| Control | What it does | Range | Default |
|---|---|---|---|
| **reductio** (red knob) | How much it compresses. Lowers the threshold and adds automatic make-up gain, so the level stays roughly the same while the dynamics get smaller. | 0–100 (threshold −6 to −48 dB, make-up 0 to +9 dB) | 40 |
| **vis** (same red knob, when the **vis** key is on) | Drive. Pushes the signal into a fixed −12 dB threshold. Loud parts stay about where they were, quiet parts come up: denser and louder. | 0–100 (0 to +24 dB of drive) | 40 |
| **impetus** (brown knob) | Speed. Moves attack, release and the detector together. See below. | 0–100 | 50 |
| **exitus** (cream knob) | Output level, after everything else. | −18 to +18 dB | 0 dB |
| **mixtura** (fader) | Dry/wet mix for parallel compression. Top = all compressed, bottom (*siccus*, "dry") = untouched. | 0–100 % | 100 % |
| **compressio** key | Ratio about 3:1 with a soft 8 dB knee. Musical, for levelling. | | on |
| **limes** key | Ratio about 10:1 with a 3 dB knee. For catching peaks or squashing. | | off |
| **vis** key (top panel) | Switches the red knob between **reductio** (clean) and **vis** (drive). The knob's label changes with it. | | off |

**Double-click** any knob or the fader to return it to its default.

### impetus (speed)

| impetus | Attack | Release | Character |
|---|---|---|---|
| 0 | ~30 ms | slow (~150 ms + the optical tail) | Very smooth, almost transparent levelling |
| **50** | **~10 ms** | **~60 ms + 1–3 s optical tail** | **The classic optical sound** |
| 75 | ~1.7 ms | ~40 ms, shorter tail | Tighter, transients get caught |
| 100 | ~0.3 ms | ~30 ms, no tail | Fast, peak-sensing, pumps with the beat |

Up to 50, Pressura behaves like an optical cell: it responds to the average level (RMS) and has a two-stage
release. Half the gain reduction lets go quickly; the rest takes seconds to recover. The longer it has been
compressing, the slower the tail. Above 50 that slow "memory" fades out and the detector switches from
average to peak level, so the compressor follows each hit and pumps.

### The display

- At rest it shows **PRS**.
- While you move a control it shows the value. **2 seconds** after you let go, it shows the control's name:
  **RED** (reductio) or **DRV** (vis), **SPD** (impetus), **OUT** (exitus), **MIX** (mixtura).
- The keys show **CMP** / **LMT** and **VIS** / **CLN** when pressed.
- **GR** meter: how much the level is being reduced right now, 0 to 20 dB. The *compressio* / *limes* and
  **VIS** indicators below and above it show the current modes.

---

## How it works

```mermaid
flowchart LR
    IN([Input]) --> DRV["vis drive<br/>0 to +24 dB<br/>(only when vis is on)"]
    DRV --> DET["Detector<br/>stereo-linked<br/>RMS → peak with impetus"]
    DET --> GC["Gain computer<br/>threshold · ratio · knee<br/>compressio 3:1 / limes 10:1"]
    GC --> REL["Attack / two-stage release<br/>set by impetus"]
    REL --> VCA["Gain + make-up"]
    DRV --> VCA
    VCA --> MIX["mixtura<br/>dry / wet"]
    IN -- dry --> MIX
    MIX --> OUT["exitus<br/>output level"] --> O([Output])
```

- **Stereo-linked:** both channels get the same gain reduction, so the stereo image doesn't move.
- **Make-up gain:** in clean mode, raising *reductio* adds up to +9 dB, so you mostly hear the dynamics
  change rather than the volume. In *vis* mode the make-up is −drive ÷ ratio: whatever is far above the
  threshold comes out about as loud as it went in, and everything quieter gets louder.
- **Dry signal** in the *mixtura* blend is always your original input, never driven.
- No latency, no oversampling, no lookahead.

---

## Starting points

| Use | Mode | Red knob | impetus | Ratio | mixtura |
|---|---|---|---|---|---|
| Vocal or bass levelling | clean | reductio 40–60 | 40–50 | compressio | 100 % |
| Mix bus glue | clean | reductio 15–30 | 30–50 | compressio | 100 % |
| Drum bus that pumps | **vis** | vis 50–70 | 80–100 | compressio | 60–80 % |
| Parallel smash | clean | reductio 80–100 | 70–90 | limes | 20–40 % |
| Loud and dense | **vis** | vis 60–90 | 50–70 | limes | 100 %, lower exitus |

With *vis* high and *impetus* low, transients get through before the compressor reacts and can hit 0 dBFS.
Pull **exitus** down or put a limiter after Pressura.

---

## Building from source

You need the Command Line Tools (`xcode-select --install`) and CMake 3.22 or newer
(from [cmake.org](https://cmake.org/download/) or `brew install cmake` on Apple Silicon). JUCE 8 is downloaded
automatically the first time you configure.

```bash
cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

With Xcode installed you can use `cmake -B build -G Xcode && cmake --build build --config Release` instead.
The build is universal (arm64 + x86_64) and copies the AU and VST3 straight into `~/Library/Audio/Plug-Ins`.
Validate the AU with `auval -v aufx Opt2 Guei`.

**Windows:** Visual Studio 2022 ("Desktop development with C++") and CMake, then
`cmake -B build -G "Visual Studio 17 2022" -A x64` and `cmake --build build --config Release`.
The Windows installer is built with [Inno Setup](https://jrsoftware.org/isinfo.php): `iscc /DAppVersion=<version> tools\installer\pressura.iss`.

**Releases:** pushing a tag like `v0.1.0` makes GitHub Actions build both installers and publish them
(`.github/workflows/release.yml`).

To build the macOS installer: `./tools/make_installer.sh` → `build/installer/Pressura-<version>.pkg`.
The script explains how to sign and notarize it if you have an Apple Developer account.

### Project files
- `Source/OptoCompressor.h` — the DSP, no JUCE dependency, can be tested on its own
- `Source/PluginProcessor.*` — parameters and audio processing
- `Source/PluginEditor.*` — the hand-drawn UI (colours in `Palette`)
- `Tests/Snapshot.cpp` — renders the UI to a PNG without a DAW:
  `cmake --build build --target OPTO2Snapshot`, then
  `build/OPTO2Snapshot_artefacts/Release/OPTO2Snapshot ui.png [vis] [limit]`

---

Blackletter font: UnifrakturMaguntia, SIL Open Font License (`Resources/OFL.txt`), embedded in the plugin.
Made by @gueibrielon.
