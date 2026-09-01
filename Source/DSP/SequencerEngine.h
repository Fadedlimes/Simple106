#pragma once
#include <array>
#include <algorithm>
#include <cstdlib>

class SequencerEngine {
public:
    enum ArpMode { ArpUp = 0, ArpDown, ArpUpDown, ArpRandom, ArpAsPlayed };

    struct Step {
        int note = 48; // MIDI Pitch (C3 = 48)
        bool active = false;
        float velocity = 0.8f;
    };

    SequencerEngine() {
        for (int i = 0; i < 64; ++i) {
            steps[i].note = 48 + (i % 12);
            steps[i].active = (i % 4 == 0);
        }
        arpHeldNotes.fill(0);
        arpAsPlayedNotes.fill(0);
    }

    void setSampleRate(double sr) { sampleRate = sr; }
    void setTempo(float bpm) { currentBpm = std::clamp(bpm, 30.0f, 300.0f); }

    // --- CRASH-PROOF ZERO-ALLOCATION ARPEGGIATOR (32-NOTE CAPACITY) ---
    static constexpr int MAX_ARP_NOTES = 32;

    void setArpMode(int mode) {
        arpMode = static_cast<ArpMode>(std::clamp(mode, 0, 4));
    }

    int getNumArpHeld() const { return numArpHeld; }

    void addArpNote(int noteNumber) {
        for (int i = 0; i < numArpHeld; ++i) {
            if (arpHeldNotes[static_cast<size_t>(i)] == noteNumber) return; // Prevent duplicates
        }
        if (numArpHeld < MAX_ARP_NOTES) {
            arpHeldNotes[static_cast<size_t>(numArpHeld)] = noteNumber;
            arpAsPlayedNotes[static_cast<size_t>(numArpHeld)] = noteNumber;
            numArpHeld++;
        }
    }

    void removeArpNote(int noteNumber) {
        for (int i = 0; i < numArpHeld; ++i) {
            if (arpHeldNotes[static_cast<size_t>(i)] == noteNumber) {
                for (int j = i; j < numArpHeld - 1; ++j) {
                    arpHeldNotes[static_cast<size_t>(j)] = arpHeldNotes[static_cast<size_t>(j + 1)];
                }
                break;
            }
        }
        for (int i = 0; i < numArpHeld; ++i) {
            if (arpAsPlayedNotes[static_cast<size_t>(i)] == noteNumber) {
                for (int j = i; j < numArpHeld - 1; ++j) {
                    arpAsPlayedNotes[static_cast<size_t>(j)] = arpAsPlayedNotes[static_cast<size_t>(j + 1)];
                }
                break;
            }
        }
        if (numArpHeld > 0) {
            numArpHeld--;
        }
        if (numArpHeld == 0) {
            arpIndex = 0;
            arpDirectionUp = true;
        }
    }

    void clearArp() {
        numArpHeld = 0;
        arpIndex = 0;
        arpDirectionUp = true;
    }

