# SIMPLE 106
### 6-Voice Programmable Analog-Modeling Synthesizer

![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20Windows-blue)
![Formats](https://img.shields.io/badge/Formats-VST3%20%7C%20AU%20%7C%20Standalone-orange)
![Version](https://img.shields.io/badge/Version-v1.0.1-green)
![License](https://img.shields.io/badge/License-GPLv3-lightgrey)

**Simple106** is a versatile 6-voice polyphonic synthesizer plugin and standalone instrument. Designed with the tactile aesthetic and immediacy of classic 80s/90s silver-box electronic hardware, it blends the analog warmth of digitally controlled oscillators, lush chorus effects, and per-voice variation with modern performance tools like a 64-step sequencer, chord engine, and multi-mode arpeggiator.
<img width="916" height="639" alt="image" src="https://github.com/user-attachments/assets/f5827dcb-fd9b-4fcc-89d5-05c679de05ef" />


---

## 🎹 The Story Behind Simple106

**Simple106** was created to complete the "Simple" vintage instrument trio alongside **[Simple303](https://github.com/Fadedlimes/Simple303)** (monophonic acid bass synth) and **[Simple606](https://github.com/Fadedlimes/Simple606)** (analog drum machine).

While the 303 and 606 were modeled directly after existing hardware legends, no polyphonic silver-box sibling ever existed. Simple106 began as a fictional "what if" concept: *What if there was a compact, accessible polyphonic synthesizer that bedroom producers in the 90s could have picked up at a local pawn shop to accompany their basslines and rhythm machines?*

During development, the design quickly evolved far beyond a basic budget synth into a hybrid synthesizer taking inspiration from multiple 80s and 90s polyphonic classics:
* **The Warmth & Grit:** Real-time PolyBLEP DCOs paired with a rich, resonant 24dB 4-pole low-pass filter and high-pass stage.
* **The Space:** Authentic dual-mode stereo bucket-brigade (BBD) chorus.
* **The Voice Matrix:** An Oberheim / Korg Mono/Poly-inspired voice variation system offering discrete per-voice panning, micro-pitch drift, and circular round-robin voice cycling.

---

## ✨ Features

### 🔊 Sound Engine & Oscillators
* **6 Polyphonic Voices** with full polyphony, monophonic legato glide, and a stacked 6-voice **Unison** mode with supersaw detune spread.
* **Dual DCOs per Voice:** Real-time, sample-accurate **PolyBLEP** anti-aliased waveform generation.
* **Continuous Waveform Morphing:** Seamlessly sweep between **Triangle $\rightarrow$ Sawtooth $\rightarrow$ Square $\rightarrow$ Variable Pulse**.
* **Pulse Width Modulation (PWM):** Dedicated pulse width depth and modulation controls.
* **DCO 2 Tuning:** Coarse tuning ($\pm24$ semitones) and fine detuning ($\pm50$ cents).
* **Mixer Section:** Blend DCO 1, DCO 2, Sub-Oscillator ($-1$ octave square wave), and White Noise.

### 🎛️ Filters & Envelopes
* **High-Pass Filter (HPF):** 1-pole high-pass stage to clean up low-end rumble.
* **24dB/Octave Resonant Low-Pass Filter (VCF):** 4-pole cascaded OTA filter model with analog saturation and self-oscillation capabilities.
* **Dual Analog-Curved ADSR Envelopes:** Independent exponential envelopes for the Filter (with bipolar envelope modulation amount) and VCA amplifier stage.

### 🔄 Voice Variation Matrix (OB-Xf / Mono/Poly Style)
* **Discrete Voice Controls:** 6 dedicated knobs for voices 1 through 6.
* **Panning Mode:** Set individual stereo pan positions for each voice to create ultra-wide stereo pads.
* **Tuning Mode:** Dial in vintage analog component drift and micro-pitch offsets ($\pm50$ cents per voice).
* **Voice Cycling Mode (`CYCLE`):** When engaged, incoming notes cycle sequentially through voices 1 $\rightarrow$ 6, rotating timbres and stereo positions on every single note.
* **Real-time Voice Status LEDs:** Hardware-style status lights show active voices as they trigger.

### 🎚️ Modulation Section (3 LFOs)
* **LFO 1 (Master):** 6 waveforms (Sine, Triangle, Saw Up, Saw Down, Square, S&H) hard-routed to Filter Cutoff and Oscillator Pitch.
* **LFO 2 & LFO 3 (Assignable Mod Matrix):** Selectable Sine, Triangle, or Sample & Hold waveforms with independent speed, depth, and a **22-destination target router** (modulate Cutoff, Resonance, HPF, Attack, Decay, Sustain, Release, Morph, PWM, Pitch, Detune, Sub, Noise, or Volume).

### 🎛️ Master FX Rack (Tab 2)
* **Stereo Chorus:** Classic BBD chorus unit with Off, Mode I, Mode II, and fast vibrato Mode I+II.
* **Tempo-Synced Ping-Pong Delay:** Musical beat divisions (`1/32` to `1/1`, dotted and triplets, plus Free ms mode), feedback, high-frequency damping, and stereo ping-pong bounce.
* **Lush Algorithmic Reverb:** Room size, high-frequency damping, and wet/dry mix.

### 🎹 Performance, Sequencer & Arpeggiator
* **64-Step Sequencer (4 Pages × 16 Steps):** Step-record notes directly from the keyboard, enter rests, set custom sequence lengths, and navigate through 4 pages of 16 steps.
* **1-Finger Chord Mode:** Instant chord generation across 9 chord types (*Major, Minor, Maj7, Min7, Dom7, Sus4, Diminished, Octave, 5th/Power*).
* **Multi-Mode Arpeggiator:** *Up, Down, Up/Down, Random,* and *As-Played* modes synced to tempo. **Works simultaneously with Chord Mode** to arpeggiate complex chords from a single key.
* **Host DAW Sync:** Automatically synchronizes sequencer playback and delay timings to your DAW's transport and BPM.
* **Integrated Button Keybed:** Two-octave push-button keyboard with glowing note indicators, octave transposition switches (`OCT -` / `OCT +`), and QWERTY musical typing.

### 🎨 Retro UI & Visual Themes
* **Brushed Silver Chassis:** Vintage metal styling with custom turned-aluminum knobs and tactile beveled push-buttons.
* **Four 14-Segment Digital LED Displays:** Authentic red glowing digital screens for Arp, Chord, Steps, and Page indicators.
* **7-Color LED Theme Engine:** Switch all LEDs and digital screens between **Vintage Red**, **Neon Cyan**, **Acid Green**, **Amber Gold**, **Solar Yellow**, **Ultraviolet**, and **Ghost White**.
* **High-Performance 60 FPS Render Engine:** Fully hardware-cached vector rendering for fluid, lag-free UI interaction and window dragging.

### 💾 Preset & Patch Management
* **Built-in Factory Presets:** Comes loaded with 8 classic factory sounds (*Init Poly, Juno Warm Pad, Poly Drift Keys, Supersaw Hoover, Acid 106 Lead, 80s Arp Dream, MonoPoly Drift, Space Pluck*).
* **Save, Load & Share:** Patches are stored as portable `.s106` XML files in your Documents folder, capturing both synth parameters and custom 64-step sequencer patterns.

---
🤖 AI Development Disclosure

In the spirit of open-source transparency:
While the overall concept, feature architecture, musical requirements, user interface design, testing, and creative vision were conceived and directed by Fadedlimes, the underlying C++ DSP code and JUCE framework implementation were developed in collaboration with conversational AI assistance.

## 📦 Installation & Formats

Download the latest pre-compiled binaries from the **[Releases](../../releases)** tab.

| Platform | Formats | Installation Location |
| :--- | :--- | :--- |
| **Linux (x86_64)** | VST3, Standalone | `~/.vst3/` |
| **macOS (Universal - Apple Silicon & Intel)** | VST3, AU (Component), Standalone | `/Library/Audio/Plug-Ins/VST3/`<br>`/Library/Audio/Plug-Ins/Components/` |
| **Windows (x64)** | VST3, Standalone (.exe) | `C:\Program Files\Common Files\VST3\` |

---

## 🛠️ Building from Source

### Prerequisites
* **CMake** 3.20 or newer
* **C++17** compliant compiler (GCC 9+, Clang 10+, or MSVC 2019+)
* **Git**

#### Linux Build Dependencies (Ubuntu/Debian/Pop!_OS):
```bash
sudo apt update
sudo apt install -y build-essential cmake libasound2-dev libjack-jackd2-dev libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype6-dev
