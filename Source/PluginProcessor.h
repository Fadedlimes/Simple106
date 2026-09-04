#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "DSP/VoiceManager.h"
#include "DSP/LFO.h"
#include "DSP/StereoChorus.h"
#include "DSP/StereoDelay.h"
#include "DSP/StereoReverb.h"
#include "DSP/SequencerEngine.h"
#include <array>
#include <vector>
#include <memory>

class Simple106AudioProcessor : public juce::AudioProcessor,
                                private juce::MidiInputCallback
{
public:
    Simple106AudioProcessor();
    ~Simple106AudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Simple106"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;

    juce::File getPresetsDirectory() const;
    juce::String createPatchXml (const juce::String& patchName);
    void loadPatchXml (const juce::String& xmlString);
    void loadFactoryPreset (int presetIndex);

    bool isVoiceActive (int voiceIndex) const {
        if (voiceIndex >= 0 && voiceIndex < VoiceManager::NUM_VOICES)
            return voiceManager.getVoices()[voiceIndex].isActive();
        return false;
    }

    std::vector<int> getChordNotes (int root, int chordType) const {
        std::vector<int> notes;
        notes.reserve(4);
        notes.push_back(root);
        switch (chordType) {
            case 0: notes.push_back(root + 4); notes.push_back(root + 7); break;
            case 1: notes.push_back(root + 3); notes.push_back(root + 7); break;
            case 2: notes.push_back(root + 4); notes.push_back(root + 7); notes.push_back(root + 11); break;
            case 3: notes.push_back(root + 3); notes.push_back(root + 7); notes.push_back(root + 10); break;
            case 4: notes.push_back(root + 4); notes.push_back(root + 7); notes.push_back(root + 10); break;
            case 5: notes.push_back(root + 5); notes.push_back(root + 7); break;
            case 6: notes.push_back(root + 3); notes.push_back(root + 6); break;
            case 7: notes.push_back(root + 12); break;
            case 8: notes.push_back(root + 7); break;
            default: break;
        }
        return notes;
    }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    VoiceManager voiceManager;
    SequencerEngine sequencer;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    LFO lfo1;
    LFO lfo2;
    LFO lfo3;
    StereoChorus chorus;
    StereoDelay delay;
    StereoReverb reverb;

    juce::MidiMessageCollector midiCollector;
    std::vector<std::unique_ptr<juce::MidiInput>> activeMidiInputs;

    std::array<bool, 128> physicalKeysHeld;
    std::array<float, 128> physicalKeyVelocities;

    int lastArpPlayingNote = -1;
    int lastSeqPlayingNote = -1;
    bool lastHostPlaying = false;
    bool lastArpOn = false;
    bool lastChordOn = false;
    int lastChordType = 0;
    int lastPlayMode = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Simple106AudioProcessor)
};
