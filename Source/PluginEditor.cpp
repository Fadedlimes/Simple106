#include "PluginProcessor.h"
#include "PluginEditor.h"

struct QwertyKeyMap {
    int keyCode;
    int semitoneOffset;
};

static const QwertyKeyMap qwertyMappings[] = {
    { 'A', 0 }, { 'W', 1 }, { 'S', 2 }, { 'E', 3 },
    { 'D', 4 }, { 'F', 5 }, { 'T', 6 }, { 'G', 7 },
    { 'Y', 8 }, { 'H', 9 }, { 'U', 10 }, { 'J', 11 },
    { 'K', 12 }, { 'O', 13 }, { 'L', 14 }, { 'P', 15 }
};

// 7 LED Theme Color Palettes
const LEDTheme Simple106AudioProcessorEditor::ledThemes[7] = {
    { "Vintage Red",   juce::Colour(0xffff2d55), juce::Colour(0x60ff2d55), juce::Colour(0xffffa2b0), juce::Colour(0xff420e14), juce::Colour(0x1830080d) },
    { "Neon Cyan",     juce::Colour(0xff00d2d3), juce::Colour(0x6000d2d3), juce::Colour(0xffa8fffc), juce::Colour(0xff042629), juce::Colour(0x18002b2c) },
    { "Acid Green",    juce::Colour(0xff2ecc71), juce::Colour(0x602ecc71), juce::Colour(0xffa3f7c4), juce::Colour(0xff0c381a), juce::Colour(0x180c381a) },
    { "Amber Gold",    juce::Colour(0xffff9f43), juce::Colour(0x60ff9f43), juce::Colour(0xffffe0b2), juce::Colour(0xff4a2a0a), juce::Colour(0x184a2a0a) },
    { "Solar Yellow",  juce::Colour(0xfffeca57), juce::Colour(0x60feca57), juce::Colour(0xfffff4cc), juce::Colour(0xff44340a), juce::Colour(0x1844340a) },
    { "Ultraviolet",   juce::Colour(0xff9b59b6), juce::Colour(0x609b59b6), juce::Colour(0xffe4b5f7), juce::Colour(0xff281133), juce::Colour(0x18281133) },
    { "Ghost White",   juce::Colour(0xffffffff), juce::Colour(0x60ffffff), juce::Colour(0xffffffff), juce::Colour(0xff2a2e36), juce::Colour(0x182a2e36) }
};

// ==============================================================================
// SavePatchDialog Implementation
// ==============================================================================
SavePatchDialog::SavePatchDialog(std::function<void(const juce::String&)> onSave, std::function<void()> onCancel)
: saveCallback(onSave), cancelCallback(onCancel)
{
    setAlwaysOnTop(true);

    patchNameEditor.setWantsKeyboardFocus(true);
    patchNameEditor.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    patchNameEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xfff0f2f5));
    patchNameEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xff14171a));
    patchNameEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff68707d));
    patchNameEditor.onReturnKey = [this]() { if (saveCallback) saveCallback(patchNameEditor.getText()); };
    patchNameEditor.onEscapeKey = [this]() { if (cancelCallback) cancelCallback(); };
    addAndMakeVisible(patchNameEditor);

    confirmSaveBtn.setWantsKeyboardFocus(false);
    confirmSaveBtn.onClick = [this]() { if (saveCallback) saveCallback(patchNameEditor.getText()); };
    addAndMakeVisible(confirmSaveBtn);

    cancelSaveBtn.setWantsKeyboardFocus(false);
    cancelSaveBtn.onClick = [this]() { if (cancelCallback) cancelCallback(); };
    addAndMakeVisible(cancelSaveBtn);
}

void SavePatchDialog::open() {
    patchNameEditor.setText("My Patch", false);
    patchNameEditor.selectAll();
    patchNameEditor.grabKeyboardFocus();
}

void SavePatchDialog::mouseDown(const juce::MouseEvent&) {}

void SavePatchDialog::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0x95000000));

    auto dialogBox = juce::Rectangle<float>(
        static_cast<float>((getWidth() - 360) / 2),
                                            static_cast<float>((getHeight() - 150) / 2),
                                            360.0f, 150.0f
    );

    g.setColour(juce::Colour(0xff23272e));
    g.fillRoundedRectangle(dialogBox.expanded(3.0f), 8.0f);
    g.setColour(juce::Colour(0xff4a5260));
    g.drawRoundedRectangle(dialogBox.expanded(3.0f), 8.0f, 1.5f);

    juce::ColourGradient plateGrad(juce::Colour(0xffe4e7ec), dialogBox.getX(), dialogBox.getY(),
                                   juce::Colour(0xffcbd0d8), dialogBox.getRight(), dialogBox.getBottom(), false);
    g.setGradientFill(plateGrad);
    g.fillRoundedRectangle(dialogBox, 6.0f);

    auto header = dialogBox.removeFromTop(32.0f);
    g.setColour(juce::Colour(0xff1f242b));
    g.fillRect(header);

    g.setColour(juce::Colour(0xffe74c3c));
    g.fillRect(header.getX(), header.getBottom(), header.getWidth(), 2.5f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    g.drawText("SAVE CUSTOM PATCH", header, juce::Justification::centred, false);
}

void SavePatchDialog::resized() {
    int boxX = (getWidth() - 360) / 2;
    int boxY = (getHeight() - 150) / 2;

    patchNameEditor.setBounds(boxX + 24, boxY + 48, 312, 30);
    confirmSaveBtn.setBounds(boxX + 75, boxY + 96, 95, 30);
    cancelSaveBtn.setBounds(boxX + 190, boxY + 96, 95, 30);
}

