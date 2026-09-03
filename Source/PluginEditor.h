#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/SilverLookAndFeel.h"
#include "UI/VintageButtonKeyboard.h"
#include <array>
#include <vector>
#include <memory>
#include <functional>

struct LEDTheme {
    const char* name;
    juce::Colour primary;
    juce::Colour glow;
    juce::Colour highlight;
    juce::Colour unlit;
    juce::Colour unlitBg;
};

// Dedicated Modal Save Patch Dialog
class SavePatchDialog : public juce::Component {
public:
    SavePatchDialog(std::function<void(const juce::String&)> onSave, std::function<void()> onCancel);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void open();

    juce::TextEditor patchNameEditor;
    juce::TextButton confirmSaveBtn { "SAVE" };
    juce::TextButton cancelSaveBtn  { "CANCEL" };

private:
    std::function<void(const juce::String&)> saveCallback;
    std::function<void()> cancelCallback;
};

class Simple106AudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::KeyListener,
    public juce::Timer {
public:
    explicit Simple106AudioProcessorEditor(Simple106AudioProcessor&);
    ~Simple106AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;
    void mouseDown(const juce::MouseEvent&) override;
    void focusLost(FocusChangeType) override;
    void timerCallback() override;

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    bool keyStateChanged(bool isKeyDown, juce::Component* originatingComponent) override;

    void refreshPresetList();
    void updateUIFromParameters();

private:
    Simple106AudioProcessor& audioProcessor;
    SilverLookAndFeel silverLookAndFeel;

    // Hardware background cache
    juce::Image backgroundCache;
    void renderBackgroundCache();

    // UI Theme (Chassis)
    int currentChassisThemeIdx = 0;
    juce::ComboBox chassisThemeBox;   // The THEME dropdown
    const ChassisTheme& getChassisTheme() const {
        int idx = juce::jlimit(0, NUM_CHASSIS_THEMES - 1, currentChassisThemeIdx);
        return chassisThemes[idx];
    }
    void applyChassisTheme();

    // LED Theme (Independent COLOUR selector)
    int currentThemeIdx = 0;
    static const LEDTheme ledThemes[7];
    const LEDTheme& getActiveTheme() const {
        int idx = juce::jlimit(0, 6, currentThemeIdx);
        return ledThemes[idx];
    }

    // Header Controls
    juce::ComboBox presetBox;
    juce::TextButton savePresetBtn { "SAVE" };
    juce::TextButton initPresetBtn { "INIT" };
    juce::ComboBox themeBox;         // COLOUR (LED) selector

    // Save Patch Modal Dialog
    SavePatchDialog saveDialog;
    void showSaveDialog();
    void hideSaveDialog();
    void performSave(const juce::String& name);

    // Tabs
    int currentTab = 0;
    juce::TextButton synthTabBtn { "SYNTH ENGINE" };
    juce::TextButton fxTabBtn { "MASTER FX" };

    struct LabeledSlider {
        juce::Slider slider;
        juce::Label label;
    };

    // --- SYNTH PAGE CONTROLS ---
    LabeledSlider dco1Morph, dco1PWM, dco1Level;
    LabeledSlider dco2Morph, dco2PWM, dco2Semi, dco2Cents, dco2Level;
    LabeledSlider subLevel, noiseLevel;
    LabeledSlider hpfCutoff, lpfCutoff, lpfRes, envMod;
    LabeledSlider ampA, ampD, ampS, ampR;
    LabeledSlider filtA, filtD, filtS, filtR;
    LabeledSlider lfo1Rate, lfo1ToFilt, lfo1ToPitch;
    LabeledSlider lfo2Rate, lfo2Amount;
    LabeledSlider lfo3Rate, lfo3Amount;
    LabeledSlider glideTime, masterVol;

    // Voice Variation Matrix
    LabeledSlider voiceKnobs[6];
    juce::ComboBox voiceVarModeBox;
    juce::Label voiceVarLabel;
    juce::TextButton cycleBtn { "CYCLE" };

    // Dropdowns
    juce::ComboBox playModeBox, lfo1ShapeBox, lfo2ShapeBox, lfo2TargetBox, lfo3ShapeBox, lfo3TargetBox;
    juce::Label playModeLabel, lfo1ShapeLabel, lfo2ShapeLabel, lfo2TargetLabel, lfo3ShapeLabel, lfo3TargetLabel;

    // --- MASTER FX CONTROLS ---
    juce::ComboBox chorusModeBox, delaySyncBox;
    juce::Label chorusLabel, delaySyncLabel;

    LabeledSlider delayTime, delayFb, delayDamp, delayMix;
    juce::ToggleButton delayPingPongBtn { "PING-PONG" };

    LabeledSlider reverbSize, reverbDamp, reverbMix;

    // --- SEQUENCER & PERFORMANCE CONTROL BAR ---
    VintageButtonKeyboard buttonKeyboard;

    juce::TextButton seqPlayBtn  { "PLAY" };
    juce::TextButton seqRecBtn   { "REC" };
    juce::TextButton seqRestBtn  { "REST" };
    juce::TextButton seqClearBtn { "CLR" };

    juce::TextButton arpToggleBtn { "ARP" };
    juce::TextButton arpPrevBtn   { "<" };
    juce::TextButton arpNextBtn   { ">" };
    int currentArpIdx = 0;

    juce::TextButton chordToggleBtn { "CHORD" };
    juce::TextButton chordPrevBtn   { "<" };
    juce::TextButton chordNextBtn   { ">" };
    int currentChordIdx = 0;

    juce::TextButton stepsPrevBtn { "<" };
    juce::TextButton stepsNextBtn { ">" };
    int currentPagesIdx = 3;

    juce::TextButton pagePrevBtn { "<" };
    juce::TextButton pageNextBtn { ">" };
    int currentSeqPage = 0;

    // QWERTY tracking
    static constexpr size_t NUM_QWERTY_KEYS = 16;
    std::array<bool, NUM_QWERTY_KEYS> qwertyDownState { false };
    std::array<int, NUM_QWERTY_KEYS> qwertyActiveNote { 0 };

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> voiceAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> boxAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> btnAttachments;

    void setupControl(LabeledSlider& ctrl, const juce::String& paramId, const juce::String& labelText);
    void setupBox(juce::ComboBox& box, juce::Label& label, const juce::String& paramId, const juce::String& text, const juce::StringArray& items);
    void updateVoiceKnobAttachments();
    void drawVoiceLED(juce::Graphics& g, float cx, float cy, bool on);
    void drawSegmentedString(juce::Graphics& g, float x, float y, float w, float h, const juce::String& text);
    void draw14SegmentChar(juce::Graphics& g, float x, float y, float w, float h, char c);

    float getSafeParamValue(const juce::String& paramId, float fallback = 0.0f) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Simple106AudioProcessorEditor)
};
