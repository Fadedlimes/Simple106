#pragma once
#include <cmath>
#include <algorithm>

class ADSREnvelope {
public:
    enum class State { Idle, Attack, Decay, Sustain, Release };

    void setSampleRate(double sr) {
        sampleRate = sr;
    }

    void setParameters(float aSec, float dSec, float sLevel, float rSec) {
        attackTime = std::max(0.001f, aSec);
        decayTime = std::max(0.001f, dSec);
        sustainLevel = std::clamp(sLevel, 0.0f, 1.0f);
        releaseTime = std::max(0.001f, rSec);

        // Exponential coefficients
        attackRate = static_cast<float>(1.0 / (attackTime * sampleRate));
        decayCoeff = std::exp(-1.0f / (decayTime * static_cast<float>(sampleRate) * 0.35f));
        releaseCoeff = std::exp(-1.0f / (releaseTime * static_cast<float>(sampleRate) * 0.35f));
    }

    void noteOn() {
        state = State::Attack;
    }

    void noteOff() {
        if (state != State::Idle) {
            state = State::Release;
        }
    }

    void reset() {
        state = State::Idle;
        currentLevel = 0.0f;
    }

    bool isActive() const {
        return state != State::Idle;
    }

    float process() {
        switch (state) {
            case State::Idle:
                currentLevel = 0.0f;
                break;

            case State::Attack:
                currentLevel += attackRate;
                if (currentLevel >= 1.0f) {
                    currentLevel = 1.0f;
                    state = State::Decay;
                }
                break;

            case State::Decay:
                currentLevel = sustainLevel + (currentLevel - sustainLevel) * decayCoeff;
                if (std::abs(currentLevel - sustainLevel) < 0.0001f) {
                    currentLevel = sustainLevel;
                    state = State::Sustain;
                }
                break;

            case State::Sustain:
                currentLevel = sustainLevel;
                break;

            case State::Release:
                currentLevel *= releaseCoeff;
                if (currentLevel < 0.0001f) {
                    currentLevel = 0.0f;
                    state = State::Idle;
                }
                break;
        }
        return currentLevel;
    }

private:
    double sampleRate = 44100.0;
    State state = State::Idle;
    float attackTime = 0.01f;
    float decayTime = 0.3f;
    float sustainLevel = 0.7f;
    float releaseTime = 0.5f;

    float attackRate = 0.0f;
    float decayCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float currentLevel = 0.0f;
};
