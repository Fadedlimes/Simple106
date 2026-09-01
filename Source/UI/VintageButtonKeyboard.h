#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <algorithm>

class VintageButtonKeyboard : public juce::Component,
public juce::MidiKeyboardState::Listener {
public:
    struct ThemeColors {
        juce::Colour primary   { 0xffff2d55 };
        juce::Colour glow      { 0x60ff2d55 };
        juce::Colour highlight { 0xffffa2b0 };
        juce::Colour unlit     { 0xff420e14 };
    };

    VintageButtonKeyboard(juce::MidiKeyboardState& state)
    : keyboardState(state)
    {
        keyboardState.addListener(this);
        setOpaque(true);
        setWantsKeyboardFocus(false);
    }

    ~VintageButtonKeyboard() override {
        keyboardState.removeListener(this);
    }

    int getOctaveOffset() const { return octaveShift * 12; }

    void setLEDTheme(juce::Colour primary, juce::Colour glow, juce::Colour highlight, juce::Colour unlit) {
        theme.primary = primary;
        theme.glow = glow;
        theme.highlight = highlight;
        theme.unlit = unlit;
        repaint();
    }

    void shiftOctave(int delta) {
        int newShift = std::clamp(octaveShift + delta, -2, 2);
        if (newShift != octaveShift) {
            octaveShift = newShift;
            noteStates.fill(false);
            repaint();
        }
    }

    void setSequencerState(bool playing, bool recording, int curStep, int recStep, int page) {
        if (seqPlaying != playing || seqRecording != recording ||
            seqCurrentStep != curStep || seqRecordStep != recStep || seqViewingPage != page)
        {
            seqPlaying = playing;
            seqRecording = recording;
            seqCurrentStep = curStep;
            seqRecordStep = recStep;
            seqViewingPage = page;
            repaint();
        }
    }

    void setActiveNotes(const std::array<bool, 25>& activeNotes) {
        if (noteStates != activeNotes) {
            noteStates = activeNotes;
            repaint();
        }
    }

    void handleNoteOn(juce::MidiKeyboardState*, int, int midiNoteNumber, float) override {
        juce::MessageManager::callAsync([this, midiNoteNumber]() {
            int currentStart = baseNote + getOctaveOffset();
            if (midiNoteNumber >= currentStart && midiNoteNumber < currentStart + numNotes) {
                noteStates[static_cast<size_t>(midiNoteNumber - currentStart)] = true;
                repaint();
            }
        });
    }

    void handleNoteOff(juce::MidiKeyboardState*, int, int midiNoteNumber, float) override {
        juce::MessageManager::callAsync([this, midiNoteNumber]() {
            int currentStart = baseNote + getOctaveOffset();
            if (midiNoteNumber >= currentStart && midiNoteNumber < currentStart + numNotes) {
                noteStates[static_cast<size_t>(midiNoteNumber - currentStart)] = false;
                repaint();
            }
        });
    }

    void paint(juce::Graphics& g) override {
        auto fullBounds = getLocalBounds().toFloat();

        // Fill background to match chassis
        g.setColour(juce::Colour(0xffcbcfd6));
        g.fillRect(fullBounds);

        auto bounds = fullBounds.reduced(1.0f);

        // Outer Bezel
        g.setColour(juce::Colour(0xff1c2026));
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(juce::Colour(0xff0e1014));
        g.drawRoundedRectangle(bounds, 6.0f, 1.5f);
        g.setColour(juce::Colour(0xff525a66));
        g.drawRoundedRectangle(bounds.expanded(0.5f), 6.5f, 1.0f);

        // Left Utility Section
        auto leftSection = bounds.removeFromLeft(210.0f).reduced(8.0f, 6.0f);
        g.setColour(juce::Colour(0xff333a46));
        g.drawVerticalLine(static_cast<int>(leftSection.getRight()) + 4, bounds.getY() + 6.0f, bounds.getBottom() - 6.0f);

        g.setColour(juce::Colour(0xff959eac));
        g.setFont(juce::FontOptions(10.5f).withStyle("Bold"));
        g.drawText("OCTAVE / CONTROL", leftSection.getX(), leftSection.getY(), leftSection.getWidth(), 14, juce::Justification::centredLeft);

        // Octave Badge
        auto badgeRect = juce::Rectangle<float>(leftSection.getX(), leftSection.getY() + 18.0f, leftSection.getWidth() - 10.0f, 22.0f);
        g.setColour(juce::Colour(0xff12151a));
        g.fillRoundedRectangle(badgeRect, 3.0f);
        g.setColour(juce::Colour(0xff3a414d));
        g.drawRoundedRectangle(badgeRect, 3.0f, 1.0f);

        juce::String octText;
        switch (octaveShift) {
            case -2: octText = "OCT -2  (C1 - C3)"; break;
            case -1: octText = "OCT -1  (C2 - C4)"; break;
            case  0: octText = "OCT  0  (C3 - C5)"; break;
            case  1: octText = "OCT +1  (C4 - C6)"; break;
            case  2: octText = "OCT +2  (C5 - C7)"; break;
            default: octText = "OCT  0"; break;
        }
        g.setColour(juce::Colour(0xffe2e6ec));
        g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        g.drawText(octText, badgeRect, juce::Justification::centred, false);

        drawUtilityButton(g, getOctDownBounds(), "OCT - [Z]", octDownDown);
        drawUtilityButton(g, getOctUpBounds(),   "OCT + [X]", octUpDown);

        // Themed 5-dot Octave LEDs
        float ledStartX = leftSection.getX() + 18.0f;
        float ledY = leftSection.getBottom() - 10.0f;
        for (int i = -2; i <= 2; ++i) {
            drawLED(g, ledStartX + (i + 2) * 38.0f, ledY, (i == octaveShift));
        }

        // Right Keybed Section
        auto keyArea = bounds.reduced(8.0f, 6.0f);
        int totalWhiteKeys = 15;
        float whiteKeyWidth = keyArea.getWidth() / static_cast<float>(totalWhiteKeys);
        float whiteKeyHeight = keyArea.getHeight() * 0.52f;
        float blackKeyHeight = keyArea.getHeight() * 0.44f;
        float topRowY = keyArea.getY() + 2.0f;
        float bottomRowY = keyArea.getY() + keyArea.getHeight() - whiteKeyHeight - 2.0f;

        const int naturalOffsets[] = { 0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24 };
        const char* noteNames[] = { "C", "D", "E", "F", "G", "A", "B", "C", "D", "E", "F", "G", "A", "B", "C" };

        // Draw Bottom Row (Naturals)
        for (int i = 0; i < totalWhiteKeys; ++i) {
            int noteIndex = naturalOffsets[i];
            bool isNoteDown = noteStates[static_cast<size_t>(noteIndex)];

            bool isStepActive = false;
            if (seqPlaying && (seqCurrentStep / 16 == seqViewingPage) && (seqCurrentStep % 16 == i)) isStepActive = true;
            if (seqRecording && (seqRecordStep / 16 == seqViewingPage) && (seqRecordStep % 16 == i)) isStepActive = true;

            auto btnRect = juce::Rectangle<float>(
                keyArea.getX() + i * whiteKeyWidth + 2.0f,
                                                  bottomRowY + (isNoteDown ? 1.5f : 0.0f),
                                                  whiteKeyWidth - 4.0f,
                                                  whiteKeyHeight
            );

            juce::Colour topCol = isNoteDown ? juce::Colour(0xffc5c8cc) : juce::Colour(0xfff7f8fa);
            juce::Colour botCol = isNoteDown ? juce::Colour(0xffa1a5ab) : juce::Colour(0xffd2d5db);
            juce::ColourGradient grad(topCol, btnRect.getCentreX(), btnRect.getY(),
                                      botCol, btnRect.getCentreX(), btnRect.getBottom(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(btnRect, 3.5f);

            g.setColour(isNoteDown ? juce::Colour(0xff4a505b) : juce::Colour(0xff878d98));
            g.drawRoundedRectangle(btnRect, 3.5f, 1.2f);

            g.setColour(juce::Colour(0xff22262c));
            g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
            g.drawText(noteNames[i], btnRect.removeFromBottom(16.0f), juce::Justification::centred, false);

            drawLED(g, btnRect.getCentreX(), btnRect.getY() + 7.0f, isNoteDown || isStepActive);
        }

        // Draw Top Row (Accidentals)
        const int sharpOffsets[] = { 1, 3, -1, 6, 8, 10, -1, 13, 15, -1, 18, 20, 22 };
        const char* sharpNames[] = { "C#", "D#", "", "F#", "G#", "A#", "", "C#", "D#", "", "F#", "G#", "A#" };

        for (int i = 0; i < 13; ++i) {
            if (sharpOffsets[i] < 0) continue;

            int noteIndex = sharpOffsets[i];
            bool isNoteDown = noteStates[static_cast<size_t>(noteIndex)];

            bool isStepActive = false;
            if (i == 12) {
                if (seqPlaying && (seqCurrentStep / 16 == seqViewingPage) && (seqCurrentStep % 16 == 15)) isStepActive = true;
                if (seqRecording && (seqRecordStep / 16 == seqViewingPage) && (seqRecordStep % 16 == 15)) isStepActive = true;
            }

            float xCenter = keyArea.getX() + (i + 1) * whiteKeyWidth;
            auto btnRect = juce::Rectangle<float>(
                xCenter - (whiteKeyWidth * 0.42f),
                                                  topRowY + (isNoteDown ? 1.5f : 0.0f),
                                                  whiteKeyWidth * 0.84f,
                                                  blackKeyHeight
            );

            juce::Colour topCol = isNoteDown ? juce::Colour(0xff22252a) : juce::Colour(0xff454b54);
            juce::Colour botCol = isNoteDown ? juce::Colour(0xff121418) : juce::Colour(0xff2a2e35);
            juce::ColourGradient grad(topCol, btnRect.getCentreX(), btnRect.getY(),
                                      botCol, btnRect.getCentreX(), btnRect.getBottom(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(btnRect, 3.5f);

            g.setColour(isNoteDown ? juce::Colour(0xff111316) : juce::Colour(0xff5e6570));
            g.drawRoundedRectangle(btnRect, 3.5f, 1.2f);

            g.setColour(juce::Colour(0xffdcdfe5));
            g.setFont(juce::FontOptions(9.5f).withStyle("Bold"));
            g.drawText(sharpNames[i], btnRect.removeFromBottom(14.0f), juce::Justification::centred, false);

            drawLED(g, btnRect.getCentreX(), btnRect.getY() + 7.0f, isNoteDown || isStepActive);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        auto pos = e.getPosition();

        if (getOctDownBounds().contains(pos.toFloat())) {
            octDownDown = true;
            shiftOctave(-1);
            repaint();
            return;
        }

        if (getOctUpBounds().contains(pos.toFloat())) {
            octUpDown = true;
            shiftOctave(1);
            repaint();
            return;
        }

        int note = getNoteAtPosition(pos);
        if (note >= 0) {
            activeMouseNote = note;
            keyboardState.noteOn(1, activeMouseNote, 1.0f);
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (octDownDown || octUpDown) return;

        int note = getNoteAtPosition(e.getPosition());
        if (note != activeMouseNote) {
            if (activeMouseNote >= 0) {
                keyboardState.noteOff(1, activeMouseNote, 0.0f);
            }
            activeMouseNote = note;
            if (activeMouseNote >= 0) {
                keyboardState.noteOn(1, activeMouseNote, 1.0f);
            }
        }
    }

    void mouseUp(const juce::MouseEvent&) override {
        if (octDownDown || octUpDown) {
            octDownDown = false;
            octUpDown = false;
            repaint();
        }

        if (activeMouseNote >= 0) {
            keyboardState.noteOff(1, activeMouseNote, 0.0f);
            activeMouseNote = -1;
        }
    }

private:
    juce::Rectangle<float> getOctDownBounds() const {
        return juce::Rectangle<float>(12.0f, 52.0f, 90.0f, 38.0f);
    }

    juce::Rectangle<float> getOctUpBounds() const {
        return juce::Rectangle<float>(110.0f, 52.0f, 90.0f, 38.0f);
    }

    void drawUtilityButton(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& text, bool isDown) {
        bounds = bounds.translated(0.0f, isDown ? 1.5f : 0.0f);

        juce::Colour topCol = isDown ? juce::Colour(0xff2d333b) : juce::Colour(0xff4c5460);
        juce::Colour botCol = isDown ? juce::Colour(0xff181b20) : juce::Colour(0xff2d333b);
        juce::ColourGradient grad(topCol, bounds.getCentreX(), bounds.getY(),
                                  botCol, bounds.getCentreX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(isDown ? juce::Colour(0xff111418) : juce::Colour(0xff6e7888));
        g.drawRoundedRectangle(bounds, 4.0f, 1.2f);

        g.setColour(juce::Colour(0xfff0f2f5));
        g.setFont(juce::FontOptions(11.5f).withStyle("Bold"));
        g.drawText(text, bounds, juce::Justification::centred, false);
    }

    void drawLED(juce::Graphics& g, float cx, float cy, bool on) {
        float r = 3.2f;
        if (on) {
            g.setColour(theme.glow);
            g.fillEllipse(cx - r - 2.5f, cy - r - 2.5f, (r + 2.5f) * 2.0f, (r + 2.5f) * 2.0f);

            g.setColour(theme.primary);
            g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

            g.setColour(theme.highlight);
            g.fillEllipse(cx - r * 0.45f, cy - r * 0.45f, r * 0.9f, r * 0.9f);
        } else {
            g.setColour(theme.unlit);
            g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
            g.setColour(theme.unlit.darker(0.3f));
            g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 0.8f);
        }
    }

    int getNoteAtPosition(juce::Point<int> pos) {
        auto keyArea = getLocalBounds().toFloat().reduced(1.0f);
        keyArea.removeFromLeft(210.0f);
        keyArea = keyArea.reduced(8.0f, 6.0f);

        if (!keyArea.contains(static_cast<float>(pos.x), static_cast<float>(pos.y))) return -1;

        int totalWhiteKeys = 15;
        float whiteKeyWidth = keyArea.getWidth() / static_cast<float>(totalWhiteKeys);
        float blackKeyHeight = keyArea.getHeight() * 0.44f;
        int currentStart = baseNote + getOctaveOffset();

        if (pos.y <= keyArea.getY() + blackKeyHeight + 2.0f) {
            const int sharpOffsets[] = { 1, 3, -1, 6, 8, 10, -1, 13, 15, -1, 18, 20, 22 };
            for (int i = 0; i < 13; ++i) {
                if (sharpOffsets[i] < 0) continue;
                float xCenter = keyArea.getX() + (i + 1) * whiteKeyWidth;
                auto btnRect = juce::Rectangle<float>(xCenter - (whiteKeyWidth * 0.42f), keyArea.getY() + 2.0f, whiteKeyWidth * 0.84f, blackKeyHeight);
                if (btnRect.contains(static_cast<float>(pos.x), static_cast<float>(pos.y))) {
                    return currentStart + sharpOffsets[i];
                }
            }
        }

        int whiteIndex = static_cast<int>((pos.x - keyArea.getX()) / whiteKeyWidth);
        if (whiteIndex >= 0 && whiteIndex < totalWhiteKeys) {
            const int naturalOffsets[] = { 0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24 };
            return currentStart + naturalOffsets[whiteIndex];
        }

        return -1;
    }

    juce::MidiKeyboardState& keyboardState;
    ThemeColors theme;

    static constexpr int baseNote = 48; // C3
    static constexpr int numNotes = 25; // 2 Octaves
    int octaveShift = 0;
    bool octDownDown = false;
    bool octUpDown = false;
    std::array<bool, numNotes> noteStates { false };
    int activeMouseNote = -1;

    bool seqPlaying = false;
    bool seqRecording = false;
    int seqCurrentStep = 0;
    int seqRecordStep = 0;
    int seqViewingPage = 0;
};
