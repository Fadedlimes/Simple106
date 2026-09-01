#pragma once
#include <juce_dsp/juce_dsp.h>
#include <algorithm>

class StereoReverb {
public:
    StereoReverb() = default;

    void prepare(double sampleRate, int samplesPerBlock) {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
        spec.numChannels = 2;
        reverb.prepare(spec);
    }

    void setParameters(float roomSize, float damping, float mixVal) {
        juce::dsp::Reverb::Parameters p;
        p.roomSize = std::clamp(roomSize, 0.0f, 1.0f);
        p.damping = std::clamp(damping, 0.0f, 1.0f);
        p.wetLevel = std::clamp(mixVal, 0.0f, 1.0f);
        p.dryLevel = 1.0f - p.wetLevel;
        p.width = 1.0f;
        p.freezeMode = 0.0f;
        reverb.setParameters(p);
        mix = mixVal;
    }

    void process(float& inL, float& inR) {
        if (mix <= 0.001f) return;

        float* channelPointers[2] = { &inL, &inR };
        juce::dsp::AudioBlock<float> block(channelPointers, 2, 1);
        juce::dsp::ProcessContextReplacing<float> context(block);
        reverb.process(context);
    }

    void reset() {
        reverb.reset();
    }

private:
    juce::dsp::Reverb reverb;
    float mix = 0.0f;
};
