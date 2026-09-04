#pragma once
#include <cmath>
#include <algorithm>

class PolyBLEPOscillator {
public:
    PolyBLEPOscillator() = default;

    void setSampleRate(double sr) {
        sampleRate = (sr > 1000.0) ? sr : 44100.0;
        updateIncrement();
    }

    void setFrequency(double freqHz) {
        frequency = std::max(1.0, freqHz);
        updateIncrement();
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

    // PolyBLEP residual function for anti-aliasing step discontinuities
    static inline double polyBlep(double t, double dt) {
        if (dt <= 1e-9) return 0.0;
        if (t < dt) {
            t /= dt;
            return t + t - t * t - 1.0;
        } else if (t > 1.0 - dt) {
            t = (t - 1.0) / dt;
            return t * t + t + t + 1.0;
        }
        return 0.0;
    }

    // Smooth diode-modeled West Coast wavefolder (pure rational math, zero transcendentals)
    static inline float applyWavefold(float in, float fold) {
        if (fold <= 0.001f) return in;

        float drive = 1.0f + fold * 3.5f;
        float x = in * drive;

        // Stage 1 fold
        if (x > 1.0f) {
            x = 2.0f - x;
        } else if (x < -1.0f) {
            x = -2.0f - x;
        }

        // Stage 2 fold for extreme settings
        if (x > 1.0f) {
            x = 2.0f - x;
        } else if (x < -1.0f) {
            x = -2.0f - x;
        }

        // Soft rational tanh saturation on folded peaks
        float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    // Standard process with phase wrap detection for master sync tracking
    float process(bool* didWrap = nullptr, double* wrapFrac = nullptr) {
        float output = static_cast<float>(computeSample(phase));

        phase += phaseIncrement;
        if (phase >= 1.0) {
            phase -= 1.0;
            if (didWrap != nullptr) *didWrap = true;
            if (wrapFrac != nullptr) *wrapFrac = (phaseIncrement > 1e-9) ? (phase / phaseIncrement) : 0.0;
        } else {
            if (didWrap != nullptr) *didWrap = false;
            if (wrapFrac != nullptr) *wrapFrac = 0.0;
        }

        return output;
    }

    // Anti-aliased slave hard-sync processing
    float processSynced(bool syncTriggered, double wrapFraction) {
        if (syncTriggered) {
            // Measure waveform voltage jump across the sync boundary
            double valBefore = computeSample(phase);
            phase = wrapFraction * phaseIncrement;
            if (phase >= 1.0) phase -= 1.0;
            double valAfter = computeSample(phase);

            double stepDiscontinuity = valAfter - valBefore;
            double blep = stepDiscontinuity * polyBlep(wrapFraction, phaseIncrement);

            float output = static_cast<float>(valAfter - blep);

            phase += phaseIncrement;
            if (phase >= 1.0) phase -= 1.0;
            return output;
        }

        return process();
    }

private:
    void updateIncrement() {
        phaseIncrement = frequency / sampleRate;
    }

    double computeSample(double p) const {
        // Sawtooth
        double saw = (2.0 * p) - 1.0;
        saw -= polyBlep(p, phaseIncrement);

        // Pulse / Square
        double pulse = (p < pulseWidth) ? 1.0 : -1.0;
        pulse += polyBlep(p, phaseIncrement);
        pulse -= polyBlep(std::fmod(p + (1.0 - pulseWidth), 1.0), phaseIncrement);

        // Triangle
        double tri = (p < 0.5) ? (4.0 * p - 1.0) : (3.0 - 4.0 * p);

        if (waveMorph < 0.333f) {
            float blend = waveMorph / 0.333f;
            return (1.0 - blend) * tri + blend * saw;
        } else if (waveMorph < 0.666f) {
            float blend = (waveMorph - 0.333f) / 0.333f;
            double square50 = (p < 0.5) ? 1.0 : -1.0;
            square50 += polyBlep(p, phaseIncrement);
            square50 -= polyBlep(std::fmod(p + 0.5, 1.0), phaseIncrement);
            return (1.0 - blend) * saw + blend * square50;
        } else {
            float blend = (waveMorph - 0.666f) / 0.334f;
            double square50 = (p < 0.5) ? 1.0 : -1.0;
            square50 += polyBlep(p, phaseIncrement);
            square50 -= polyBlep(std::fmod(p + 0.5, 1.0), phaseIncrement);
            return (1.0 - blend) * square50 + blend * pulse;
        }
    }

    double sampleRate = 44100.0;
    double frequency = 440.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    float waveMorph = 0.0f;
    float pulseWidth = 0.5f;
};