#pragma once
#include "PolyBLEPOscillator.h"
#include "JunoFilter.h"
#include "ADSREnvelope.h"
#include <cmath>
#include <algorithm>
#include <cstdint>

class SynthVoice {
public:
    void init(double sr) {
        sampleRate = (sr > 1000.0) ? sr : 44100.0;
        dco1.setSampleRate(sampleRate);
        dco2.setSampleRate(sampleRate);
        filter.setSampleRate(sampleRate);
        ampEnv.setSampleRate(sampleRate);
        filterEnv.setSampleRate(sampleRate);
        noiseSeed = 123456789u;
        setVoicePan(0.0f);
        updateTuneRatio();
        antiClickOffsetL = 0.0f;
        antiClickOffsetR = 0.0f;
        antiClickStepL = 0.0f;
        antiClickStepR = 0.0f;
        lastOutL = 0.0f;
        lastOutR = 0.0f;
    }

    void setUnisonDetune(float detuneCents) {
        unisonDetuneCents = detuneCents;
        updateTuneRatio();
    }

    void setVoiceTuneOffset(float cents) {
        voiceTuneOffset = cents;
        updateTuneRatio();
    }

    void setVoicePan(float pan) {
        voicePan = std::clamp(pan, -1.0f, 1.0f);
        float panNorm = (voicePan + 1.0f) * 0.5f;
        panL = std::cos(panNorm * 1.57079632679f);
        panR = std::sin(panNorm * 1.57079632679f);
    }

    void noteOn(int midiNote, float vel) {
        // If this voice is currently sounding, apply instant step-compensation to eliminate clicks
        if (isActive() && ampEnv.getLevel() > 0.001f) {
            antiClickOffsetL = lastOutL;
            antiClickOffsetR = lastOutR;
            antiClickStepL = antiClickOffsetL / static_cast<float>(ANTI_CLICK_SAMPLES);
            antiClickStepR = antiClickOffsetR / static_cast<float>(ANTI_CLICK_SAMPLES);
        } else {
            antiClickOffsetL = 0.0f;
            antiClickOffsetR = 0.0f;
            antiClickStepL = 0.0f;
            antiClickStepR = 0.0f;
        }

        // Start note immediately: zero latency and noteOff can NEVER be missed
        noteNumber = midiNote;
        velocity = vel;
        currentFreq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
        targetFreq = currentFreq;
        if (!glideActive) {
            renderedFreq = currentFreq;
        }

        filter.reset();
        ampEnv.reset();
        filterEnv.reset();
        ampEnv.noteOn();
        filterEnv.noteOn();
        active = true;
    }

    void updateLegatoPitch(int midiNote, float vel) {
        noteNumber = midiNote;
        velocity = vel;
        currentFreq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
        targetFreq = currentFreq;
        if (!glideActive) {
            renderedFreq = currentFreq;
        }
        active = true;
    }

    void noteOff() {
        ampEnv.noteOff();
        filterEnv.noteOff();
    }

    bool isActive() const {
        return active && ampEnv.isActive();
    }

    int getNoteNumber() const {
        return noteNumber;
    }

    void setGlide(bool enable, float glideTimeSec) {
        glideActive = enable;
        glideRate = (glideTimeSec <= 0.001f) ? 1.0f : static_cast<float>(1.0 / (glideTimeSec * sampleRate));
    }

