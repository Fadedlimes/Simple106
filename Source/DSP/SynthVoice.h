#pragma once
#include "PolyBLEPOscillator.h"
#include "JunoFilter.h"
#include "ADSREnvelope.h"
#include <cstdlib>

class SynthVoice {
public:
    void init(double sr) {
        sampleRate = sr;
        dco1.setSampleRate(sr);
        dco2.setSampleRate(sr);
        filter.setSampleRate(sr);
        ampEnv.setSampleRate(sr);
        filterEnv.setSampleRate(sr);
    }

    void setUnisonDetune(float detuneCents) { unisonDetuneCents = detuneCents; }
    void setVoiceTuneOffset(float cents)   { voiceTuneOffset = cents; }
    void setVoicePan(float pan)            { voicePan = std::clamp(pan, -1.0f, 1.0f); }

    void noteOn(int midiNote, float vel) {
        noteNumber = midiNote;
        velocity = vel;
        currentFreq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);

        targetFreq = currentFreq;
        if (!glideActive) {
            renderedFreq = currentFreq;
        }

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

    bool isActive() const { return ampEnv.isActive(); }
    int getNoteNumber() const { return noteNumber; }

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
            return;
        }

        // Portamento / Glide
        if (glideActive && std::abs(renderedFreq - targetFreq) > 0.01) {
            renderedFreq += (targetFreq - renderedFreq) * glideRate;
        } else {
            renderedFreq = targetFreq;
        }

        // Pitch Mod: LFO + Unison Detune + Per-Voice Microtune Offset
        double totalCents = unisonDetuneCents + voiceTuneOffset;
        double tuneRatio = std::pow(2.0, totalCents / 1200.0);
        double pitchMod = (1.0 + (lfo1Value * lfo1ToPitch * 0.05)) * tuneRatio;
        double baseFreq = renderedFreq * pitchMod;

        // DCO 1
        dco1.setFrequency(baseFreq);
        dco1.setWaveMorph(dco1Morph);
        dco1.setPulseWidth(dco1PWM);
        float sigDco1 = dco1.process() * dco1Level;

        // DCO 2
        double dco2Freq = baseFreq * std::pow(2.0, (dco2Semi + (dco2Cents / 100.0)) / 12.0);
        dco2.setFrequency(dco2Freq);
        dco2.setWaveMorph(dco2Morph);
        dco2.setPulseWidth(dco2PWM);
        float sigDco2 = dco2.process() * dco2Level;

        // Sub Oscillator (-1 Octave Square)
        subPhase += (baseFreq * 0.5) / sampleRate;
        if (subPhase >= 1.0) subPhase -= 1.0;
        float sigSub = ((subPhase < 0.5) ? 1.0f : -1.0f) * subLevel;

        // White Noise
        float sigNoise = (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * noiseLevel;

        // Balanced Analog Voice Mixer (Eliminates internal filter overloading)
        float voiceMix = (sigDco1 * 0.70f) + (sigDco2 * 0.70f) + (sigSub * 0.60f) + (sigNoise * 0.40f);

        float filterEnvVal = filterEnv.process();
        float ampEnvVal = ampEnv.process();

        float modulatedCutoff = lpfCutoff + (envModAmount * filterEnvVal * 8000.0f) + (lfo1Value * lfo1ToFilter * 4000.0f);
        modulatedCutoff = std::clamp(modulatedCutoff, 20.0f, static_cast<float>(sampleRate * 0.49));

        filter.setHPFCutoff(hpfCutoff);
        filter.setLPFParams(modulatedCutoff, lpfRes);

        float monoSignal = filter.process(voiceMix) * ampEnvVal * velocity;

        // Equal-Power Stereo Panning
        float panNorm = (voicePan + 1.0f) * 0.5f;
        outL = monoSignal * std::cos(panNorm * 1.5707963f);
        outR = monoSignal * std::sin(panNorm * 1.5707963f);
    }

    ADSREnvelope ampEnv;
    ADSREnvelope filterEnv;

private:
    double sampleRate = 44100.0;
    int noteNumber = -1;
    float velocity = 0.0f;
    bool active = false;

    float unisonDetuneCents = 0.0f;
    float voiceTuneOffset = 0.0f;
    float voicePan = 0.0f;

    double currentFreq = 440.0;
    double targetFreq = 440.0;
    double renderedFreq = 440.0;
    bool glideActive = false;
    float glideRate = 1.0f;

    PolyBLEPOscillator dco1;
    PolyBLEPOscillator dco2;
    double subPhase = 0.0;
    JunoFilter filter;
};
