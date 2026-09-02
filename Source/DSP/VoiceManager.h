#pragma once
#include "SynthVoice.h"
#include <array>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdint>

class VoiceManager {
public:
    static constexpr int NUM_VOICES = 6;
    enum PlayMode { Poly = 0, Mono, Unison };
    enum ChordType { Major = 0, Minor, Maj7, Min7, Dom7, Sus4, Diminished, Octave, Power5th };

    struct NoteEvent {
        int noteNumber;
        float velocity;
    };

    void init(double sampleRate) {
        for (auto& voice : voices) {
            voice.init(sampleRate);
        }
        voiceAge.fill(0);
        keyOffAge.fill(0);
        isKeyHeld.fill(false);
        currentAgeCounter = 0;
        cycleVoiceIndex = 0;
        lastMonoVoice = 0;
        activeChords.clear();
        monoNoteStack.clear();
        unisonNoteStack.clear();
    }

    void setPlayMode(int mode) {
        playMode = static_cast<PlayMode>(mode);
        isKeyHeld.fill(false);
        monoNoteStack.clear();
        unisonNoteStack.clear();
    }

    void setCycleMode(bool enable) {
        cycleMode = enable;
    }

    void setChordMode(bool enable, int type) {
        chordEnabled = enable;
        chordType = static_cast<ChordType>(type);
    }

    std::vector<int> getChordIntervals(int root) const {
        std::vector<int> notes;
        notes.push_back(root);
        switch (chordType) {
            case Major:      notes.push_back(root + 4); notes.push_back(root + 7); break;
            case Minor:      notes.push_back(root + 3); notes.push_back(root + 7); break;
            case Maj7:       notes.push_back(root + 4); notes.push_back(root + 7); notes.push_back(root + 11); break;
            case Min7:       notes.push_back(root + 3); notes.push_back(root + 7); notes.push_back(root + 10); break;
            case Dom7:       notes.push_back(root + 4); notes.push_back(root + 7); notes.push_back(root + 10); break;
            case Sus4:       notes.push_back(root + 5); notes.push_back(root + 7); break;
            case Diminished: notes.push_back(root + 3); notes.push_back(root + 6); break;
            case Octave:     notes.push_back(root + 12); break;
            case Power5th:   notes.push_back(root + 7); break;
        }
        return notes;
    }

    void handleNoteOn(int noteNumber, float velocity, bool glideEnabled, float glideTime) {
        // --- 1. MONOPHONIC MODE ---
        if (playMode == Mono) {
            if (cycleMode) {
                voices[lastMonoVoice].noteOff();
                isKeyHeld[lastMonoVoice] = false;

                int v = cycleVoiceIndex;
                cycleVoiceIndex = (cycleVoiceIndex + 1) % NUM_VOICES;
                lastMonoVoice = v;

                voices[v].setGlide(glideEnabled, glideTime);
                voices[v].setUnisonDetune(0.0f);
                voices[v].noteOn(noteNumber, velocity);
                isKeyHeld[v] = true;
                return;
            }

            monoNoteStack.erase(
                std::remove_if(monoNoteStack.begin(), monoNoteStack.end(),
                               [noteNumber](const NoteEvent& e) { return e.noteNumber == noteNumber; }),
                                monoNoteStack.end());

            bool isLegato = !monoNoteStack.empty();
            monoNoteStack.push_back({ noteNumber, velocity });

            voices[0].setUnisonDetune(0.0f);
            isKeyHeld[0] = true;
            voices[0].setGlide(glideEnabled && isLegato, glideTime);
            voices[0].noteOn(noteNumber, velocity);
            return;
        }

        // --- 2. UNISON MODE ---
        if (playMode == Unison) {
            unisonNoteStack.erase(
                std::remove_if(unisonNoteStack.begin(), unisonNoteStack.end(),
                               [noteNumber](const NoteEvent& e) { return e.noteNumber == noteNumber; }),
                                  unisonNoteStack.end());

            bool isLegato = !unisonNoteStack.empty();
            unisonNoteStack.push_back({ noteNumber, velocity });

            const float detuneSpread[NUM_VOICES] = { -14.0f, -7.0f, -2.0f, 2.0f, 7.0f, 14.0f };
            for (int i = 0; i < NUM_VOICES; ++i) {
                isKeyHeld[i] = true;
                voices[i].setUnisonDetune(detuneSpread[i]);
                voices[i].setGlide(glideEnabled && isLegato, glideTime);
                voices[i].noteOn(noteNumber, velocity);
            }
            return;
        }

        // --- 3. POLYPHONIC & CHORD MODE ---
        if (chordEnabled) {
            auto chordNotes = getChordIntervals(noteNumber);
            activeChords[noteNumber] = chordNotes;
            for (int note : chordNotes) {
                allocateSingleVoice(note, velocity, glideEnabled, glideTime);
            }
        } else {
            allocateSingleVoice(noteNumber, velocity, glideEnabled, glideTime);
        }
    }

