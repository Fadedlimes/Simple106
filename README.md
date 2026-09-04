# SIMPLE 106
### 6-Voice Programmable Analog-Modeling Synthesizer

![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20Windows-blue)
![Formats](https://img.shields.io/badge/Formats-VST3%20%7C%20AU%20%7C%20Standalone-orange)
![Version](https://img.shields.io/badge/Version-v1.1.2-green)
![License](https://img.shields.io/badge/License-GPLv3-lightgrey)

**Simple106** is a versatile 6-voice polyphonic synthesizer plugin and standalone instrument. Designed with the tactile aesthetic and immediacy of classic 80s/90s silver-box electronic hardware, it blends the analog warmth of digitally controlled oscillators, rich diode wavefolding, audio-rate cross-modulation, lush stereo BBD chorus, and discrete voice variation with modern performance tools like a 64-step sequencer, chord engine, and multi-mode arpeggiator.

<img width="920" height="639" alt="image" src="https://github.com/user-attachments/assets/6df81ea4-26aa-4e53-b818-b47371cc58d2" />

---

## 🎹 The Story Behind Simple106

**Simple106** was created to complete the "Simple" vintage instrument trio alongside **[Simple303](https://github.com/Fadedlimes/Simple303)** (monophonic acid bass synth) and **[Simple606](https://github.com/Fadedlimes/Simple606)** (analog drum machine).

While the 303 and 606 were modeled directly after existing hardware legends, no polyphonic silver-box sibling ever existed. Simple106 began as a fictional "what if" concept: *What if there was a compact, accessible polyphonic synthesizer that bedroom producers in the 90s could have picked up at a local pawn shop to accompany their basslines and rhythm machines?*

During development, the design quickly evolved far beyond a basic budget synth into an analog-modeling powerhouse drawing inspiration from multiple 80s and 90s polyphonic classics:
* **The Warmth & Grit:** Real-time PolyBLEP anti-aliased DCOs paired with a West Coast diode wavefolder and a rich, resonant 24dB 4-pole low-pass filter with high-pass stage.
* **The Inter-Oscillator Power:** Phase-aligned hard sync, audio-rate FM, and 4-quadrant analog ring modulation.
* **The Space:** Authentic dual-mode stereo bucket-brigade (BBD) chorus, ping-pong delay, and algorithmic plate/room reverb.
* **The Voice Matrix:** An Oberheim / Korg Mono/Poly-inspired voice variation system offering discrete per-voice panning, micro-pitch drift, and circular round-robin voice cycling.

---

## ✨ Features

### 🔊 Dual DCO Sound Engine (5×5 Panel Symmetry)
* **6 Polyphonic Voices** with true polyphony, legato glide portamento, and a stacked 6-voice **Unison** mode with supersaw detune spread.
* **Real-time PolyBLEP Anti-Aliasing:** Band-limited step and ramp residuals provide clean, high-frequency response with zero aliasing across the entire keyboard.
* **DCO 1 (Master / Core Oscillator):**
  * **Continuous Waveform Morphing:** Smoothly sweep through `Triangle` $\rightarrow$ `Sawtooth` $\rightarrow$ `Square` $\rightarrow$ `Variable Pulse`.
  * **Dedicated Pulse Width (PWM):** Variable pulse width ranging from `5%` to `95%`.
  * **Footage Selector:** 4-position stepped octave switch (`32'`, `16'`, `8'`, `4'`).
  * **Diode Wavefolder (`Fold`):** Diode-modeled West Coast folding circuit using rational non-linear saturation to inject woody, reedy, and acoustic acoustic harmonics.
  * **Level Slider:** Independent level attenuation (`0.0` to `1.0`).
* **DCO 2 (Slave / Modulator Oscillator):**
  * **Continuous Waveform Morphing:** Independent sweep through `Triangle` $\rightarrow$ `Sawtooth` $\rightarrow$ `Square` $\rightarrow$ `Variable Pulse`.
  * **Dedicated Pulse Width (PWM):** Variable pulse width ranging from `5%` to `95%`.
  * **Coarse Pitch Tuning:** Stepped pitch adjustment across $\pm24$ semitones ($\pm2$ octaves).
  * **Fine Detune:** Micro-tuning across $\pm50$ cents for lush beating pads and thick leads.
  * **Level Slider:** Independent level attenuation (`0.0` to `1.0`).

### ⚡ Oscillator Interaction Matrix (Sync & Cross-Modulation)
* **Anti-Aliased Hard Sync (`SYNC`):** Tactile push-button with active LED. When engaged, DCO 1 phase wraps trigger fractional sub-sample resets of DCO 2, smoothed via PolyBLEP correction.
* **Cross-Modulation (`X-MOD`):** Variable modulation depth (`0%` to `100%`) with selectable routing:
  * **`FM` (Frequency Modulation):** Audio-rate frequency modulation of DCO 2 by DCO 1 for metallic bells, FM basses, and aggressive timbres.
  * **`RING` (Ring Modulation):** Four-quadrant analog multiplier ($DCO 1 \times DCO 2$) blended seamlessly into DCO 2 for bell-like chimes and robotic sci-fi clangors.

### 🔉 Multi-Mode Sub-Oscillator & Noise Generator
* **Sub-Oscillator (`SUB`):** True PolyBLEP anti-aliased square wave locked to DCO 1.
  * **Sub Octave Switch:** Toggle instantly between **`-1 Octave`** for classic low-end reinforcement and **`-2 Octaves`** for earth-shaking sub-bass.
  * Dedicated level control.
* **White Noise Generator (`NOISE`):** 32-bit lock-free pseudo-random LCG noise source for vintage snare snaps, breathy flutes, and rushing sweeps.

### 🎛️ Filters & Envelopes
* **1-Pole High-Pass Filter (HPF):** Sweepable from 10 Hz to 1000 Hz to carve away muddy low-end build-up.
* **24dB/Octave Resonant Low-Pass Filter (VCF):** 2x oversampled 4-pole cascaded OTA filter model with rational saturation and self-oscillation capabilities.
* **Dual Exponential ADSR Envelopes:** True $T_{60}$ exponential envelopes for the Filter (with bipolar modulation amount) and Amplifier stage with instantaneous click-free voice voice retriggering.

### 🔄 Voice Variation Matrix (OB-X / Mono/Poly Style)
* **Discrete Voice Controls:** 6 dedicated hardware knobs for voices 1 through 6.
* **Panning Mode:** Set individual stereo pan locations for each voice to paint expansive, cinematic stereo fields.
* **Tuning Mode:** Introduce vintage component drift and micro-pitch offsets ($\pm50$ cents per voice) for organic analog instability.
* **Voice Cycling Mode (`CYCLE`):** When active, incoming notes rotate sequentially through voices $1 \rightarrow 2 \rightarrow 3 \rightarrow 4 \rightarrow 5 \rightarrow 6$, cycling different panning, tuning, and timbral variations on every keystroke.
* **Real-time Voice Status LEDs:** Hardware-style indicator lights monitor voice triggering and allocation in real time.

### 🎚️ Modulation Section (3 LFOs & 25 Mod Targets)
* **LFO 1 (Master Performance LFO):** 6 selectable waveforms (*Sine, Triangle, Saw Up, Saw Down, Square, S&H*) hardwired to Filter Cutoff and Oscillator Pitch.
* **LFO 2 & LFO 3 (Assignable Mod Matrix):** Independent speed, shape (*Sine, Triangle, S&H*), and depth routing to **25 distinct destinations**:
  * *LPF Cutoff, LPF Resonance, HPF Cutoff, Filter Env Mod*
  * *Filter Attack, Filter Decay, Filter Sustain, Filter Release*
  * *Amp Attack, Amp Decay, Amp Sustain, Amp Release*
  * *DCO 1 Morph, DCO 1 PWM, DCO 1 Fold*
  * *DCO 2 Morph, DCO 2 PWM, DCO 2 Semi, DCO 2 Detune*
  * *X-Mod Amount, Sub Level, Noise Level, Glide Time, Master Volume*

### 🎛️ Master FX Suite (Tab 2)
* **Stereo Chorus:** Classic analog-modeled BBD chorus unit (*Off, Chorus I, Chorus II, and rich Chorus I+II*).
* **Tempo-Synced Ping-Pong Delay:** 16 musical sync timings (`1/32` up to `1/1`, including triplets and dotted notes) plus Free ms mode, damping, feedback, and ping-pong stereo bouncing.
* **Algorithmic Reverb:** Room size, high-frequency damping, and mix for lush ambient spaces.

### 🎹 Performance, Sequencer & Arpeggiator
* **64-Step Sequencer (4 Pages × 16 Steps):** Step-record notes directly from the keyboard, enter rests, set custom sequence lengths, and navigate across 4 pages of 16 steps.
* **1-Finger Chord Mode:** Instant chord voicing across 9 chord types (*Major, Minor, Maj7, Min7, Dom7, Sus4, Diminished, Octave, Power 5th*).
* **Multi-Mode Arpeggiator:** *Up, Down, Up/Down, Random,* and *As-Played* modes synced to host tempo. **Works simultaneously with Chord Mode** to arpeggiate complex chords from a single held key.
* **Host DAW Sync:** Synchronizes sequencer clock and delay divisions seamlessly with your DAW's transport and BPM.
* **Integrated Button Keybed:** Two-octave tactile push-button keyboard with glowing LED indicators, octave transpose (`OCT -` / `OCT +`), and QWERTY musical typing with pointer focus lock.

### 🎨 Visual Themes & Hardware Customization
* **4 Distinct Chassis Themes:**
  * **Classic Silver:** Turned-aluminum faceplate with brushed metal textures.
  * **Midnight Blue:** Deep navy chassis with vintage blue-grey accents.
  * **Vintage Wood:** Warm walnut wooden end-cheeks and bronze paneling.
  * **Stealth Dark:** Matte-black stealth chassis with a top accent line that dynamically matches your chosen LED color palette.
* **7-Color Dynamic LED Theme Engine:** Switch all 14-segment digital displays, voice lights, button indicators, and keyboard LEDs between:
  * **Vintage Red**, **Neon Cyan**, **Acid Green**, **Amber Gold**, **Solar Yellow**, **Ultraviolet**, and **Ghost White**.
* **Theme Decoupling:** Chassis themes and LED palettes are preserved independently across preset and patch loading.

### 💾 Preset & Patch Management
* **Factory Preset Bank:** Includes classic factory presets (*Init Poly, Juno Warm Pad, Poly Drift Keys, Supersaw Hoover, Acid 106 Lead, 80s Arp Dream, MonoPoly 6-Voice Drift, Space Pluck*).
* **Portable Patch Format:** Store and share custom `.s106` XML patches containing both sound engine parameters and custom 64-step sequencer patterns.

---

## 🤖 AI Development Disclosure

In the spirit of open-source transparency:
While the overall concept, feature architecture, musical requirements, user interface design, testing, and creative vision were conceived and directed by Fadedlimes, the underlying C++ DSP code and JUCE framework implementation were developed in collaboration with conversational AI assistance.

---

## 📦 Installation & Formats

Download the latest pre-compiled binaries from the **[Releases](../../releases)** tab.

| Platform | Formats | Default Installation Directory |
| :--- | :--- | :--- |
| **Linux (x86_64)** | VST3, Standalone | `~/.vst3/` |
| **macOS (Universal - Apple Silicon & Intel)** | VST3, AU (Component), Standalone | `/Library/Audio/Plug-Ins/VST3/`<br>`/Library/Audio/Plug-Ins/Components/` |
| **Windows (x64)** | VST3, Standalone (.exe) | `C:\Program Files\Common Files\VST3\` |

---

## 🛠️ Building from Source

### Prerequisites
* **CMake** 3.20 or newer
* **C++17** compliant compiler (GCC 11+, Clang 12+, or MSVC 2019+)
* **Git**

### 🐧 Linux Build Instructions

#### Fedora / RHEL:
```bash
# Install development dependencies
sudo dnf install -y gcc-c++ cmake alsa-lib-devel libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel freetype-devel

# Clone and build
git clone https://github.com/Fadedlimes/Simple106.git
cd Simple106
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)

# Run Standalone
./build/Simple106_artefacts/Release/Standalone/Simple106
```

#### Ubuntu / Debian / Pop!_OS:
```bash
# Install development dependencies
sudo apt update
sudo apt install -y build-essential cmake libasound2-dev libjack-jackd2-dev libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype6-dev

# Clone and build
git clone https://github.com/Fadedlimes/Simple106.git
cd Simple106
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)

# Run Standalone
./build/Simple106_artefacts/Release/Standalone/Simple106
```

### 🍎 macOS Build Instructions
```bash
git clone https://github.com/Fadedlimes/Simple106.git
cd Simple106
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(sysctl -n hw.ncpu)
```

### 🪟 Windows Build Instructions (Visual Studio Developer Command Prompt)
```cmd
git clone https://github.com/Fadedlimes/Simple106.git
cd Simple106
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

---

## 📄 License

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**. See the [LICENSE](LICENSE) file for full details.
```