// ==============================================================================
// Simple106AudioProcessorEditor Implementation
// ==============================================================================
Simple106AudioProcessorEditor::Simple106AudioProcessorEditor(Simple106AudioProcessor& p)
: AudioProcessorEditor(&p), audioProcessor(p),
saveDialog([this](const juce::String& name) { performSave(name); }, [this]() { hideSaveDialog(); }),
buttonKeyboard(p.keyboardState)
{
    setLookAndFeel(&silverLookAndFeel);
    setSize(920, 640);
    setOpaque(true);

    // --- HEADER PRESET & THEME CONTROLS ---
    refreshPresetList();
    presetBox.setWantsKeyboardFocus(false);
    presetBox.onChange = [this]() {
        int id = presetBox.getSelectedId();
        if (id >= 1 && id <= 8) {
            audioProcessor.loadFactoryPreset(id - 1);
        } else if (id > 8) {
            auto presetFile = audioProcessor.getPresetsDirectory().getChildFile(presetBox.getText() + ".s106");
            if (presetFile.existsAsFile()) {
                audioProcessor.loadPatchXml(presetFile.loadFileAsString());
            }
        }
        repaint();
    };
    addAndMakeVisible(presetBox);

    savePresetBtn.setWantsKeyboardFocus(false);
    savePresetBtn.onClick = [this]() {
        showSaveDialog();
    };
    addAndMakeVisible(savePresetBtn);

    initPresetBtn.setWantsKeyboardFocus(false);
    initPresetBtn.onClick = [this]() {
        audioProcessor.loadFactoryPreset(0);
        presetBox.setSelectedId(1, juce::dontSendNotification);
        repaint();
    };
    addAndMakeVisible(initPresetBtn);

    // Colour Selector Setup
    themeBox.addItemList({"Red", "Cyan", "Green", "Amber", "Yellow", "Purple", "White"}, 1);
    themeBox.setWantsKeyboardFocus(false);
    currentThemeIdx = static_cast<int>(audioProcessor.apvts.getRawParameterValue("ledTheme")->load());
    themeBox.setSelectedId(currentThemeIdx + 1, juce::dontSendNotification);

    auto applyTheme = [this]() {
        auto t = getActiveTheme();
        silverLookAndFeel.setAccentColour(t.primary, t.glow, t.highlight);
        buttonKeyboard.setLEDTheme(t.primary, t.glow, t.highlight, t.unlit);
        repaint();
    };

    themeBox.onChange = [this, applyTheme]() {
        currentThemeIdx = themeBox.getSelectedItemIndex();
        if (auto* param = audioProcessor.apvts.getParameter("ledTheme"))
            param->setValueNotifyingHost(static_cast<float>(currentThemeIdx) / 6.0f);
        applyTheme();
    };
    addAndMakeVisible(themeBox);
    applyTheme();

    // Tab buttons
    auto setupTabBtn = [this](juce::TextButton& btn, int tabIdx) {
        btn.setButtonText(tabIdx == 0 ? "SYNTH" : "FX");
        btn.setWantsKeyboardFocus(false);
        btn.onClick = [this, tabIdx]() {
            currentTab = tabIdx;
            renderBackgroundCache();
            resized();
            repaint();
        };
        addAndMakeVisible(btn);
    };
    setupTabBtn(synthTabBtn, 0);
    setupTabBtn(fxTabBtn, 1);

    // --- SYNTH CONTROLS ---
    setupControl(dco1Morph, "dco1Morph", "MORPH");
    setupControl(dco1PWM,   "dco1PWM",   "PWM");
    setupControl(dco1Level, "dco1Level", "LEVEL");

    setupControl(dco2Morph, "dco2Morph", "MORPH");
    setupControl(dco2PWM,   "dco2PWM",   "PWM");
    setupControl(dco2Semi,  "dco2Semi",  "SEMI");
    setupControl(dco2Cents, "dco2Cents", "DETUNE");
    setupControl(dco2Level, "dco2Level", "LEVEL");

    auto formatMorph = [](double val) -> juce::String {
        if (val <= 0.06) return "TRI";
        if (val < 0.28)  return "TRI-SAW";
        if (val <= 0.38) return "SAW";
        if (val < 0.61)  return "SAW-SQR";
        if (val <= 0.72) return "SQUARE";
        if (val < 0.95)  return "SQR-PLS";
        return "PULSE";
    };

    dco1Morph.slider.textFromValueFunction = formatMorph;
    dco2Morph.slider.textFromValueFunction = formatMorph;

    dco2Semi.slider.textFromValueFunction = [](double v) { return juce::String((int)v) + " st"; };
    dco2Cents.slider.textFromValueFunction = [](double v) { return juce::String((int)v) + " ct"; };
    hpfCutoff.slider.textFromValueFunction = [](double v) { return juce::String((int)v) + " Hz"; };
    lpfCutoff.slider.textFromValueFunction = [](double v) { return (v >= 1000.0) ? juce::String(v / 1000.0, 1) + "k" : juce::String((int)v); };
    lfo1Rate.slider.textFromValueFunction = [](double v) { return juce::String(v, 1) + " Hz"; };
    lfo2Rate.slider.textFromValueFunction = [](double v) { return juce::String(v, 1) + " Hz"; };
    lfo3Rate.slider.textFromValueFunction = [](double v) { return juce::String(v, 1) + " Hz"; };
    envMod.slider.textFromValueFunction = [](double v) { return juce::String((int)(v * 100.0)) + "%"; };

    setupControl(subLevel,   "subLevel",   "SUB OSC");
    setupControl(noiseLevel, "noiseLevel", "NOISE");

    setupControl(hpfCutoff, "hpfCutoff", "HPF");
    setupControl(lpfCutoff, "lpfCutoff", "CUTOFF");
    setupControl(lpfRes,    "lpfRes",    "RESO");
    setupControl(envMod,    "envMod",    "ENV MOD");

    setupControl(filtA, "filtAttack",  "ATTACK");
    setupControl(filtD, "filtDecay",   "DECAY");
    setupControl(filtS, "filtSustain", "SUSTAIN");
    setupControl(filtR, "filtRelease", "RELEASE");

    setupControl(ampA, "ampAttack",  "ATTACK");
    setupControl(ampD, "ampDecay",   "DECAY");
    setupControl(ampS, "ampSustain", "SUSTAIN");
    setupControl(ampR, "ampRelease", "RELEASE");

    setupControl(lfo1Rate,    "lfo1Rate",     "RATE");
    setupControl(lfo1ToFilt,  "lfo1ToFilter", "VCF MOD");
    setupControl(lfo1ToPitch, "lfo1ToPitch",  "PITCH");

    setupControl(lfo2Rate,    "lfo2Rate",     "RATE");
    setupControl(lfo2Amount,  "lfo2Amount",   "DEPTH");

    setupControl(lfo3Rate,    "lfo3Rate",     "RATE");
    setupControl(lfo3Amount,  "lfo3Amount",   "DEPTH");

    setupControl(glideTime,  "glideTime",    "GLIDE");
    setupControl(masterVol,  "masterVolume", "VOLUME");

    // Voice Variation Knobs
    for (int i = 0; i < 6; ++i) {
        voiceKnobs[i].label.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        voiceKnobs[i].label.setColour(juce::Label::textColourId, juce::Colour(0xff14171a));
        voiceKnobs[i].label.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(voiceKnobs[i].label);

        voiceKnobs[i].slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        voiceKnobs[i].slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 16);
        voiceKnobs[i].slider.setWantsKeyboardFocus(false);
        voiceKnobs[i].slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff14171a));
        voiceKnobs[i].slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x00000000));
        voiceKnobs[i].slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
        addAndMakeVisible(voiceKnobs[i].slider);
    }

    auto setupBox = [this](juce::ComboBox& box, juce::Label& label, const juce::String& paramId, const juce::String& text, const juce::StringArray& items) {
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::FontOptions(10.5f).withStyle("Bold"));
        label.setColour(juce::Label::textColourId, juce::Colour(0xff14171a));
        label.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(label);

        box.addItemList(items, 1);
        box.setWantsKeyboardFocus(false);
        addAndMakeVisible(box);
        boxAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.apvts, paramId, box));
    };

    setupBox(voiceVarModeBox, voiceVarLabel, "voiceVarMode", "VOICE VAR", {"PANNING", "TUNING"});
    voiceVarModeBox.onChange = [this]() { updateVoiceKnobAttachments(); };
    updateVoiceKnobAttachments();

    cycleBtn.setClickingTogglesState(true);
    cycleBtn.setWantsKeyboardFocus(false);
    cycleBtn.setToggleState(audioProcessor.apvts.getRawParameterValue("cycleMode")->load() > 0.5f, juce::dontSendNotification);
    cycleBtn.onClick = [this]() {
        bool on = cycleBtn.getToggleState();
        if (auto* param = audioProcessor.apvts.getParameter("cycleMode"))
            param->setValueNotifyingHost(on ? 1.0f : 0.0f);
        repaint(655, 258, 250, 205);
    };
    addAndMakeVisible(cycleBtn);

    const juce::StringArray fullTargets = {
        "None", "LPF Cutoff", "LPF Reso", "HPF Cutoff", "Env Mod",
        "Filt Attack", "Filt Decay", "Filt Sustain", "Filt Release",
        "Amp Attack", "Amp Decay", "Amp Sustain", "Amp Release",
        "DCO1 Morph", "DCO1 PWM", "DCO2 Morph", "DCO2 PWM", "DCO2 Semi", "DCO2 Detune",
        "Sub Level", "Noise Level", "Glide Time", "Master Volume"
    };

    setupBox(lfo1ShapeBox, lfo1ShapeLabel, "lfo1Shape", "SHAPE", {"Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H"});
    setupBox(lfo2ShapeBox, lfo2ShapeLabel, "lfo2Shape", "SHAPE", {"Sine", "Triangle", "S&H"});
    setupBox(lfo2TargetBox, lfo2TargetLabel, "lfo2Target", "TARGET", fullTargets);
    setupBox(lfo3ShapeBox, lfo3ShapeLabel, "lfo3Shape", "SHAPE", {"Sine", "Triangle", "S&H"});
    setupBox(lfo3TargetBox, lfo3TargetLabel, "lfo3Target", "TARGET", fullTargets);

    // Clean Play Mode Choices: Poly, Mono, Unison
    setupBox(playModeBox, playModeLabel, "playMode", "PLAY MODE", {"Poly", "Mono", "Unison"});

    // --- MASTER FX CONTROLS ---
    setupBox(chorusModeBox, chorusLabel, "chorusMode", "STEREO CHORUS", {"Chorus OFF", "Chorus I", "Chorus II", "Chorus I+II"});

    const juce::StringArray syncTimings = {
        "1/32", "1/32t", "1/32d", "1/16", "1/16t", "1/16d",
        "1/8", "1/8t", "1/8d", "1/4", "1/4t", "1/4d",
        "1/2", "1/2t", "1/2d", "1/1", "FREE"
    };
    setupBox(delaySyncBox, delaySyncLabel, "delaySync", "DELAY TIME / SYNC", syncTimings);

    setupControl(delayTime, "delayTime", "TIME (FREE)");
    setupControl(delayFb,   "delayFeedback", "FEEDBACK");
    setupControl(delayDamp, "delayDamp", "DAMPING");
    setupControl(delayMix,  "delayMix",  "DELAY MIX");

    delayPingPongBtn.setWantsKeyboardFocus(false);
    addAndMakeVisible(delayPingPongBtn);
    btnAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "delayPingPong", delayPingPongBtn));

    setupControl(reverbSize, "reverbSize", "ROOM SIZE");
    setupControl(reverbDamp, "reverbDamp", "DAMPING");
    setupControl(reverbMix,  "reverbMix",  "REVERB MIX");

    // --- SEQUENCER & PERFORMANCE CONTROLS ---
    seqPlayBtn.setClickingTogglesState(true);
    seqPlayBtn.setWantsKeyboardFocus(false);
    seqPlayBtn.onClick = [this]() {
        audioProcessor.sequencer.isPlaying = seqPlayBtn.getToggleState();
        repaint(14, 470, 892, 46);
    };
    addAndMakeVisible(seqPlayBtn);

    seqRecBtn.setClickingTogglesState(true);
    seqRecBtn.setWantsKeyboardFocus(false);
    seqRecBtn.onClick = [this]() {
        audioProcessor.sequencer.isRecording = seqRecBtn.getToggleState();
        repaint(14, 470, 892, 46);
    };
    addAndMakeVisible(seqRecBtn);

    seqRestBtn.setWantsKeyboardFocus(false);
    seqRestBtn.onClick = [this]() {
        if (audioProcessor.sequencer.isRecording) {
            audioProcessor.sequencer.recordRest();
            repaint(14, 470, 892, 46);
        }
    };
    addAndMakeVisible(seqRestBtn);

    seqClearBtn.setWantsKeyboardFocus(false);
    seqClearBtn.onClick = [this]() {
        audioProcessor.sequencer.clearPattern();
        repaint(14, 470, 892, 46);
    };
    addAndMakeVisible(seqClearBtn);

    // Arp Module
    arpToggleBtn.setClickingTogglesState(true);
    arpToggleBtn.setWantsKeyboardFocus(false);
    arpToggleBtn.setToggleState(audioProcessor.apvts.getRawParameterValue("arpEnable")->load() > 0.5f, juce::dontSendNotification);
    arpToggleBtn.onClick = [this]() {
        bool on = arpToggleBtn.getToggleState();
        if (auto* param = audioProcessor.apvts.getParameter("arpEnable"))
            param->setValueNotifyingHost(on ? 1.0f : 0.0f);
        repaint(14, 470, 892, 46);
    };
    addAndMakeVisible(arpToggleBtn);

    currentArpIdx = static_cast<int>(audioProcessor.apvts.getRawParameterValue("arpMode")->load());

    arpPrevBtn.setWantsKeyboardFocus(false);
    arpPrevBtn.onClick = [this]() {
        currentArpIdx = (currentArpIdx + 4) % 5;
        if (auto* param = audioProcessor.apvts.getParameter("arpMode"))
            param->setValueNotifyingHost(static_cast<float>(currentArpIdx) / 4.0f);
        repaint(14, 470, 892, 46);
    };
    addAndMakeVisible(arpPrevBtn);

    arpNextBtn.setWantsKeyboardFocus(false);
    arpNextBtn.onClick = [this]() {
        currentArpIdx = (currentArpIdx + 1) % 5;
        if (auto* param = audioProcessor.apvts.getParameter("arpMode"))
            param->setValueNotifyingHost(static_cast<float>(currentArpIdx) / 4.0f);
        repaint(14, 470, 892, 46);
    };
    addAndMakeVisible(arpNextBtn);

    // Chord Module
    chordToggleBtn.setClickingTogglesState(true);
    chordToggleBtn.setWantsKeyboardFocus(false);
    chordToggleBtn.setToggleState(audioProcessor.apvts.getRawParameterValue("chordEnable")->load() > 0.5f, juce::dontSendNotification);
    chordToggleBtn.onClick = [this]() {
        bool on = chordToggleBtn.getToggleState();
        if (auto* param = audioProcessor.apvts.getParameter("chordEnable"))
            param->setValueNotifyingHost(on ? 1.0f : 0.0f);
        repaint(14, 470, 892, 46);
    };
    addAndMakeVisible(chordToggleBtn);

    currentChordIdx = static_cast<int>(audioProcessor.apvts.getRawParameterValue("chordType")->load());

    chordPrevBtn.setWantsKeyboardFocus(false);
    chordPrevBtn.onClick = [this]() {
        currentChordIdx = (currentChordIdx + 8) % 9;
        if (auto* param = audioProcessor.apvts.getParameter("chordType"))
            param->setValueNotifyingHost(static_cast<float>(currentChordIdx) / 8.0f);
        repaint(14, 470, 892, 46);
    };
    addAndMakeVisible(chordPrevBtn);

    chordNextBtn.setWantsKeyboardFocus(false);
    chordNextBtn.onClick = [this]() {
        currentChordIdx = (currentChordIdx + 1) % 9;
        if (auto* param = audioProcessor.apvts.getParameter("chordType"))
            param->setValueNotifyingHost(static_cast<float>(currentChordIdx) / 8.0f);
        repaint(14, 470, 892, 46);
    };
    addAndMakeVisible(chordNextBtn);

    // Steps Module
    currentPagesIdx = static_cast<int>(audioProcessor.apvts.getRawParameterValue("seqPages")->load());
    audioProcessor.sequencer.setNumPages(currentPagesIdx + 1);

    stepsPrevBtn.setWantsKeyboardFocus(false);
    stepsPrevBtn.onClick = [this]() {
        currentPagesIdx = (currentPagesIdx + 3) % 4;
        audioProcessor.sequencer.setNumPages(currentPagesIdx + 1);
        if (auto* param = audioProcessor.apvts.getParameter("seqPages"))
            param->setValueNotifyingHost(static_cast<float>(currentPagesIdx) / 3.0f);
        if (currentSeqPage > currentPagesIdx) currentSeqPage = currentPagesIdx;
        repaint(14, 470, 892, 46);
    };
    addAndMakeVisible(stepsPrevBtn);

    stepsNextBtn.setWantsKeyboardFocus(false);
    stepsNextBtn.onClick = [this]() {
        currentPagesIdx = (currentPagesIdx + 1) % 4;
        audioProcessor.sequencer.setNumPages(currentPagesIdx + 1);
        if (auto* param = audioProcessor.apvts.getParameter("seqPages"))
            param->setValueNotifyingHost(static_cast<float>(currentPagesIdx) / 3.0f);
        repaint(14, 470, 892, 46);
    };
    addAndMakeVisible(stepsNextBtn);

    // Page Module
    pagePrevBtn.setWantsKeyboardFocus(false);
    pagePrevBtn.onClick = [this]() {
        int maxPages = audioProcessor.sequencer.getNumPages();
        currentSeqPage = (currentSeqPage + maxPages - 1) % maxPages;
        audioProcessor.sequencer.setRecordStep(currentSeqPage * 16);
        repaint(14, 470, 892, 46);
    };
    addAndMakeVisible(pagePrevBtn);

    pageNextBtn.setWantsKeyboardFocus(false);
    pageNextBtn.onClick = [this]() {
        int maxPages = audioProcessor.sequencer.getNumPages();
        currentSeqPage = (currentSeqPage + 1) % maxPages;
        audioProcessor.sequencer.setRecordStep(currentSeqPage * 16);
        repaint(14, 470, 892, 46);
    };
    addAndMakeVisible(pageNextBtn);

    addAndMakeVisible(buttonKeyboard);

    // Add Modal Save Patch Dialog (starts hidden)
    addChildComponent(saveDialog);

    setWantsKeyboardFocus(true);
    addKeyListener(this);
    startTimerHz(60);
}