    bool advanceArpClock(int numSamples, int& outNote, float& outVel, bool& outNoteOff) {
        outNoteOff = false;
        outNote = -1;

        if (!arpEnabled || numArpHeld <= 0) return false;

        double samplesPer16th = (sampleRate * 60.0) / (currentBpm * 4.0);
        arpSampleCounter += numSamples;

        if (arpSampleCounter >= samplesPer16th) {
            arpSampleCounter -= samplesPer16th;

            // Safe stack-allocated sort (zero heap allocations)
            std::array<int, MAX_ARP_NOTES> sortedNotes = arpHeldNotes;
            std::sort(sortedNotes.begin(), sortedNotes.begin() + numArpHeld);

            switch (arpMode) {
                case ArpUp:
                    arpIndex = (arpIndex + 1) % numArpHeld;
                    outNote = sortedNotes[static_cast<size_t>(arpIndex)];
                    break;

                case ArpDown:
                    arpIndex = (arpIndex - 1 + numArpHeld) % numArpHeld;
                    outNote = sortedNotes[static_cast<size_t>(arpIndex)];
                    break;

                case ArpUpDown:
                    if (numArpHeld <= 1) {
                        outNote = sortedNotes[0];
                    } else {
                        if (arpDirectionUp) {
                            arpIndex++;
                            if (arpIndex >= numArpHeld - 1) {
                                arpIndex = numArpHeld - 1;
                                arpDirectionUp = false;
                            }
                        } else {
                            arpIndex--;
                            if (arpIndex <= 0) {
                                arpIndex = 0;
                                arpDirectionUp = true;
                            }
                        }
                        outNote = sortedNotes[static_cast<size_t>(arpIndex)];
                    }
                    break;

                case ArpRandom:
                    outNote = sortedNotes[static_cast<size_t>(std::rand() % numArpHeld)];
                    break;

                case ArpAsPlayed:
                    arpIndex = (arpIndex + 1) % numArpHeld;
                    outNote = arpAsPlayedNotes[static_cast<size_t>(arpIndex)];
                    break;
            }

            outVel = 0.85f;
            return (outNote >= 0);
        }
        return false;
    }

    // --- 64-STEP SEQUENCER ---
    int getNumSteps() const { return sequenceLength; }
    int getNumPages() const { return numPages; }

    void setNumPages(int pages) {
        numPages = std::clamp(pages, 1, 4);
        sequenceLength = numPages * 16;
        if (currentStep >= sequenceLength) currentStep = 0;
        if (recordStep >= sequenceLength) recordStep = 0;
    }

    int getCurrentStep() const { return currentStep; }
    int getRecordStep() const { return recordStep; }

    Step& getStep(int index) { return steps[std::clamp(index, 0, 63)]; }
    const Step& getStep(int index) const { return steps[std::clamp(index, 0, 63)]; }

    void recordNote(int pitch) {
        if (recordStep >= 0 && recordStep < sequenceLength) {
            steps[recordStep].note = pitch;
            steps[recordStep].active = true;
            recordStep = (recordStep + 1) % sequenceLength;
        }
    }

    void recordRest() {
        if (recordStep >= 0 && recordStep < sequenceLength) {
            steps[recordStep].active = false;
            recordStep = (recordStep + 1) % sequenceLength;
        }
    }

    void clearPattern() {
        for (int i = 0; i < 64; ++i) {
            steps[i].active = false;
            steps[i].note = 48;
        }
        recordStep = 0;
        currentStep = 0;
    }

    void setRecordStep(int step) {
        recordStep = std::clamp(step, 0, sequenceLength - 1);
    }

    bool advanceStepClock(int numSamples, int& outTriggerNote, float& outTriggerVel, bool& outNoteOff) {
        outNoteOff = false;
        outTriggerNote = -1;

        if (!isPlaying) return false;

        double samplesPer16th = (sampleRate * 60.0) / (currentBpm * 4.0);
        sampleCounter += numSamples;

        if (sampleCounter >= samplesPer16th) {
            sampleCounter -= samplesPer16th;
            currentStep = (currentStep + 1) % sequenceLength;

            if (steps[currentStep].active) {
                outTriggerNote = steps[currentStep].note;
                outTriggerVel = steps[currentStep].velocity;
                return true;
            } else {
                outNoteOff = true;
            }
        }
        return false;
    }

    bool isPlaying = false;
    bool isRecording = false;
    bool arpEnabled = false;

    int currentStep = 0;
    int recordStep = 0;
    int numPages = 4;
    int sequenceLength = 64;

private:
    double sampleRate = 44100.0;
    float currentBpm = 120.0f;
    double sampleCounter = 0.0;
    std::array<Step, 64> steps;

    ArpMode arpMode = ArpUp;
    double arpSampleCounter = 0.0;
    int arpIndex = 0;
    int numArpHeld = 0;
    bool arpDirectionUp = true;
    std::array<int, MAX_ARP_NOTES> arpHeldNotes {};
    std::array<int, MAX_ARP_NOTES> arpAsPlayedNotes {};
};
