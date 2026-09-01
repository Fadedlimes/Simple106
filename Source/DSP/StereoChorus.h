#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

class StereoChorus {
public:
    enum Mode { Off = 0, Chorus1, Chorus2, Chorus1And2 };

    StereoChorus() {
        setSampleRate(44100.0);
        setMode(Chorus1);
    }

    void setSampleRate(double sr) {
        sampleRate = (sr > 1000.0) ? sr : 44100.0;
        bufferSize = static_cast<int>(sampleRate * 0.1); // 100ms safe buffer
        if (bufferSize < 1024) bufferSize = 1024;

        delayBufferL.assign(static_cast<size_t>(bufferSize), 0.0f);
        delayBufferR.assign(static_cast<size_t>(bufferSize), 0.0f);
        writeIndex = 0;
    }

    void setMode(int m) {
        mode = static_cast<Mode>(std::clamp(m, 0, 3));
        switch (mode) {
            case Chorus1:
                lfoRate1 = 0.513f; // Juno Chorus I rate (~0.5 Hz)
                depth1 = 0.0018f;  // 1.8 ms depth
                baseDelay = 0.0035f;
                break;
            case Chorus2:
                lfoRate1 = 0.863f; // Juno Chorus II rate (~0.86 Hz)
                depth1 = 0.0028f;  // 2.8 ms depth
                baseDelay = 0.0035f;
                break;
            case Chorus1And2:
                lfoRate1 = 1.0f;
                depth1 = 0.0035f;
                baseDelay = 0.0035f;
                break;
            default:
                break;
        }
    }

    void process(float inL, float inR, float& outL, float& outR) {
        if (mode == Off || delayBufferL.empty() || delayBufferR.empty()) {
            outL = inL;
            outR = inR;
            return;
        }

        writeIndex = writeIndex % bufferSize;
        if (writeIndex < 0) writeIndex = 0;

        delayBufferL[static_cast<size_t>(writeIndex)] = inL;
        delayBufferR[static_cast<size_t>(writeIndex)] = inR;

        // LFO for modulation (anti-phase for wide stereo)
        float lfo = std::sin(2.0f * 3.14159265f * static_cast<float>(lfoPhase));
        lfoPhase += lfoRate1 / sampleRate;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;

        float delaySecondsL = baseDelay + depth1 * lfo;
        float delaySecondsR = baseDelay + depth1 * (-lfo);

        float delayedL = readInterpolated(delayBufferL, delaySecondsL);
        float delayedR = readInterpolated(delayBufferR, delaySecondsR);

        outL = 0.707f * (inL + delayedL);
        outR = 0.707f * (inR + delayedR);

        writeIndex = (writeIndex + 1) % bufferSize;
    }

private:
    float readInterpolated(const std::vector<float>& buffer, float delaySeconds) {
        if (buffer.empty() || bufferSize <= 0) return 0.0f;

        float delaySamples = delaySeconds * static_cast<float>(sampleRate);
        float readPos = static_cast<float>(writeIndex) - delaySamples;

        while (readPos < 0.0f) readPos += static_cast<float>(bufferSize);
        while (readPos >= static_cast<float>(bufferSize)) readPos -= static_cast<float>(bufferSize);

        int i0 = static_cast<int>(readPos) % bufferSize;
        if (i0 < 0) i0 = 0;
        int i1 = (i0 + 1) % bufferSize;
        float frac = readPos - static_cast<float>(static_cast<int>(readPos));

        return buffer[static_cast<size_t>(i0)] + frac * (buffer[static_cast<size_t>(i1)] - buffer[static_cast<size_t>(i0)]);
    }

    double sampleRate = 44100.0;
    int bufferSize = 4096;
    int writeIndex = 0;
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;

    Mode mode = Chorus1;
    double lfoPhase = 0.0;
    float lfoRate1 = 0.5f;
    float depth1 = 0.002f;
    float baseDelay = 0.004f;
};