    void process(float dco1Morph, float dco1PWM, float dco1Level,
                 float dco2Morph, float dco2PWM, float dco2Level, int dco2Semi, float dco2Cents,
                 float subLevel, float noiseLevel,
                 float hpfCutoff, float lpfCutoff, float lpfRes, float envModAmount,
                 float lfo1Value, float lfo1ToFilter, float lfo1ToPitch,
                 float& outL, float& outR)
    {
        if (!isActive()) {
            outL = outR = 0.0f;
            lastOutL = lastOutR = 0.0f;
            antiClickOffsetL = antiClickOffsetR = 0.0f;
            active = false;
            return;
        }

        // Portamento / Glide
        if (glideActive && std::abs(renderedFreq - targetFreq) > 0.01) {
            renderedFreq += (targetFreq - renderedFreq) * glideRate;
        } else {
            renderedFreq = targetFreq;
        }

        // Pitch Mod: LFO + Unison Detune + Per-Voice Microtune Offset (precalculated tuneRatio)
        double pitchMod = (1.0 + (lfo1Value * lfo1ToPitch * 0.05)) * tuneRatio;
        double baseFreq = renderedFreq * pitchMod;

        // DCO 1
        dco1.setFrequency(baseFreq);
        dco1.setWaveMorph(dco1Morph);
        dco1.setPulseWidth(dco1PWM);
        float sigDco1 = dco1.process() * dco1Level;

        // DCO 2
        double dco2Ratio = std::pow(2.0, (dco2Semi + (dco2Cents / 100.0)) / 12.0);
        dco2.setFrequency(baseFreq * dco2Ratio);
        dco2.setWaveMorph(dco2Morph);
        dco2.setPulseWidth(dco2PWM);
        float sigDco2 = dco2.process() * dco2Level;

        // Sub Oscillator (-1 Octave Square with PolyBLEP anti-aliasing)
        double subInc = (baseFreq * 0.5) / sampleRate;
        subPhase += subInc;
        if (subPhase >= 1.0) subPhase -= 1.0;

        float subVal = (subPhase < 0.5) ? 1.0f : -1.0f;
        auto polyBlep = [](double t, double dt) -> float {
            if (t < dt) {
                double x = t / dt;
                return static_cast<float>(x + x - x * x - 1.0);
            } else if (t > 1.0 - dt) {
                double x = (t - 1.0) / dt;
                return static_cast<float>(x * x + x + x + 1.0);
            }
            return 0.0f;
        };
        subVal += polyBlep(subPhase, subInc);
        double subHalf = subPhase - 0.5;
        if (subHalf < 0.0) subHalf += 1.0;
        subVal -= polyBlep(subHalf, subInc);
        float sigSub = subVal * subLevel;

        // White Noise: Fast 32-bit lock-free LCG
        noiseSeed = noiseSeed * 196314165u + 907633515u;
        float sigNoise = (static_cast<float>(static_cast<int32_t>(noiseSeed)) * (1.0f / 2147483648.0f)) * noiseLevel;

        // Balanced Voice Mixer
        float voiceMix = (sigDco1 * 0.50f) + (sigDco2 * 0.50f) + (sigSub * 0.40f) + (sigNoise * 0.25f);

        float filterEnvVal = filterEnv.process();
        float ampEnvVal = ampEnv.process();

        float modulatedCutoff = lpfCutoff + (envModAmount * filterEnvVal * 8000.0f) + (lfo1Value * lfo1ToFilter * 4000.0f);
        filter.setHPFCutoff(hpfCutoff);
        filter.setLPFParams(modulatedCutoff, lpfRes);

        // Calibrated output level (+15% punch boost)
        float monoSignal = filter.process(voiceMix) * ampEnvVal * velocity * 0.72f;

        // Precalculated Panning with Anti-Click Step Compensation
        outL = (monoSignal * panL) + antiClickOffsetL;
        outR = (monoSignal * panR) + antiClickOffsetR;
        lastOutL = outL;
        lastOutR = outR;

        // Decay anti-click offset ramp smoothly to zero
        if (std::abs(antiClickOffsetL) > std::abs(antiClickStepL)) {
            antiClickOffsetL -= antiClickStepL;
        } else {
            antiClickOffsetL = 0.0f;
        }

        if (std::abs(antiClickOffsetR) > std::abs(antiClickStepR)) {
            antiClickOffsetR -= antiClickStepR;
        } else {
            antiClickOffsetR = 0.0f;
        }
    }

    ADSREnvelope ampEnv;
    ADSREnvelope filterEnv;

private:
    void updateTuneRatio() {
        double totalCents = unisonDetuneCents + voiceTuneOffset;
        tuneRatio = std::pow(2.0, totalCents / 1200.0);
    }

    static constexpr int ANTI_CLICK_SAMPLES = 48; // ~1.1ms at 44.1 kHz

    double sampleRate = 44100.0;
    int noteNumber = -1;
    float velocity = 0.0f;
    bool active = false;

    float unisonDetuneCents = 0.0f;
    float voiceTuneOffset = 0.0f;
    double tuneRatio = 1.0;
    float voicePan = 0.0f;
    float panL = 0.7071f;
    float panR = 0.7071f;

    float antiClickOffsetL = 0.0f;
    float antiClickOffsetR = 0.0f;
    float antiClickStepL = 0.0f;
    float antiClickStepR = 0.0f;
    float lastOutL = 0.0f;
    float lastOutR = 0.0f;

    double currentFreq = 440.0;
    double targetFreq = 440.0;
    double renderedFreq = 440.0;
    bool glideActive = false;
    float glideRate = 1.0f;

    PolyBLEPOscillator dco1;
    PolyBLEPOscillator dco2;
    double subPhase = 0.0;
    uint32_t noiseSeed = 123456789u;
    JunoFilter filter;
};