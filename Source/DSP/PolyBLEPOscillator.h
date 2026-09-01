#pragma once
#include <cmath>
#include <algorithm>

class PolyBLEPOscillator {
public:
    PolyBLEPOscillator() = default;

    void setSampleRate(double sr) {
        sampleRate = sr;
    }

    void setFrequency(double freqHz) {
        frequency = freqHz;
        phaseIncrement = frequency / sampleRate;
    }

    void setWaveMorph(float morph) {
        waveMorph = std::clamp(morph, 0.0f, 1.0f);
    }

    void setPulseWidth(float pw) {
        pulseWidth = std::clamp(pw, 0.05f, 0.95f);
    }

    void reset() {
        phase = 0.0;
    }

    float process() {
        // PolyBLEP residual function for anti-aliasing steps
        auto polyBlep = [](double t, double dt) -> double {
            if (t < dt) {
                t /= dt;
                return t + t - t * t - 1.0;
            } else if (t > 1.0 - dt) {
                t = (t - 1.0) / dt;
                return t * t + t + t + 1.0;
            }
            return 0.0;
        };

        // 1. Naive Waveforms with PolyBLEP correction
        // Sawtooth
        double saw = (2.0 * phase) - 1.0;
        saw -= polyBlep(phase, phaseIncrement);

        // Pulse / Square
        double pulse = (phase < pulseWidth) ? 1.0 : -1.0;
        pulse += polyBlep(phase, phaseIncrement);
        pulse -= polyBlep(std::fmod(phase + (1.0 - pulseWidth), 1.0), phaseIncrement);

        // Triangle (Integrated Square)
        double tri = (phase < 0.5) ? (4.0 * phase - 1.0) : (3.0 - 4.0 * phase);

        // 2. Wave Morphing Logic:
        // 0.0 - 0.33 : Tri -> Saw
        // 0.33 - 0.66: Saw -> Square (50% PW)
        // 0.66 - 1.00: Square -> Pulse (with pulseWidth control)
        float output = 0.0f;
        if (waveMorph < 0.333f) {
            float blend = waveMorph / 0.333f;
            output = static_cast<float>((1.0 - blend) * tri + blend * saw);
        } else if (waveMorph < 0.666f) {
            float blend = (waveMorph - 0.333f) / 0.333f;
            // Square (fixed at 50% duty cycle)
            double square50 = (phase < 0.5) ? 1.0 : -1.0;
            square50 += polyBlep(phase, phaseIncrement);
            square50 -= polyBlep(std::fmod(phase + 0.5, 1.0), phaseIncrement);
            output = static_cast<float>((1.0 - blend) * saw + blend * square50);
        } else {
            float blend = (waveMorph - 0.666f) / 0.334f;
            double square50 = (phase < 0.5) ? 1.0 : -1.0;
            square50 += polyBlep(phase, phaseIncrement);
            square50 -= polyBlep(std::fmod(phase + 0.5, 1.0), phaseIncrement);
            output = static_cast<float>((1.0 - blend) * square50 + blend * pulse);
        }

        // Increment phase
        phase += phaseIncrement;
        if (phase >= 1.0) phase -= 1.0;

        return output;
    }

private:
    double sampleRate = 44100.0;
    double frequency = 440.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    float waveMorph = 0.0f;
    float pulseWidth = 0.5f;
};