Simple106AudioProcessorEditor::~Simple106AudioProcessorEditor() {
    stopTimer();
    removeKeyListener(this);
    setLookAndFeel(nullptr);
}

void Simple106AudioProcessorEditor::showSaveDialog() {
    saveDialog.setVisible(true);
    saveDialog.toFront(true);
    saveDialog.open();
    resized();
}

void Simple106AudioProcessorEditor::hideSaveDialog() {
    saveDialog.setVisible(false);
    grabKeyboardFocus();
    repaint();
}

void Simple106AudioProcessorEditor::performSave(const juce::String& rawName) {
    auto cleanName = juce::File::createLegalFileName(rawName.trim());
    if (cleanName.isEmpty()) cleanName = "Custom_Patch";

    auto file = audioProcessor.getPresetsDirectory().getChildFile(cleanName + ".s106");
    file.replaceWithText(audioProcessor.createPatchXml(cleanName));

    hideSaveDialog();
    refreshPresetList();

    for (int i = 0; i < presetBox.getNumItems(); ++i) {
        if (presetBox.getItemText(i) == cleanName) {
            presetBox.setSelectedId(presetBox.getItemId(i), juce::dontSendNotification);
            break;
        }
    }
}

void Simple106AudioProcessorEditor::refreshPresetList() {
    presetBox.clear(juce::dontSendNotification);

    // 1. Factory Presets
    const char* factoryNames[] = {
        "01. Init Poly", "02. Juno Warm Pad", "03. Poly Drift Keys", "04. Supersaw Hoover",
        "05. Acid 106 Lead", "06. 80s Arp Dream", "07. MonoPoly Drift", "08. Space Pluck"
    };
    for (int i = 0; i < 8; ++i) {
        presetBox.addItem(factoryNames[i], i + 1);
    }

    presetBox.addSeparator();

    // 2. User Presets
    auto dir = audioProcessor.getPresetsDirectory();
    auto userFiles = dir.findChildFiles(juce::File::findFiles, false, "*.s106");
    int userIdx = 9;
    for (const auto& file : userFiles) {
        presetBox.addItem(file.getFileNameWithoutExtension(), userIdx++);
    }
    presetBox.setSelectedId(1, juce::dontSendNotification);
}

