#pragma once
#include "SynthVoice.h"
#include <array>
#include <vector>
#include <algorithm>
#include <cstdint>

class VoiceManager {
public:
    static constexpr int NUM_VOICES = 6;
    static constexpr int MAX_NOTE_STACK = 16;
    static constexpr int MAX_ACTIVE_CHORDS = 16;

    enum PlayMode { Poly = 0, Mono, Unison };
    enum ChordType { Major = 0, Minor, Maj7, Min7, Dom7, Sus4, Diminished, Octave, Power5th };

    struct NoteEvent {
        int noteNumber;
        float velocity;
    };

    struct ChordEntry {
        int rootNote = -1;
        int noteCount = 0;
        int notes[5] = { 0 };
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
        monoStackSize = 0;
        unisonStackSize = 0;
        numActiveChords = 0;
    }

    void setPlayMode(int mode) {
        playMode = static_cast<PlayMode>(mode);
        isKeyHeld.fill(false);
        monoStackSize = 0;
        unisonStackSize = 0;
        numActiveChords = 0;
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
        notes.reserve(4);
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

            eraseFromStack(monoNoteStack, monoStackSize, noteNumber);
            bool isLegato = (monoStackSize > 0);
            pushToStack(monoNoteStack, monoStackSize, { noteNumber, velocity });

            voices[0].setUnisonDetune(0.0f);
            isKeyHeld[0] = true;
            voices[0].setGlide(glideEnabled && isLegato, glideTime);

            if (isLegato && glideEnabled) {
                voices[0].updateLegatoPitch(noteNumber, velocity);
            } else {
                voices[0].noteOn(noteNumber, velocity);
            }
            return;
        }

        // --- 2. UNISON MODE ---
        if (playMode == Unison) {
            eraseFromStack(unisonNoteStack, unisonStackSize, noteNumber);
            bool isLegato = (unisonStackSize > 0);
            pushToStack(unisonNoteStack, unisonStackSize, { noteNumber, velocity });

            const float detuneSpread[NUM_VOICES] = { -14.0f, -7.0f, -2.0f, 2.0f, 7.0f, 14.0f };
            for (int i = 0; i < NUM_VOICES; ++i) {
                isKeyHeld[i] = true;
                voices[i].setUnisonDetune(detuneSpread[i]);
                voices[i].setGlide(glideEnabled && isLegato, glideTime);
                if (isLegato && glideEnabled) {
                    voices[i].updateLegatoPitch(noteNumber, velocity);
                } else {
                    voices[i].noteOn(noteNumber, velocity);
                }
            }
            return;
        }

        // --- 3. POLYPHONIC & CHORD MODE ---
        if (chordEnabled) {
            auto chordNotes = getChordIntervals(noteNumber);
            if (numActiveChords < MAX_ACTIVE_CHORDS) {
                ChordEntry& entry = activeChordsTable[numActiveChords++];
                entry.rootNote = noteNumber;
                entry.noteCount = std::min(static_cast<int>(chordNotes.size()), 5);
                for (int i = 0; i < entry.noteCount; ++i) {
                    entry.notes[i] = chordNotes[i];
                }
            }
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

            eraseFromStack(monoNoteStack, monoStackSize, noteNumber);
            if (monoStackSize == 0) {
                isKeyHeld[0] = false;
                voices[0].noteOff();
            } else {
                const auto& prevNote = monoNoteStack[monoStackSize - 1];
                voices[0].updateLegatoPitch(prevNote.noteNumber, prevNote.velocity);
            }
            return;
        }

        // --- 2. UNISON RELEASE ---
        if (playMode == Unison) {
            eraseFromStack(unisonNoteStack, unisonStackSize, noteNumber);
            if (unisonStackSize == 0) {
                for (int i = 0; i < NUM_VOICES; ++i) {
                    isKeyHeld[i] = false;
                    voices[i].noteOff();
                }
            } else {
                const auto& prevNote = unisonNoteStack[unisonStackSize - 1];
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
            int foundIndex = -1;
            for (int i = 0; i < numActiveChords; ++i) {
                if (activeChordsTable[i].rootNote == noteNumber) {
                    foundIndex = i;
                    break;
                }
            }

            if (foundIndex >= 0) {
                for (int i = 0; i < activeChordsTable[foundIndex].noteCount; ++i) {
                    releaseSingleVoice(activeChordsTable[foundIndex].notes[i]);
                }
                activeChordsTable[foundIndex] = activeChordsTable[numActiveChords - 1];
                numActiveChords--;
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
        numActiveChords = 0;
        monoStackSize = 0;
        unisonStackSize = 0;
    }

    std::array<SynthVoice, NUM_VOICES>& getVoices() {
        return voices;
    }

    const std::array<SynthVoice, NUM_VOICES>& getVoices() const {
        return voices;
    }

private:
    static void eraseFromStack(std::array<NoteEvent, MAX_NOTE_STACK>& stack, int& size, int note) {
        int w = 0;
        for (int i = 0; i < size; ++i) {
            if (stack[i].noteNumber != note) {
                stack[w++] = stack[i];
            }
        }
        size = w;
    }

    static void pushToStack(std::array<NoteEvent, MAX_NOTE_STACK>& stack, int& size, const NoteEvent& event) {
        if (size < MAX_NOTE_STACK) {
            stack[size++] = event;
        }
    }

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

        // Tier 1: Same Note Reuse
        for (int i = 0; i < NUM_VOICES; ++i) {
            if (voices[i].getNoteNumber() == noteNumber && voices[i].isActive()) {
                voiceIndex = i;
                break;
            }
        }

        // Tier 2: Idle Voice
        if (voiceIndex == -1) {
            for (int i = 0; i < NUM_VOICES; ++i) {
                if (!voices[i].isActive()) {
                    voiceIndex = i;
                    break;
                }
            }
        }

        // Tier 3: Steal Oldest Released Voice
        if (voiceIndex == -1) {
            uint32_t oldestReleasedAge = 0xFFFFFFFF;
            for (int i = 0; i < NUM_VOICES; ++i) {
                if (!isKeyHeld[i] && keyOffAge[i] < oldestReleasedAge) {
                    oldestReleasedAge = keyOffAge[i];
                    voiceIndex = i;
                }
            }
        }

        // Tier 4: Steal Oldest Held Voice Fallback
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

    std::array<ChordEntry, MAX_ACTIVE_CHORDS> activeChordsTable;
    int numActiveChords = 0;

    std::array<NoteEvent, MAX_NOTE_STACK> monoNoteStack;
    int monoStackSize = 0;

    std::array<NoteEvent, MAX_NOTE_STACK> unisonNoteStack;
    int unisonStackSize = 0;

    uint32_t currentAgeCounter = 0;
    int cycleVoiceIndex = 0;
    int lastMonoVoice = 0;
    PlayMode playMode = Poly;

    bool cycleMode = false;
    bool chordEnabled = false;
    ChordType chordType = Major;
};