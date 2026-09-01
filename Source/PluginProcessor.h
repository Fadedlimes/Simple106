#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/VoiceManager.h"
#include "DSP/LFO.h"
#include "DSP/StereoChorus.h"
#include "DSP/StereoDelay.h"
#include "DSP/StereoReverb.h"
#include "DSP/SequencerEngine.h"
#include <vector>
#include <memory>

class Simple106AudioProcessor : public juce::AudioProcessor,
private juce::MidiInputCallback {
public:
    Simple106AudioProcessor();
    ~Simple106AudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Simple106"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

    bool isVoiceActive(int index) {
        if (index >= 0 && index < 6)
            return voiceManager.getVoices()[static_cast<size_t>(index)].isActive();
        return false;
    }

    std::array<SynthVoice, 6>& getVoices() {
        return voiceManager.getVoices();
    }

    std::vector<int> getChordNotes(int rootNote, int chordTypeIdx) const {
        return voiceManager.getChordIntervals(rootNote);
    }

    // Patch Management
    juce::File getPresetsDirectory() const;
    juce::String createPatchXml(const juce::String& patchName);
    void loadPatchXml(const juce::String& xmlString);
    void loadFactoryPreset(int presetIndex);

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    SequencerEngine sequencer;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    VoiceManager voiceManager;
    LFO lfo1;
    LFO lfo2;
    LFO lfo3;

    StereoChorus chorus;
    StereoDelay delay;
    StereoReverb reverb;

    std::vector<std::unique_ptr<juce::MidiInput>> activeMidiInputs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Simple106AudioProcessor)
};