void Simple106AudioProcessorEditor::renderBackgroundCache() {
    backgroundCache = juce::Image(juce::Image::RGB, getWidth(), getHeight(), true);
    juce::Graphics g(backgroundCache);

    auto fullBounds = getLocalBounds().toFloat();

    // Chassis Outer Frame
    g.setColour(juce::Colour(0xff23272e));
    g.fillRoundedRectangle(fullBounds, 8.0f);
    g.setColour(juce::Colour(0xff4a5260));
    g.drawRoundedRectangle(fullBounds.reduced(0.5f), 8.0f, 1.5f);

    // Inner Faceplate Panel
    auto faceplate = fullBounds.reduced(6.0f);
    juce::ColourGradient plateGrad(juce::Colour(0xffe0e3e8), faceplate.getX(), faceplate.getY(),
                                   juce::Colour(0xffcbcfd6), faceplate.getRight(), faceplate.getBottom(), false);
    g.setGradientFill(plateGrad);
    g.fillRoundedRectangle(faceplate, 4.0f);

    // Corner Screws
    auto drawScrew = [&](float cx, float cy) {
        g.setColour(juce::Colour(0xff717986));
        g.fillEllipse(cx - 3.5f, cy - 3.5f, 7.0f, 7.0f);
        g.setColour(juce::Colour(0xff4a505b));
        g.drawEllipse(cx - 3.5f, cy - 3.5f, 7.0f, 7.0f, 0.8f);
        g.drawLine(cx - 2.2f, cy, cx + 2.2f, cy, 1.0f);
    };
    drawScrew(12.0f, 12.0f);
    drawScrew(getWidth() - 12.0f, 12.0f);
    drawScrew(12.0f, getHeight() - 12.0f);
    drawScrew(getWidth() - 12.0f, getHeight() - 12.0f);

    // Header Area
    auto headerArea = juce::Rectangle<float>(faceplate.getX(), faceplate.getY(), faceplate.getWidth(), 42.0f);
    g.setColour(juce::Colour(0xff1f242b));
    g.fillRect(headerArea);

    g.setColour(juce::Colour(0xffe74c3c));
    g.fillRect(faceplate.getX(), headerArea.getBottom(), faceplate.getWidth(), 3.0f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(19.0f).withStyle("Bold"));
    g.drawText("SIMPLE 106", 24, static_cast<int>(headerArea.getY()), 130, static_cast<int>(headerArea.getHeight()), juce::Justification::centredLeft);

    // Header Labels (PATCH & COLOUR)
    g.setColour(juce::Colour(0xffa5b1c2));
    g.setFont(juce::FontOptions(10.5f).withStyle("Bold"));
    g.drawText("PATCH", 160, static_cast<int>(headerArea.getY()), 45, static_cast<int>(headerArea.getHeight()), juce::Justification::centredRight);
    g.drawText("COLOUR", 508, static_cast<int>(headerArea.getY()), 52, static_cast<int>(headerArea.getHeight()), juce::Justification::centredRight);

    auto drawSection = [&](juce::Rectangle<int> bounds, const juce::String& title, juce::Colour accent) {
        g.setColour(juce::Colour(0xffbac0cc));
        g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.5f);

        auto titleBox = bounds.removeFromTop(20).reduced(2, 0);
        g.setColour(accent);
        g.fillRect(titleBox.getX(), titleBox.getY() + 1, 4, 16);

        g.setColour(juce::Colour(0xff14171a));
        g.setFont(juce::FontOptions(11.5f).withStyle("Bold"));
        g.drawText(title, titleBox.getX() + 8, titleBox.getY(), titleBox.getWidth() - 8, 20, juce::Justification::centredLeft);
    };

    if (currentTab == 0) {
        drawSection({14, 56, 350, 195}, "DCO 1 & DCO 2", juce::Colour(0xff2980b9));
        drawSection({372, 56, 275, 195}, "HPF & 24dB VCF", juce::Colour(0xffe67e22));
        drawSection({655, 56, 250, 195}, "ENVELOPES", juce::Colour(0xff27ae60));

        drawSection({14, 258, 200, 205}, "SUB & NOISE", juce::Colour(0xff8e44ad));
        drawSection({222, 258, 425, 205}, "MODULATION (LFO 1, 2 & 3)", juce::Colour(0xffd35400));
        drawSection({655, 258, 250, 205}, "VOICE VARIATION MATRIX", juce::Colour(0xffc0392b));

        // Sub-column dividers inside Modulation section
        g.setColour(juce::Colour(0xffbac0cc));
        g.drawVerticalLine(364, 280.0f, 452.0f);
        g.drawVerticalLine(502, 280.0f, 452.0f);
    } else {
        drawSection({14, 56, 280, 407}, "STEREO CHORUS", juce::Colour(0xff8e44ad));
        drawSection({304, 56, 300, 407}, "TEMPO-SYNC PING-PONG DELAY", juce::Colour(0xff2980b9));
        drawSection({614, 56, 290, 407}, "LUSH DIGITAL REVERB", juce::Colour(0xff27ae60));
    }

    // --- SEQUENCER TRAY BACKGROUND ---
    auto seqBar = juce::Rectangle<int>(14, 472, 892, 42);
    g.setColour(juce::Colour(0xff1a1d24));
    g.fillRoundedRectangle(seqBar.toFloat(), 4.0f);
    g.setColour(juce::Colour(0xff4a505b));
    g.drawRoundedRectangle(seqBar.toFloat(), 4.0f, 1.2f);

    auto drawScreenBox = [&](juce::Rectangle<float> rect) {
        g.setColour(juce::Colour(0xff090b0e));
        g.fillRoundedRectangle(rect, 3.0f);
        g.setColour(juce::Colour(0xff2a303a));
        g.drawRoundedRectangle(rect, 3.0f, 1.2f);
    };

    g.setColour(juce::Colour(0xff98a2b3));
    g.setFont(juce::FontOptions(9.5f).withStyle("Bold"));
    g.drawText("STEPS", 574, 479, 36, 28, juce::Justification::centredRight);
    g.drawText("PAGE", 730, 479, 34, 28, juce::Justification::centredRight);

    drawScreenBox({274.0f, 479.0f, 60.0f, 28.0f}); // Arp
    drawScreenBox({438.0f, 479.0f, 74.0f, 28.0f}); // Chord
    drawScreenBox({632.0f, 479.0f, 44.0f, 28.0f}); // Steps
    drawScreenBox({786.0f, 479.0f, 66.0f, 28.0f}); // Page
}