    void handleNoteOff(int noteNumber) {
        // --- 1. MONOPHONIC RELEASE ---
        if (playMode == Mono) {
            if (cycleMode) {
                if (voices[lastMonoVoice].getNoteNumber() == noteNumber) {
                    isKeyHeld[lastMonoVoice] = false;
                    voices[lastMonoVoice].noteOff();
                }
                return;
            }

            monoNoteStack.erase(
                std::remove_if(monoNoteStack.begin(), monoNoteStack.end(),
                               [noteNumber](const NoteEvent& e) { return e.noteNumber == noteNumber; }),
                                monoNoteStack.end());

            if (monoNoteStack.empty()) {
                isKeyHeld[0] = false;
                voices[0].noteOff();
            } else {
                const auto& prevNote = monoNoteStack.back();
                voices[0].updateLegatoPitch(prevNote.noteNumber, prevNote.velocity);
            }
            return;
        }

        // --- 2. UNISON RELEASE ---
        if (playMode == Unison) {
            unisonNoteStack.erase(
                std::remove_if(unisonNoteStack.begin(), unisonNoteStack.end(),
                               [noteNumber](const NoteEvent& e) { return e.noteNumber == noteNumber; }),
                                  unisonNoteStack.end());

            if (unisonNoteStack.empty()) {
                for (int i = 0; i < NUM_VOICES; ++i) {
                    isKeyHeld[i] = false;
                    voices[i].noteOff();
                }
            } else {
                const auto& prevNote = unisonNoteStack.back();
                const float detuneSpread[NUM_VOICES] = { -14.0f, -7.0f, -2.0f, 2.0f, 7.0f, 14.0f };
                for (int i = 0; i < NUM_VOICES; ++i) {
                    voices[i].setUnisonDetune(detuneSpread[i]);
                    voices[i].updateLegatoPitch(prevNote.noteNumber, prevNote.velocity);
                }
            }
            return;
        }

        // --- 3. POLYPHONIC & CHORD RELEASE ---
        if (chordEnabled) {
            auto it = activeChords.find(noteNumber);
            if (it != activeChords.end()) {
                for (int note : it->second) {
                    releaseSingleVoice(note);
                }
                activeChords.erase(it);
            } else {
                releaseSingleVoice(noteNumber);
            }
        } else {
            releaseSingleVoice(noteNumber);
        }
    }

    void allNotesOff() {
        for (int i = 0; i < NUM_VOICES; ++i) {
            isKeyHeld[i] = false;
            voices[i].noteOff();
        }
        activeChords.clear();
        monoNoteStack.clear();
        unisonNoteStack.clear();
    }

    std::array<SynthVoice, NUM_VOICES>& getVoices() {
        return voices;
    }

    const std::array<SynthVoice, NUM_VOICES>& getVoices() const {
        return voices;
    }

private:
    void allocateSingleVoice(int noteNumber, float velocity, bool glideEnabled, float glideTime) {
        currentAgeCounter++;

        if (cycleMode) {
            int voiceIndex = cycleVoiceIndex;
            cycleVoiceIndex = (cycleVoiceIndex + 1) % NUM_VOICES;

            voiceAge[voiceIndex] = currentAgeCounter;
            isKeyHeld[voiceIndex] = true;
            voices[voiceIndex].setGlide(false, 0.0f);
            voices[voiceIndex].setUnisonDetune(0.0f);
            voices[voiceIndex].noteOn(noteNumber, velocity);
            return;
        }

        int voiceIndex = -1;

        // 1. Same Note Reuse
        for (int i = 0; i < NUM_VOICES; ++i) {
            if (voices[i].getNoteNumber() == noteNumber && voices[i].isActive()) {
                voiceIndex = i;
                break;
            }
        }

        // 2. Idle Voice
        if (voiceIndex == -1) {
            for (int i = 0; i < NUM_VOICES; ++i) {
                if (!voices[i].isActive()) {
                    voiceIndex = i;
                    break;
                }
            }
        }

        // 3. Steal from Released Voice
        if (voiceIndex == -1) {
            uint32_t oldestReleasedAge = 0xFFFFFFFF;
            for (int i = 0; i < NUM_VOICES; ++i) {
                if (!isKeyHeld[i] && keyOffAge[i] < oldestReleasedAge) {
                    oldestReleasedAge = keyOffAge[i];
                    voiceIndex = i;
                }
            }
        }

        // 4. Steal from Oldest Held Voice
        if (voiceIndex == -1) {
            uint32_t oldestHeldAge = 0xFFFFFFFF;
            for (int i = 0; i < NUM_VOICES; ++i) {
                if (voiceAge[i] < oldestHeldAge) {
                    oldestHeldAge = voiceAge[i];
                    voiceIndex = i;
                }
            }
        }

        if (voiceIndex >= 0 && voiceIndex < NUM_VOICES) {
            voiceAge[voiceIndex] = currentAgeCounter;
            isKeyHeld[voiceIndex] = true;
            voices[voiceIndex].setGlide(false, 0.0f);
            voices[voiceIndex].setUnisonDetune(0.0f);
            voices[voiceIndex].noteOn(noteNumber, velocity);
        }
    }

    void releaseSingleVoice(int noteNumber) {
        currentAgeCounter++;
        for (int i = 0; i < NUM_VOICES; ++i) {
            if (voices[i].getNoteNumber() == noteNumber && voices[i].isActive()) {
                isKeyHeld[i] = false;
                keyOffAge[i] = currentAgeCounter;
                voices[i].noteOff();
            }
        }
    }

    std::array<SynthVoice, NUM_VOICES> voices;
    std::array<uint32_t, NUM_VOICES> voiceAge;
    std::array<uint32_t, NUM_VOICES> keyOffAge;
    std::array<bool, NUM_VOICES> isKeyHeld;
    std::map<int, std::vector<int>> activeChords;
    std::vector<NoteEvent> monoNoteStack;
    std::vector<NoteEvent> unisonNoteStack;

    uint32_t currentAgeCounter = 0;
    int cycleVoiceIndex = 0;
    int lastMonoVoice = 0;
    PlayMode playMode = Poly;

    bool cycleMode = false;
    bool chordEnabled = false;
    ChordType chordType = Major;
};
