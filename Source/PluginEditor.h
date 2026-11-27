#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

#include "S116Theme.h"
#include "SkinManager.h"
// Explicit path: Source/UI also contains a legacy header with this class name.
#include "../external/s116-ui/include/SilverLookAndFeel.h"
#include "S116Widgets.h"
#include "S116Layout.h"

#if S116_HAS_FONTS
 #include "BinaryData.h"
#endif

#include "UI/VintageButtonKeyboard.h"
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <vector>

struct LEDTheme {
    juce::String name;
    juce::Colour primary;
    juce::Colour glow;
    juce::Colour highlight;
    juce::Colour unlit;
    juce::Colour unlitBg;
};

// Footer readout: keeps the original 7-segment drawing and palette for C1.
// The kit LCD plate / token restyle is C2.
class SegmentReadout : public juce::Component {
public:
    SegmentReadout(float charWidth, int insetX);
    void setText(const juce::String& newText);
    void setPalette(const LEDTheme& newPalette);
    juce::String getText() const { return text; }
    void paint(juce::Graphics& g) override;

private:
    void drawString(juce::Graphics& g, float x, float y, float w, float h, const juce::String& text);
    void drawChar(juce::Graphics& g, float x, float y, float w, float h, char c);

    juce::String text;
    float charW;
    int inset;
    LEDTheme palette;
};

// Voice panel: panel skin + the six live voice LEDs, driven by isVoiceActive().
class VoicePanel : public S116::SectionPanel {
public:
    VoicePanel() : SectionPanel("VOICE VARIATION MATRIX") {}

    std::function<bool(int)> isVoiceActive;
    std::array<juce::Rectangle<int>, 6> ledRects;

protected:
    void paintDynamic(juce::Graphics& g) override;
};

class SavePatchDialog : public juce::Component {
public:
    SavePatchDialog(std::function<void(const juce::String&)> onSave, std::function<void()> onCancel);
    void open();
    void paint(juce::Graphics&) override;
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
                                      private juce::Timer,
                                      private juce::ChangeListener
{
public:
    static constexpr int NUM_CHASSIS_THEMES = 4;
    static constexpr int NUM_QWERTY_KEYS = 16;

    explicit Simple106AudioProcessorEditor (Simple106AudioProcessor&);
    ~Simple106AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;
    void focusGained (FocusChangeType) override;
    void focusLost (FocusChangeType) override;
    void parentHierarchyChanged() override;
    void visibilityChanged() override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    bool keyPressed (const juce::KeyPress& key) override;
    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override;
    bool keyStateChanged (bool isKeyDown) override;
    bool keyStateChanged (bool isKeyDown, juce::Component* originatingComponent) override;

    const LEDTheme& getActiveTheme() const {
        return ledThemes[juce::jlimit (0, 6, currentThemeIdx)];
    }

private:
    struct LabeledSlider {
        juce::Slider slider;
        juce::Label label;
    };

    struct KnobCellRef {
        juce::Slider* slider = nullptr;
        juce::Label* caption = nullptr;
        juce::Rectangle<int> cell;   // local to the slider's parent
        juce::String name;
    };

    bool handleKeyPress (const juce::KeyPress& key);
    bool handleKeyStateChanged (bool isKeyDown);

    void setupControl (LabeledSlider& ctrl, const juce::String& paramId,
                       const juce::String& labelText, juce::Component& parent);
    void setupBox (juce::ComboBox& box, juce::Label& label, const juce::String& paramId,
                   const juce::String& text, const juce::StringArray& items,
                   juce::Component& parent);

    void updateVoiceKnobAttachments();
    void updateSegmentDisplays();

    void applyChassisTheme();
    void applyLedTheme();
    void applyThisEditorsSkin();
    bool syncThemeParamsFromParameters();
    S116::ThemeId mapChassisTheme (int index) const;
    void refreshKeyboardLedTheme();
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void showSaveDialog();
    void hideSaveDialog();
    void performSave (const juce::String& rawName);
    void refreshPresetList();
    void updateUIFromParameters();
    float getSafeParamValue (const juce::String& paramId, float fallback = 0.0f) const;

    void layout();
    void layoutHeader();
    void layoutSynthPage();
    void layoutFxPage();
    void layoutFooter();

    void placeBare (juce::Slider& slider, juce::Label& caption, juce::Rectangle<int> cell,
                    const juce::String& name);
    void placeVoice (juce::Slider& slider, juce::Label& caption, juce::Rectangle<int> cell,
                     const juce::String& name);

    void showSliderValue (juce::Slider* slider);
    void setIdleHeaderLcd();
    juce::Slider* sliderAt (juce::Component* component) const;

    void runDebugAudit();
    bool collectBounds (juce::Component& parent, const juce::String& name,
                        bool verbose, bool& ok) const;
    bool auditFocus (bool verbose) const;
    bool auditAttachments (bool verbose) const;
    bool auditVoiceMatrix (bool verbose);
    bool auditRepaintTargets (bool verbose) const;

    Simple106AudioProcessor& audioProcessor;
    SilverLookAndFeel silverLookAndFeel;

    // Kit backdrops / pages / panels
    S116::ChassisBackground chassisBg;
    S116::HeaderBar headerBar;
    S116::LcdText headerLcd;
    juce::Component synthPage, fxPage;

    S116::SectionPanel dcoPanel { "DCO 1 & DCO 2" };
    S116::SectionPanel filterPanel { "HPF & 24dB VCF" };
    S116::SectionPanel envPanel { "ENVELOPES" };
    S116::SectionPanel subPanel { "SUB, NOISE & X-MOD" };
    S116::SectionPanel lfo1Panel { "LFO 1" };
    S116::SectionPanel lfo2Panel { "LFO 2" };
    S116::SectionPanel lfo3Panel { "LFO 3" };
    VoicePanel voicePanel;

    S116::SectionPanel chorusPanel { "STEREO CHORUS" };
    S116::SectionPanel delayPanel { "TEMPO-SYNC PING-PONG DELAY" };
    S116::SectionPanel reverbPanel { "LUSH DIGITAL REVERB" };
    S116::SectionPanel footerPanel { "SEQUENCER & KEYBOARD" };

    // Header
    juce::Label presetLabel, chassisThemeLabel, ledThemeLabel;
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

    juce::Label stepsLabel, pageLabel;
    SegmentReadout arpDisplay { 10.0f, 4 };
    SegmentReadout chordDisplay { 11.0f, 4 };
    SegmentReadout stepsDisplay { 11.0f, 6 };
    SegmentReadout pageDisplay { 11.0f, 4 };

    VintageButtonKeyboard buttonKeyboard;
    SavePatchDialog saveDialog;

    // Layout (single source of truth)
    juce::Rectangle<int> headerRect, bodyRect, footerRect;
    std::array<juce::Rectangle<int>, 8> synthPanelRects {};
    std::array<juce::Rectangle<int>, 3> fxPanelRects {};
    std::array<juce::Rectangle<int>, 8> synthPanelContent {};
    std::array<juce::Rectangle<int>, 3> fxPanelContent {};
    juce::Rectangle<int> footerContent;
    std::vector<KnobCellRef> knobCells;
    std::vector<std::pair<S116::SectionPanel*, juce::String>> panelTitles;

    // Attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> boxAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> btnAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> voiceAttachments;
    std::vector<juce::String> fixedAttachedIds, comboAttachedIds, buttonAttachedIds, voiceAttachedIds;

    std::map<juce::Slider*, juce::String> sliderNames;
    int headerLcdHoldTicks = 0;

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