void Simple106AudioProcessorEditor::focusLost(FocusChangeType) {
    for (size_t i = 0; i < NUM_QWERTY_KEYS; ++i) {
        if (qwertyDownState[i]) {
            qwertyDownState[i] = false;
            audioProcessor.keyboardState.noteOff(1, qwertyActiveNote[i], 0.0f);
        }
    }
}

void Simple106AudioProcessorEditor::timerCallback() {
    buttonKeyboard.setSequencerState(
        audioProcessor.sequencer.isPlaying,
        audioProcessor.sequencer.isRecording,
        audioProcessor.sequencer.getCurrentStep(),
                                     audioProcessor.sequencer.getRecordStep(),
                                     currentSeqPage
    );

    // Instant Physical/Active Key Feedback (Zero Lag, No Release-Tail Hanging)
    std::array<bool, 25> activeKeys { false };
    int currentStart = 48 + buttonKeyboard.getOctaveOffset();

    // 1. Direct Physical Key Checks from KeyboardState
    for (int i = 0; i < 25; ++i) {
        int noteNum = currentStart + i;
        if (audioProcessor.keyboardState.isNoteOn(1, noteNum)) {
            activeKeys[static_cast<size_t>(i)] = true;
        }
    }

    // 2. Chord Mode Visual Key Illuminations for Active Physical Keys
    bool chordOn = audioProcessor.apvts.getRawParameterValue("chordEnable")->load() > 0.5f;
    int chordIdx = static_cast<int>(audioProcessor.apvts.getRawParameterValue("chordType")->load());
    if (chordOn) {
        for (int i = 0; i < 25; ++i) {
            int noteNum = currentStart + i;
            if (audioProcessor.keyboardState.isNoteOn(1, noteNum)) {
                auto chordNotes = audioProcessor.getChordNotes(noteNum, chordIdx);
                for (int n : chordNotes) {
                    int idx = n - currentStart;
                    if (idx >= 0 && idx < 25) {
                        activeKeys[static_cast<size_t>(idx)] = true;
                    }
                }
            }
        }
    }

    buttonKeyboard.setActiveNotes(activeKeys);

    // QWERTY Physical Key Release Check
    if (!saveDialog.isVisible()) {
        for (size_t i = 0; i < NUM_QWERTY_KEYS; ++i) {
            bool isDown = juce::KeyPress::isKeyCurrentlyDown(qwertyMappings[i].keyCode);
            if (qwertyDownState[i] && !isDown) {
                qwertyDownState[i] = false;
                audioProcessor.keyboardState.noteOff(1, qwertyActiveNote[i], 0.0f);
            }
        }
    }

    repaint(14, 470, 892, 46);
    if (currentTab == 0) {
        repaint(655, 258, 250, 205);
    }
}

