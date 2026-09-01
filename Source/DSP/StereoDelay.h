#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

class StereoDelay {
public:
    StereoDelay() {
        setSampleRate(44100.0);
    }

    void setSampleRate(double sr) {
        sampleRate = (sr > 1000.0) ? sr : 44100.0;
        maxDelaySamples = static_cast<int>(sampleRate * 2.0); // 2 seconds max
        bufferL.assign(maxDelaySamples, 0.0f);
        bufferR.assign(maxDelaySamples, 0.0f);
        writeIndex = 0;
    }

    void setParameters(float timeSec, float fb, float damp, float mixVal, bool pingPong) {
        delayTimeSec = std::clamp(timeSec, 0.01f, 1.95f);
        feedback = std::clamp(fb, 0.0f, 0.95f);
        damping = std::clamp(damp, 0.0f, 0.95f);
        mix = std::clamp(mixVal, 0.0f, 1.0f);
        isPingPong = pingPong;
    }

    void process(float inL, float inR, float& outL, float& outR) {
        if (mix <= 0.001f || maxDelaySamples <= 0) {
            outL = inL;
            outR = inR;
            return;
        }

        float delaySamples = delayTimeSec * static_cast<float>(sampleRate);
        float readPos = static_cast<float>(writeIndex) - delaySamples;
        while (readPos < 0.0f) readPos += static_cast<float>(maxDelaySamples);

        int i0 = static_cast<int>(readPos) % maxDelaySamples;
        int i1 = (i0 + 1) % maxDelaySamples;
        float frac = readPos - static_cast<float>(static_cast<int>(readPos));

        float delayedL = bufferL[i0] + frac * (bufferL[i1] - bufferL[i0]);
        float delayedR = bufferR[i0] + frac * (bufferR[i1] - bufferR[i0]);

        // Damping 1-pole lowpass
        filteredL = filteredL + (1.0f - damping) * (delayedL - filteredL);
        filteredR = filteredR + (1.0f - damping) * (delayedR - filteredR);

        // Feedback write
        if (isPingPong) {
            bufferL[writeIndex] = inL + (filteredR * feedback);
            bufferR[writeIndex] = inR + (filteredL * feedback);
        } else {
            bufferL[writeIndex] = inL + (filteredL * feedback);
            bufferR[writeIndex] = inR + (filteredR * feedback);
        }

        writeIndex = (writeIndex + 1) % maxDelaySamples;

        outL = inL * (1.0f - mix) + delayedL * mix;
        outR = inR * (1.0f - mix) + delayedR * mix;
    }

    void reset() {
        std::fill(bufferL.begin(), bufferL.end(), 0.0f);
        std::fill(bufferR.begin(), bufferR.end(), 0.0f);
        filteredL = filteredR = 0.0f;
    }

private:
    double sampleRate = 44100.0;
    int maxDelaySamples = 88200;
    int writeIndex = 0;
    std::vector<float> bufferL;
    std::vector<float> bufferR;

    float delayTimeSec = 0.35f;
    float feedback = 0.4f;
    float damping = 0.2f;
    float mix = 0.0f;
    bool isPingPong = true;
    float filteredL = 0.0f, filteredR = 0.0f;
};
