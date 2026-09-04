#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/SilverLookAndFeel.h"
#include "UI/VintageButtonKeyboard.h"
#include <array>
#include <memory>
#include <functional>

struct LEDTheme {
    juce::String name;
    juce::Colour primary;
    juce::Colour glow;
    juce::Colour highlight;
    juce::Colour unlit;
    juce::Colour unlitBg;
};

class SavePatchDialog : public juce::Component {
public:
    SavePatchDialog(std::function<void(const juce::String&)> onSave, std::function<void()> onCancel);
    void open();
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    juce::TextEditor patchNameEditor;
    juce::TextButton confirmSaveBtn { "SAVE" };
    juce::TextButton cancelSaveBtn { "CANCEL" };
    std::function<void(const juce::String&)> saveCallback;
    std::function<void()> cancelCallback;
};

class Simple106AudioProcessorEditor : public juce::AudioProcessorEditor,
                                      public juce::KeyListener,
                                      private juce::Timer
{
public:
    static constexpr int NUM_CHASSIS_THEMES = 4;
    static constexpr int NUM_QWERTY_KEYS = 16;

    explicit Simple106AudioProcessorEditor (Simple106AudioProcessor&);
    ~Simple106AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;
    void focusLost (FocusChangeType) override;
    void parentHierarchyChanged() override;
    void visibilityChanged() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    // Dual Component & KeyListener Overrides
    bool keyPressed (const juce::KeyPress& key) override;
    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override;
    bool keyStateChanged (bool isKeyDown) override;
    bool keyStateChanged (bool isKeyDown, juce::Component* originatingComponent) override;

    const LEDTheme& getActiveTheme() const {
        return ledThemes[juce::jlimit(0, 6, currentThemeIdx)];
    }

    const ChassisTheme& getChassisTheme() const {
        return chassisThemes[juce::jlimit(0, NUM_CHASSIS_THEMES - 1, currentChassisThemeIdx)];
    }

private:
    struct LabeledSlider {
        juce::Slider slider;
        juce::Label label;
    };

    bool handleKeyPress (const juce::KeyPress& key);
    bool handleKeyStateChanged (bool isKeyDown);

    void setupControl (LabeledSlider& ctrl, const juce::String& paramId, const juce::String& labelText);
    void setupBox (juce::ComboBox& box, juce::Label& label, const juce::String& paramId, const juce::String& text, const juce::StringArray& items);
    void updateVoiceKnobAttachments();
    void applyChassisTheme();
    void applyComboTextColours();
    void applyValueTextColours();
    void showSaveDialog();
    void hideSaveDialog();
    void performSave (const juce::String& rawName);
    void refreshPresetList();
    void renderBackgroundCache();
    void updateUIFromParameters();
    float getSafeParamValue (const juce::String& paramId, float fallback = 0.0f) const;

    void drawSegmentedString (juce::Graphics& g, float x, float y, float w, float h, const juce::String& text);
    void draw14SegmentChar (juce::Graphics& g, float x, float y, float w, float h, char c);
    void drawVoiceLED (juce::Graphics& g, float cx, float cy, bool on);

    Simple106AudioProcessor& audioProcessor;
    SilverLookAndFeel silverLookAndFeel;

    // Header Controls
    juce::ComboBox presetBox;
    juce::TextButton savePresetBtn { "SAVE" };
    juce::TextButton initPresetBtn { "INIT" };
    juce::ComboBox chassisThemeBox;
    juce::ComboBox themeBox;
    juce::TextButton pageTabBtn { "SYNTH/FX" };

    // DCO 1
    LabeledSlider dco1Morph, dco1PWM, dco1Octave, dco1Fold, dco1Level;

    // DCO 2
    LabeledSlider dco2Morph, dco2PWM, dco2Semi, dco2Cents, dco2Level;

    // Sub, Noise, X-Mod, Sync
    LabeledSlider subLevel, noiseLevel, glideTime, xmodAmount;
    juce::ComboBox subOctBox, xmodModeBox;
    juce::Label subOctLabel, xmodModeLabel;
    juce::TextButton syncBtn { "SYNC" };

    // Filters
    LabeledSlider hpfCutoff, lpfCutoff, lpfRes, envMod;

    // Envelopes & Master
    LabeledSlider filtA, filtD, filtS, filtR;
    LabeledSlider ampA, ampD, ampS, ampR;
    juce::ComboBox playModeBox;
    juce::Label playModeLabel;
    LabeledSlider masterVol;

    // LFO 1, 2, 3
    LabeledSlider lfo1Rate, lfo1ToFilt, lfo1ToPitch;
    juce::ComboBox lfo1ShapeBox;
    juce::Label lfo1ShapeLabel;

    LabeledSlider lfo2Rate, lfo2Amount;
    juce::ComboBox lfo2ShapeBox, lfo2TargetBox;
    juce::Label lfo2ShapeLabel, lfo2TargetLabel;

    LabeledSlider lfo3Rate, lfo3Amount;
    juce::ComboBox lfo3ShapeBox, lfo3TargetBox;
    juce::Label lfo3ShapeLabel, lfo3TargetLabel;

    // Voice Variation Matrix
    LabeledSlider voiceKnobs[6];
    juce::ComboBox voiceVarModeBox;
    juce::Label voiceVarLabel;
    juce::TextButton cycleBtn { "CYCLE" };

    // Master FX
    juce::Label chorusLabel;
    juce::ComboBox chorusModeBox;

    juce::Label delaySyncLabel;
    juce::ComboBox delaySyncBox;
    LabeledSlider delayTime, delayFb, delayDamp, delayMix;
    juce::TextButton delayPingPongBtn { "PING PONG" };

    LabeledSlider reverbSize, reverbDamp, reverbMix;

    // Sequencer, Arp & Keyboard Controls
    juce::TextButton seqPlayBtn { "PLAY" };
    juce::TextButton seqRecBtn  { "REC" };
    juce::TextButton seqRestBtn { "REST" };
    juce::TextButton seqClearBtn { "CLR" };

    juce::TextButton arpToggleBtn { "ARP" };
    juce::TextButton arpPrevBtn { "<" };
    juce::TextButton arpNextBtn { ">" };

    juce::TextButton chordToggleBtn { "CHORD" };
    juce::TextButton chordPrevBtn { "<" };
    juce::TextButton chordNextBtn { ">" };

    juce::TextButton stepsPrevBtn { "<" };
    juce::TextButton stepsNextBtn { ">" };
    juce::TextButton pagePrevBtn { "<" };
    juce::TextButton pageNextBtn { ">" };

    VintageButtonKeyboard buttonKeyboard;
    SavePatchDialog saveDialog;

    juce::Image backgroundCache;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> boxAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> btnAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> voiceAttachments;

    int currentTab = 0;
    int currentThemeIdx = 0;
    int currentChassisThemeIdx = 0;
    int currentArpIdx = 0;
    int currentChordIdx = 0;
    int currentPagesIdx = 3;
    int currentSeqPage = 0;

    std::array<bool, NUM_QWERTY_KEYS> qwertyDownState { false };
    std::array<int, NUM_QWERTY_KEYS> qwertyActiveNote { 0 };

    static const LEDTheme ledThemes[7];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Simple106AudioProcessorEditor)
};