void Simple106AudioProcessorEditor::updateVoiceKnobAttachments() {
    voiceAttachments.clear();
    bool isTune = (voiceVarModeBox.getSelectedItemIndex() == 1);

    for (int i = 0; i < 6; ++i) {
        juce::String paramId = (isTune ? "vTune" : "vPan") + juce::String(i + 1);
        voiceKnobs[i].label.setText("V" + juce::String(i + 1), juce::dontSendNotification);

        if (isTune) {
            voiceKnobs[i].slider.textFromValueFunction = [](double v) { return juce::String((int)v) + " ct"; };
        } else {
            voiceKnobs[i].slider.textFromValueFunction = [](double v) {
                if (std::abs(v) < 0.05) return juce::String("MID");
                return (v < 0) ? "L" + juce::String((int)(-v * 100)) : "R" + juce::String((int)(v * 100));
            };
        }

        voiceAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, paramId, voiceKnobs[i].slider));
        voiceKnobs[i].slider.updateText();
    }
}

void Simple106AudioProcessorEditor::setupControl(LabeledSlider& ctrl, const juce::String& paramId, const juce::String& labelText) {
    ctrl.label.setText(labelText, juce::dontSendNotification);
    ctrl.label.setFont(juce::FontOptions(10.5f).withStyle("Bold"));
    ctrl.label.setColour(juce::Label::textColourId, juce::Colour(0xff14171a));
    ctrl.label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(ctrl.label);

    ctrl.slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    ctrl.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 16);
    ctrl.slider.setWantsKeyboardFocus(false);

    ctrl.slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff14171a));
    ctrl.slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x00000000));
    ctrl.slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
    addAndMakeVisible(ctrl.slider);

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, paramId, ctrl.slider));
}

void Simple106AudioProcessorEditor::parentHierarchyChanged() {
    if (isShowing()) grabKeyboardFocus();
}

void Simple106AudioProcessorEditor::mouseDown(const juce::MouseEvent&) {
    if (!saveDialog.isVisible()) grabKeyboardFocus();
}

bool Simple106AudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component*) {
    if (saveDialog.isVisible()) return false;

    int keyCode = key.getKeyCode();
    if (keyCode == 'Z' || keyCode == 'z') {
        buttonKeyboard.shiftOctave(-1);
        return true;
    }
    if (keyCode == 'X' || keyCode == 'x') {
        buttonKeyboard.shiftOctave(1);
        return true;
    }

    if (keyCode >= 'a' && keyCode <= 'z') {
        keyCode -= ('a' - 'A');
    }

    int baseMidi = 48 + buttonKeyboard.getOctaveOffset();

    for (size_t i = 0; i < NUM_QWERTY_KEYS; ++i) {
        if (keyCode == qwertyMappings[i].keyCode) {
            if (!qwertyDownState[i]) {
                qwertyDownState[i] = true;
                qwertyActiveNote[i] = baseMidi + qwertyMappings[i].semitoneOffset;
                audioProcessor.keyboardState.noteOn(1, qwertyActiveNote[i], 1.0f);
            }
            return true;
        }
    }
    return false;
}

bool Simple106AudioProcessorEditor::keyStateChanged(bool /*isKeyDown*/, juce::Component*) {
    if (saveDialog.isVisible()) return false;

    for (size_t i = 0; i < NUM_QWERTY_KEYS; ++i) {
        bool isDown = juce::KeyPress::isKeyCurrentlyDown(qwertyMappings[i].keyCode);
        if (qwertyDownState[i] && !isDown) {
            qwertyDownState[i] = false;
            audioProcessor.keyboardState.noteOff(1, qwertyActiveNote[i], 0.0f);
        }
    }
    return true;
}

void Simple106AudioProcessorEditor::paint(juce::Graphics& g) {
    if (backgroundCache.isValid()) {
        g.drawImageAt(backgroundCache, 0, 0);
    }

    if (currentTab == 0) {
        for (int i = 0; i < 6; ++i) {
            auto b = voiceKnobs[i].label.getBounds();
            float ledX = static_cast<float>(b.getRight()) - 10.0f;
            float ledY = static_cast<float>(b.getCentreY());
            bool voiceIsSounding = audioProcessor.isVoiceActive(i);
            drawVoiceLED(g, ledX, ledY, voiceIsSounding);
        }
    }

    // Dynamic 14-Segment Themed LED Screens
    const char* arpNames[] = { "UP", "DOWN", "U-D", "RAND", "PLAY" };
    drawSegmentedString(g, 280.0f, 483.0f, 12.0f, 20.0f, arpNames[currentArpIdx]);

    const char* chordNames[] = { "MAJ", "MIN", "MAJ7", "MIN7", "DOM7", "SUS4", "DIM", "OCT", "5TH" };
    drawSegmentedString(g, 446.0f, 483.0f, 13.0f, 20.0f, chordNames[currentChordIdx]);

    const char* stepNames[] = { "16", "32", "48", "64" };
    drawSegmentedString(g, 638.0f, 483.0f, 13.0f, 20.0f, stepNames[currentPagesIdx]);

    juce::String pageStr = "PG-" + juce::String(currentSeqPage + 1);
    drawSegmentedString(g, 792.0f, 483.0f, 13.0f, 20.0f, pageStr);

    // Status LEDs on Buttons (Themed)
    auto theme = getActiveTheme();
    auto drawBtnLED = [&](juce::Rectangle<int> b, bool on) {
        float cx = static_cast<float>(b.getX()) + 7.0f;
        float cy = static_cast<float>(b.getY()) + 7.0f;
        float r = 2.4f;
        if (on) {
            g.setColour(theme.glow);
            g.fillEllipse(cx - r - 2.0f, cy - r - 2.0f, (r + 2.0f) * 2.0f, (r + 2.0f) * 2.0f);
            g.setColour(theme.primary);
            g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
            g.setColour(theme.highlight);
            g.fillEllipse(cx - r * 0.45f, cy - r * 0.45f, r * 0.9f, r * 0.9f);
        } else {
            g.setColour(theme.unlit);
            g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
        }
    };

    drawBtnLED(seqPlayBtn.getBounds(), seqPlayBtn.getToggleState());
    drawBtnLED(seqRecBtn.getBounds(), seqRecBtn.getToggleState());
    drawBtnLED(arpToggleBtn.getBounds(), arpToggleBtn.getToggleState());
    drawBtnLED(chordToggleBtn.getBounds(), chordToggleBtn.getToggleState());
    if (currentTab == 0) {
        drawBtnLED(cycleBtn.getBounds(), cycleBtn.getToggleState());
    }
}

void Simple106AudioProcessorEditor::drawSegmentedString(juce::Graphics& g, float x, float y, float w, float h, const juce::String& text) {
    float startX = x;
    for (int i = 0; i < text.length(); ++i) {
        draw14SegmentChar(g, startX, y, w, h, text[i]);
        startX += (w + 2.0f);
    }
}

