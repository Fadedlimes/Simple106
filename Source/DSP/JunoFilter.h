#pragma once
#include <cmath>
#include <algorithm>

class JunoFilter {
public:
    void setSampleRate(double sr) {
        sampleRate = sr;
    }

    void setHPFCutoff(float cutoffHz) {
        hpfCutoff = std::clamp(cutoffHz, 10.0f, 2000.0f);
        // 1-pole high-pass coefficient
        float omega = 2.0f * 3.14159265f * (hpfCutoff / static_cast<float>(sampleRate));
        hpfCoeff = 1.0f / (1.0f + omega);
    }

    void setLPFParams(float cutoffHz, float resonance) {
        lpfCutoff = std::clamp(cutoffHz, 20.0f, static_cast<float>(sampleRate * 0.49));
        lpfResonance = std::clamp(resonance, 0.0f, 1.0f);
    }

    float process(float input) {
        // --- 1. High-Pass Filter Stage (1-pole) ---
        float hpfOut = hpfCoeff * (hpfPrevOut + input - hpfPrevIn);
        hpfPrevIn = input;
        hpfPrevOut = hpfOut;

        // --- 2. Low-Pass Filter Stage (4-pole cascade with resonance feedback) ---
        // Normalized frequency tuning calculation
        float f = (2.0f * lpfCutoff) / static_cast<float>(sampleRate);
        f = std::clamp(f, 0.0f, 0.99f);

        // Feedback gain based on resonance (scaled for self-oscillation)
        float k = 4.0f * lpfResonance;

        // Saturation in the feedback loop to replicate analog OTA clipping
        float feedback = s4 - (hpfOut * 0.5f);
        float saturatedInput = std::tanh(hpfOut - k * feedback);

        // 4 cascaded 1-pole stages
        s1 += f * (saturatedInput - s1);
        s2 += f * (s1 - s2);
        s3 += f * (s2 - s3);
        s4 += f * (s3 - s4);

        return s4;
    }

    void reset() {
        s1 = s2 = s3 = s4 = 0.0f;
        hpfPrevIn = hpfPrevOut = 0.0f;
    }

private:
    double sampleRate = 44100.0;
    float hpfCutoff = 20.0f;
    float hpfCoeff = 0.0f;
    float hpfPrevIn = 0.0f;
    float hpfPrevOut = 0.0f;

    float lpfCutoff = 1000.0f;
    float lpfResonance = 0.0f;
    float s1 = 0.0f, s2 = 0.0f, s3 = 0.0f, s4 = 0.0f;
};