void Simple106AudioProcessorEditor::draw14SegmentChar(juce::Graphics& g, float x, float y, float w, float h, char c) {
    bool s[7] = { false };
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    switch (c) {
        case '0': s[0] = s[1] = s[2] = s[3] = s[4] = s[5] = true; break;
        case '1': s[1] = s[2] = true; break;
        case '2': s[0] = s[1] = s[3] = s[4] = s[6] = true; break;
        case '3': s[0] = s[1] = s[2] = s[3] = s[6] = true; break;
        case '4': s[1] = s[2] = s[5] = s[6] = true; break;
        case '5': s[0] = s[2] = s[3] = s[5] = s[6] = true; break;
        case '6': s[0] = s[2] = s[3] = s[4] = s[5] = s[6] = true; break;
        case '7': s[0] = s[1] = s[2] = true; break;
        case '8': s[0] = s[1] = s[2] = s[3] = s[4] = s[5] = s[6] = true; break;
        case '9': s[0] = s[1] = s[2] = s[3] = s[5] = s[6] = true; break;
        case 'P': s[0] = s[1] = s[4] = s[5] = s[6] = true; break;
        case 'G': s[0] = s[2] = s[3] = s[4] = s[5] = true; break;
        case '-': s[6] = true; break;
        case 'M': s[0] = s[1] = s[2] = s[4] = s[5] = true; break;
        case 'A': s[0] = s[1] = s[2] = s[4] = s[5] = s[6] = true; break;
        case 'J': s[1] = s[2] = s[3] = s[4] = true; break;
        case 'I': s[1] = s[2] = true; break;
        case 'N': s[2] = s[4] = s[6] = true; break;
        case 'D': s[1] = s[2] = s[3] = s[4] = s[6] = true; break;
        case 'O': s[0] = s[1] = s[2] = s[3] = s[4] = s[5] = true; break;
        case 'S': s[0] = s[2] = s[3] = s[5] = s[6] = true; break;
        case 'U': s[1] = s[2] = s[3] = s[4] = s[5] = true; break;
        case 'T': s[0] = s[4] = s[5] = s[6] = true; break;
        case 'H': s[1] = s[2] = s[4] = s[5] = s[6] = true; break;
        case 'C': s[0] = s[3] = s[4] = s[5] = true; break;
        case 'E': s[0] = s[3] = s[4] = s[5] = s[6] = true; break;
        case 'R': s[4] = s[6] = true; break;
        case 'W': s[1] = s[2] = s[3] = s[4] = s[5] = true; break;
        case 'Y': s[1] = s[2] = s[3] = s[5] = s[6] = true; break;
        case 'L': s[3] = s[4] = s[5] = true; break;
        default: break;
    }

    float t = 1.8f;
    float midY = y + h * 0.5f;
    auto theme = getActiveTheme();

    auto drawSeg = [&](int idx, juce::Rectangle<float> rect) {
        if (s[idx]) {
            g.setColour(theme.glow);
            g.fillRoundedRectangle(rect.expanded(1.2f), 1.0f);
            g.setColour(theme.primary);
            g.fillRoundedRectangle(rect, 1.0f);
            g.setColour(theme.highlight);
            g.fillRoundedRectangle(rect.reduced(0.4f), 0.5f);
        } else {
            g.setColour(theme.unlitBg);
            g.fillRoundedRectangle(rect, 0.5f);
        }
    };

    drawSeg(0, { x + t, y, w - 2 * t, t });
    drawSeg(1, { x + w - t, y + t, t, (h * 0.5f) - t - 0.5f });
    drawSeg(2, { x + w - t, midY + 0.5f, t, (h * 0.5f) - t - 0.5f });
    drawSeg(3, { x + t, y + h - t, w - 2 * t, t });
    drawSeg(4, { x, midY + 0.5f, t, (h * 0.5f) - t - 0.5f });
    drawSeg(5, { x, y + t, t, (h * 0.5f) - t - 0.5f });
    drawSeg(6, { x + t, midY - (t * 0.5f), w - 2 * t, t });
}

void Simple106AudioProcessorEditor::drawVoiceLED(juce::Graphics& g, float cx, float cy, bool on) {
    float r = 3.0f;
    auto theme = getActiveTheme();
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
        g.setColour(juce::Colour(0xff2a080c));
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 0.8f);
    }
}

void Simple106AudioProcessorEditor::resized() {
    renderBackgroundCache();

    // --- HEADER CONTROLS ---
    presetBox.setBounds(210, 8, 150, 26);
    savePresetBtn.setBounds(364, 8, 44, 26);
    initPresetBtn.setBounds(412, 8, 40, 26);

    themeBox.setBounds(565, 8, 80, 26);

    synthTabBtn.setBounds(660, 8, 115, 26);
    fxTabBtn.setBounds(780, 8, 105, 26);

    saveDialog.setBounds(getLocalBounds());

    auto layoutKnob = [](LabeledSlider& ctrl, int x, int y, int w = 58, int h = 60) {
        ctrl.label.setBounds(x, y, w, 14);
        ctrl.slider.setBounds(x, y + 14, w, h);
    };

    bool isSynth = (currentTab == 0);

    auto setSynthVisible = [this](bool v) {
        dco1Morph.slider.setVisible(v); dco1PWM.slider.setVisible(v); dco1Level.slider.setVisible(v);
        dco1Morph.label.setVisible(v); dco1PWM.label.setVisible(v); dco1Level.label.setVisible(v);
        dco2Morph.slider.setVisible(v); dco2PWM.slider.setVisible(v); dco2Level.slider.setVisible(v);
        dco2Semi.slider.setVisible(v); dco2Cents.slider.setVisible(v);
        dco2Morph.label.setVisible(v); dco2PWM.label.setVisible(v); dco2Level.label.setVisible(v);
        dco2Semi.label.setVisible(v); dco2Cents.label.setVisible(v);

        hpfCutoff.slider.setVisible(v); lpfCutoff.slider.setVisible(v); lpfRes.slider.setVisible(v); envMod.slider.setVisible(v);
        hpfCutoff.label.setVisible(v); lpfCutoff.label.setVisible(v); lpfRes.label.setVisible(v); envMod.label.setVisible(v);

        filtA.slider.setVisible(v); filtD.slider.setVisible(v); filtS.slider.setVisible(v); filtR.slider.setVisible(v);
        filtA.label.setVisible(v); filtD.label.setVisible(v); filtS.label.setVisible(v); filtR.label.setVisible(v);

        ampA.slider.setVisible(v); ampD.slider.setVisible(v); ampS.slider.setVisible(v); ampR.slider.setVisible(v);
        ampA.label.setVisible(v); ampD.label.setVisible(v); ampS.label.setVisible(v); ampR.label.setVisible(v);

        subLevel.slider.setVisible(v); noiseLevel.slider.setVisible(v); glideTime.slider.setVisible(v);
        subLevel.label.setVisible(v); noiseLevel.label.setVisible(v); glideTime.label.setVisible(v);

        lfo1Rate.slider.setVisible(v); lfo1ToFilt.slider.setVisible(v); lfo1ToPitch.slider.setVisible(v);
        lfo1Rate.label.setVisible(v); lfo1ToFilt.label.setVisible(v); lfo1ToPitch.label.setVisible(v);

        lfo2Rate.slider.setVisible(v); lfo2Amount.slider.setVisible(v);
        lfo2Rate.label.setVisible(v); lfo2Amount.label.setVisible(v);

        lfo3Rate.slider.setVisible(v); lfo3Amount.slider.setVisible(v);
        lfo3Rate.label.setVisible(v); lfo3Amount.label.setVisible(v);

        lfo1ShapeBox.setVisible(v); lfo1ShapeLabel.setVisible(v);
        lfo2ShapeBox.setVisible(v); lfo2ShapeLabel.setVisible(v);
        lfo2TargetBox.setVisible(v); lfo2TargetLabel.setVisible(v);

        lfo3ShapeBox.setVisible(v); lfo3ShapeLabel.setVisible(v);
        lfo3TargetBox.setVisible(v); lfo3TargetLabel.setVisible(v);

        voiceVarModeBox.setVisible(v); voiceVarLabel.setVisible(v);
        cycleBtn.setVisible(v);
        for (int i = 0; i < 6; ++i) {
            voiceKnobs[i].slider.setVisible(v);
            voiceKnobs[i].label.setVisible(v);
        }

        playModeBox.setVisible(v); playModeLabel.setVisible(v);
        masterVol.slider.setVisible(v); masterVol.label.setVisible(v);
    };

    auto setFXVisible = [this](bool v) {
        chorusLabel.setVisible(v); chorusModeBox.setVisible(v);
        delaySyncLabel.setVisible(v); delaySyncBox.setVisible(v);
        delayTime.slider.setVisible(v); delayFb.slider.setVisible(v); delayDamp.slider.setVisible(v); delayMix.slider.setVisible(v);
        delayTime.label.setVisible(v); delayFb.label.setVisible(v); delayDamp.label.setVisible(v); delayMix.label.setVisible(v);
        delayPingPongBtn.setVisible(v);

        reverbSize.slider.setVisible(v); reverbDamp.slider.setVisible(v); reverbMix.slider.setVisible(v);
        reverbSize.label.setVisible(v); reverbDamp.label.setVisible(v); reverbMix.label.setVisible(v);
    };

    setSynthVisible(isSynth);
    setFXVisible(!isSynth);

    if (isSynth) {
        layoutKnob(dco1Morph, 24, 78, 62); layoutKnob(dco1PWM, 88, 78); layoutKnob(dco1Level, 148, 78);
        layoutKnob(dco2Morph, 24, 162, 62); layoutKnob(dco2PWM, 88, 162); layoutKnob(dco2Semi, 148, 162); layoutKnob(dco2Cents, 208, 162); layoutKnob(dco2Level, 268, 162);

        layoutKnob(hpfCutoff, 385, 78); layoutKnob(lpfCutoff, 448, 78); layoutKnob(lpfRes, 511, 78); layoutKnob(envMod, 574, 78);
        layoutKnob(filtA, 385, 162); layoutKnob(filtD, 448, 162); layoutKnob(filtS, 511, 162); layoutKnob(filtR, 574, 162);

        layoutKnob(ampA, 668, 78); layoutKnob(ampD, 728, 78); layoutKnob(ampS, 788, 78); layoutKnob(ampR, 848, 78);
        playModeLabel.setBounds(668, 162, 70, 14); playModeBox.setBounds(668, 178, 110, 24);
        layoutKnob(masterVol, 830, 162, 64, 68);

        layoutKnob(subLevel, 24, 282); layoutKnob(noiseLevel, 88, 282); layoutKnob(glideTime, 152, 282);

        // LFO 1
        lfo1ShapeLabel.setBounds(234, 280, 120, 14);
        lfo1ShapeBox.setBounds(234, 296, 120, 22);
        layoutKnob(lfo1Rate, 234, 326, 56, 56);
        layoutKnob(lfo1ToFilt, 298, 326, 56, 56);
        layoutKnob(lfo1ToPitch, 266, 388, 56, 56);

        // LFO 2
        lfo2ShapeLabel.setBounds(372, 280, 120, 14);
        lfo2ShapeBox.setBounds(372, 296, 120, 22);
        lfo2TargetLabel.setBounds(372, 322, 120, 14);
        lfo2TargetBox.setBounds(372, 338, 120, 22);
        layoutKnob(lfo2Rate, 372, 376, 56, 56);
        layoutKnob(lfo2Amount, 436, 376, 56, 56);

        // LFO 3
        lfo3ShapeLabel.setBounds(510, 280, 120, 14);
        lfo3ShapeBox.setBounds(510, 296, 120, 22);
        lfo3TargetLabel.setBounds(510, 322, 120, 14);
        lfo3TargetBox.setBounds(510, 338, 120, 22);
        layoutKnob(lfo3Rate, 510, 376, 56, 56);
        layoutKnob(lfo3Amount, 574, 376, 56, 56);

        // Voice Variation Matrix
        voiceVarLabel.setBounds(668, 280, 68, 16);
        voiceVarModeBox.setBounds(738, 276, 94, 24);
        cycleBtn.setBounds(836, 276, 56, 24);

        layoutKnob(voiceKnobs[0], 668, 310, 72, 58);
        layoutKnob(voiceKnobs[1], 744, 310, 72, 58);
        layoutKnob(voiceKnobs[2], 820, 310, 72, 58);

        layoutKnob(voiceKnobs[3], 668, 384, 72, 58);
        layoutKnob(voiceKnobs[4], 744, 384, 72, 58);
        layoutKnob(voiceKnobs[5], 820, 384, 72, 58);
    } else {
        // Master FX Page
        chorusLabel.setBounds(30, 90, 120, 16); chorusModeBox.setBounds(30, 115, 200, 30);

        delaySyncLabel.setBounds(320, 90, 140, 16); delaySyncBox.setBounds(320, 110, 140, 26);
        layoutKnob(delayTime, 470, 90, 65, 70);
        layoutKnob(delayFb, 320, 185, 65, 70); layoutKnob(delayDamp, 395, 185, 65, 70); layoutKnob(delayMix, 470, 185, 65, 70);
        delayPingPongBtn.setBounds(470, 265, 110, 30);

        layoutKnob(reverbSize, 630, 90, 65, 70); layoutKnob(reverbDamp, 705, 90, 65, 70);
        layoutKnob(reverbMix, 780, 90, 65, 70);
    }

    // --- SEQUENCER & PERFORMANCE CONTROL BAR ---
    seqPlayBtn.setBounds(22, 479, 44, 28);
    seqRecBtn.setBounds(68, 479, 44, 28);
    seqRestBtn.setBounds(114, 479, 42, 28);
    seqClearBtn.setBounds(158, 479, 38, 28);

    // Arp Module (x: 208..330)
    arpToggleBtn.setBounds(212, 479, 46, 28);
    arpPrevBtn.setBounds(260, 480, 16, 26);
    arpNextBtn.setBounds(332, 480, 16, 26);

    // Chord Module (x: 360..500)
    chordToggleBtn.setBounds(364, 479, 56, 28);
    chordPrevBtn.setBounds(422, 480, 16, 26);
    chordNextBtn.setBounds(510, 480, 16, 26);

    // Steps Module (x: 540..660)
    stepsPrevBtn.setBounds(614, 480, 16, 26);
    stepsNextBtn.setBounds(678, 480, 16, 26);

    // Page Module (x: 690..840)
    pagePrevBtn.setBounds(768, 480, 16, 26);
    pageNextBtn.setBounds(854, 480, 16, 26);

    buttonKeyboard.setBounds(14, 518, 892, 114);
}
