#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <set>
#include <string>
#include <utility>

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

namespace {
    std::atomic<int> activeEditorInstances { 0 };

    constexpr int kMinW = 1080;
    constexpr int kMinH = 840;
    constexpr int kMaxW = 1280;
    constexpr int kMaxH = 960;
    constexpr int kSideMargin = 36;
    constexpr int kTopMargin = 14;
    constexpr int kHeaderH = 72;
    constexpr int kHeaderBodyGap = 10;
    constexpr int kBodyFooterGap = 10;
    constexpr int kFooterH = 182;
    constexpr int kBottomMargin = 16;
    constexpr int kRowGap = 10;
    constexpr int kTopRowMin = 200;
    constexpr int kBottomRowMin = 310;
    constexpr int kSmallBody = 42;
    constexpr int kPanelTitleFont = 16;

    const char* const kBaselineParamIds[] = {
        "ampAttack", "ampDecay", "ampRelease", "ampSustain", "arpEnable", "arpMode",
        "chordEnable", "chordType", "chorusMode", "cycleMode", "dco1Fold", "dco1Level",
        "dco1Morph", "dco1Octave", "dco1PWM", "dco2Cents", "dco2Level", "dco2Morph",
        "dco2PWM", "dco2Semi", "delayDamp", "delayFeedback", "delayMix", "delayPingPong",
        "delaySync", "delayTime", "envMod", "filtAttack", "filtDecay", "filtRelease",
        "filtSustain", "glideTime", "guiTheme", "hpfCutoff", "ledTheme", "lfo1Rate",
        "lfo1Shape", "lfo1ToFilter", "lfo1ToPitch", "lfo2Amount", "lfo2Rate", "lfo2Shape",
        "lfo2Target", "lfo3Amount", "lfo3Rate", "lfo3Shape", "lfo3Target", "lpfCutoff",
        "lpfRes", "masterVolume", "noiseLevel", "playMode", "reverbDamp", "reverbMix",
        "reverbSize", "seqPages", "subLevel", "subOctave", "syncMode", "vPan1",
        "vPan2", "vPan3", "vPan4", "vPan5", "vPan6", "vTune1",
        "vTune2", "vTune3", "vTune4", "vTune5", "vTune6", "voiceVarMode",
        "xmodAmount", "xmodMode"
    };
}

// 7 LED theme palettes. C1 keeps these for the legacy footer drawing;
// C2 replaces the footer colours with kit LED tokens.
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
// Footer segment readout
// ==============================================================================
SegmentReadout::SegmentReadout(float charWidth, int insetX)
    : charW(charWidth), inset(insetX)
{
    setWantsKeyboardFocus(false);
    setMouseClickGrabsKeyboardFocus(false);
    setInterceptsMouseClicks(false, false);
}

void SegmentReadout::setText(const juce::String& newText)
{
    if (text == newText) return;
    text = newText;
    repaint();
}

void SegmentReadout::setPalette(const LEDTheme& newPalette)
{
    palette = newPalette;
    repaint();
}

void SegmentReadout::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff090b0e));
    g.fillRoundedRectangle(bounds, 3.0f);
    g.setColour(juce::Colour(0xff4a5260));
    g.drawRoundedRectangle(bounds, 3.0f, 1.2f);
    drawString(g, static_cast<float>(inset), 3.0f, charW, 18.0f, text);
}

void SegmentReadout::drawString(juce::Graphics& g, float x, float y, float w, float h, const juce::String& s)
{
    float startX = x;
    for (int i = 0; i < s.length(); ++i)
    {
        drawChar(g, startX, y, w, h, static_cast<char>(s[i]));
        startX += (w + 2.0f);
    }
}

void SegmentReadout::drawChar(juce::Graphics& g, float x, float y, float w, float h, char ch)
{
    bool s[7] = { false };
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));

    switch (ch)
    {
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

    const float t = 1.8f;
    const float midY = y + h * 0.5f;

    auto drawSeg = [&](int idx, juce::Rectangle<float> rect) {
        if (s[idx]) {
            g.setColour(palette.glow);
            g.fillRoundedRectangle(rect.expanded(1.2f), 1.0f);
            g.setColour(palette.primary);
            g.fillRoundedRectangle(rect, 1.0f);
            g.setColour(palette.highlight);
            g.fillRoundedRectangle(rect.reduced(0.4f), 0.5f);
        } else {
            g.setColour(palette.unlitBg);
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

// ==============================================================================
// Voice panel with live LEDs
// ==============================================================================
void VoicePanel::paintDynamic(juce::Graphics& g)
{
    const auto& t = S116::skin().theme();

    for (int i = 0; i < 6; ++i)
    {
        const auto r = ledRects[(size_t) i];
        if (r.isEmpty()) continue;

        const float cx = (float) r.getCentreX();
        const float cy = (float) r.getCentreY();
        const float radius = 3.0f;
        const bool on = isVoiceActive ? isVoiceActive(i) : false;

        if (on)
        {
            g.setColour (t.ledOn.withAlpha (0.38f));
            g.fillEllipse (cx - radius - 2.5f, cy - radius - 2.5f,
                           (radius + 2.5f) * 2.0f, (radius + 2.5f) * 2.0f);
            g.setColour (t.ledOn);
            g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
            g.setColour (t.ledOn.brighter (0.6f));
            g.fillEllipse (cx - radius * 0.45f, cy - radius * 0.45f,
                           radius * 0.9f, radius * 0.9f);
        }
        else
        {
            g.setColour (t.ledOff);
            g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.65f));
            g.drawEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 0.8f);
        }
    }
}

// ==============================================================================
// SavePatchDialog Implementation
// ==============================================================================
SavePatchDialog::SavePatchDialog(std::function<void(const juce::String&)> onSave, std::function<void()> onCancel)
    : saveCallback(onSave), cancelCallback(onCancel)
{
    setAlwaysOnTop(true);
    setWantsKeyboardFocus(false);
    setMouseClickGrabsKeyboardFocus(false);

    patchNameEditor.setName("PATCH NAME");
    patchNameEditor.setWantsKeyboardFocus(true);
    patchNameEditor.setMouseClickGrabsKeyboardFocus(true);
    patchNameEditor.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    patchNameEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xfff0f2f5));
    patchNameEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xff14171a));
    patchNameEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff68707d));
    patchNameEditor.onReturnKey = [this]() { if (saveCallback) saveCallback(patchNameEditor.getText()); };
    patchNameEditor.onEscapeKey = [this]() { if (cancelCallback) cancelCallback(); };
    addAndMakeVisible(patchNameEditor);

    confirmSaveBtn.setWantsKeyboardFocus(false);
    confirmSaveBtn.setMouseClickGrabsKeyboardFocus(false);
    confirmSaveBtn.onClick = [this]() { if (saveCallback) saveCallback(patchNameEditor.getText()); };
    addAndMakeVisible(confirmSaveBtn);

    cancelSaveBtn.setWantsKeyboardFocus(false);
    cancelSaveBtn.setMouseClickGrabsKeyboardFocus(false);
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
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      buttonKeyboard(p.keyboardState),
      saveDialog([this](const juce::String& name) { performSave(name); },
                 [this]() { hideSaveDialog(); })
{
    ++activeEditorInstances;

    setName("EDITOR");
    setLookAndFeel(&silverLookAndFeel);
    setOpaque(true);

#if S116_HAS_FONTS
    S116::skin().setTypefaces(
        juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowCondensedBold_ttf,
                                                BinaryData::BarlowCondensedBold_ttfSize),
        juce::Typeface::createSystemTypefaceFor(BinaryData::ShareTechMonoRegular_ttf,
                                                BinaryData::ShareTechMonoRegular_ttfSize));
#endif

    currentThemeIdx = juce::jlimit(0, 6, static_cast<int>(getSafeParamValue("ledTheme", 0.0f)));
    currentArpIdx = juce::jlimit(0, 4, static_cast<int>(getSafeParamValue("arpMode", 0.0f)));
    currentChordIdx = juce::jlimit(0, 8, static_cast<int>(getSafeParamValue("chordType", 0.0f)));
    currentPagesIdx = juce::jlimit(0, 3, static_cast<int>(getSafeParamValue("seqPages", 3.0f)));
    currentChassisThemeIdx = juce::jlimit(0, NUM_CHASSIS_THEMES - 1,
                                          static_cast<int>(getSafeParamValue("guiTheme", 0.0f)));

    S116::skin().setLedColour(ledThemes[currentThemeIdx].primary);
    S116::skin().setTheme(mapChassisTheme(currentChassisThemeIdx));
    silverLookAndFeel.applyTheme();

    // Backdrops / pages
    chassisBg.setWantsKeyboardFocus(false);
    chassisBg.setMouseClickGrabsKeyboardFocus(false);
    chassisBg.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(chassisBg);

    headerBar.setName("HEADER BAR");
    headerBar.setWantsKeyboardFocus(false);
    headerBar.setMouseClickGrabsKeyboardFocus(false);
    headerBar.setLogo("S", "-", "106");
    addAndMakeVisible(headerBar);

    auto prepLabel = [](juce::Label& l, const juce::String& text, bool field) {
        l.setText(text, juce::dontSendNotification);
        if (field) S116::layout::styleField(l);
        else       S116::layout::styleCaption(l);
        l.setWantsKeyboardFocus(false);
        l.setMouseClickGrabsKeyboardFocus(false);
    };

    // Header
    prepLabel(presetLabel, "PATCH", true);
    headerBar.addAndMakeVisible(presetLabel);

    refreshPresetList();
    presetBox.setName("PRESET");
    presetBox.setWantsKeyboardFocus(false);
    presetBox.setMouseClickGrabsKeyboardFocus(false);
    presetBox.onChange = [this]() {
        int id = presetBox.getSelectedId();
        if (id >= 1 && id <= 8) {
            audioProcessor.loadFactoryPreset(id - 1);
        } else if (id > 8) {
            auto presetFile = audioProcessor.getPresetsDirectory().getChildFile(presetBox.getText() + ".s106");
            if (presetFile.existsAsFile())
                audioProcessor.loadPatchXml(presetFile.loadFileAsString());
        }
        updateUIFromParameters();
    };
    headerBar.addAndMakeVisible(presetBox);

    savePresetBtn.setWantsKeyboardFocus(false);
    savePresetBtn.setMouseClickGrabsKeyboardFocus(false);
    savePresetBtn.getProperties().set("s116Compact", true);
    savePresetBtn.onClick = [this]() { showSaveDialog(); };
    headerBar.addAndMakeVisible(savePresetBtn);

    initPresetBtn.setWantsKeyboardFocus(false);
    initPresetBtn.setMouseClickGrabsKeyboardFocus(false);
    initPresetBtn.getProperties().set("s116Compact", true);
    initPresetBtn.onClick = [this]() {
        audioProcessor.loadFactoryPreset(0);
        presetBox.setSelectedId(1, juce::dontSendNotification);
        updateUIFromParameters();
    };
    headerBar.addAndMakeVisible(initPresetBtn);

    prepLabel(chassisThemeLabel, "CHASSIS", true);
    headerBar.addAndMakeVisible(chassisThemeLabel);
    chassisThemeBox.addItemList({ "Classic Silver", "JP-8000 Blue", "SH-101 Red", "Graphite" }, 1);
    chassisThemeBox.setName("CHASSIS");
    chassisThemeBox.setWantsKeyboardFocus(false);
    chassisThemeBox.setMouseClickGrabsKeyboardFocus(false);
    chassisThemeBox.setSelectedId(currentChassisThemeIdx + 1, juce::dontSendNotification);
    chassisThemeBox.onChange = [this]() {
        currentChassisThemeIdx = juce::jlimit(0, NUM_CHASSIS_THEMES - 1,
                                              chassisThemeBox.getSelectedItemIndex());
        if (auto* param = audioProcessor.apvts.getParameter("guiTheme"))
            param->setValueNotifyingHost(static_cast<float>(currentChassisThemeIdx)
                                         / static_cast<float>(NUM_CHASSIS_THEMES - 1));
        applyChassisTheme();
    };
    headerBar.addAndMakeVisible(chassisThemeBox);

    prepLabel(ledThemeLabel, "LED COLOUR", true);
    headerBar.addAndMakeVisible(ledThemeLabel);
    themeBox.addItemList({ "Red", "Cyan", "Green", "Amber", "Yellow", "Purple", "White" }, 1);
    themeBox.setName("LED COLOUR");
    themeBox.setWantsKeyboardFocus(false);
    themeBox.setMouseClickGrabsKeyboardFocus(false);
    themeBox.setSelectedId(currentThemeIdx + 1, juce::dontSendNotification);
    themeBox.onChange = [this]() {
        currentThemeIdx = juce::jlimit(0, 6, themeBox.getSelectedItemIndex());
        if (auto* param = audioProcessor.apvts.getParameter("ledTheme"))
            param->setValueNotifyingHost(static_cast<float>(currentThemeIdx) / 6.0f);
        applyLedTheme();
    };
    headerBar.addAndMakeVisible(themeBox);

    headerLcd.setName("HEADER LCD");
    headerLcd.setWantsKeyboardFocus(false);
    headerLcd.setMouseClickGrabsKeyboardFocus(false);
    headerLcd.setInterceptsMouseClicks(false, false);
    setIdleHeaderLcd();
    headerBar.addAndMakeVisible(headerLcd);

    pageTabBtn.setWantsKeyboardFocus(false);
    pageTabBtn.setMouseClickGrabsKeyboardFocus(false);
    pageTabBtn.getProperties().set("s116Compact", true);
    pageTabBtn.onClick = [this]() {
        currentTab = currentTab == 0 ? 1 : 0;
        synthPage.setVisible(currentTab == 0);
        fxPage.setVisible(currentTab == 1);
        resized();
        repaint();
    };
    headerBar.addAndMakeVisible(pageTabBtn);

    // Pages and panels
    synthPage.setName("SYNTH PAGE");
    fxPage.setName("FX PAGE");
    for (auto* page : { &synthPage, &fxPage }) {
        page->setWantsKeyboardFocus(false);
        page->setMouseClickGrabsKeyboardFocus(false);
        addAndMakeVisible(*page);
    }

    auto prepPanel = [](S116::SectionPanel& panel, const juce::String& name) {
        panel.setName(name);
        panel.setCompactPadding(true);
        panel.setWantsKeyboardFocus(false);
        panel.setMouseClickGrabsKeyboardFocus(false);
    };

    prepPanel(dcoPanel, "DCO 1 & 2");
    prepPanel(filterPanel, "HPF & VCF");
    prepPanel(envPanel, "ENVELOPES");
    prepPanel(subPanel, "SUB, NOISE & X-MOD");
    prepPanel(lfo1Panel, "LFO 1");
    prepPanel(lfo2Panel, "LFO 2");
    prepPanel(lfo3Panel, "LFO 3");
    prepPanel(voicePanel, "VOICE VARIATION MATRIX");

    synthPage.addAndMakeVisible(dcoPanel);
    synthPage.addAndMakeVisible(filterPanel);
    synthPage.addAndMakeVisible(envPanel);
    synthPage.addAndMakeVisible(subPanel);
    synthPage.addAndMakeVisible(lfo1Panel);
    synthPage.addAndMakeVisible(lfo2Panel);
    synthPage.addAndMakeVisible(lfo3Panel);
    synthPage.addAndMakeVisible(voicePanel);

    prepPanel(chorusPanel, "STEREO CHORUS");
    prepPanel(delayPanel, "DELAY");
    prepPanel(reverbPanel, "REVERB");
    fxPage.addAndMakeVisible(chorusPanel);
    fxPage.addAndMakeVisible(delayPanel);
    fxPage.addAndMakeVisible(reverbPanel);

    prepPanel(footerPanel, "SEQUENCER & KEYBOARD");
    addAndMakeVisible(footerPanel);

    panelTitles = {
        { &dcoPanel, "DCO 1 & DCO 2" },
        { &filterPanel, "HPF & 24dB VCF" },
        { &envPanel, "ENVELOPES" },
        { &subPanel, "SUB, NOISE & X-MOD" },
        { &lfo1Panel, "LFO 1" },
        { &lfo2Panel, "LFO 2" },
        { &lfo3Panel, "LFO 3" },
        { &voicePanel, "VOICE VARIATION MATRIX" },
        { &chorusPanel, "STEREO CHORUS" },
        { &delayPanel, "TEMPO-SYNC PING-PONG DELAY" },
        { &reverbPanel, "LUSH DIGITAL REVERB" },
        { &footerPanel, "SEQUENCER & KEYBOARD" }
    };

    // --- DCO 1 ---
    setupControl(dco1Morph,  "dco1Morph",  "MORPH",  dcoPanel);
    setupControl(dco1PWM,    "dco1PWM",    "PWM",    dcoPanel);
    setupControl(dco1Octave, "dco1Octave", "OCTAVE", dcoPanel);
    setupControl(dco1Fold,   "dco1Fold",   "FOLD",   dcoPanel);
    setupControl(dco1Level,  "dco1Level",  "LEVEL",  dcoPanel);

    // --- DCO 2 ---
    setupControl(dco2Morph,  "dco2Morph",  "MORPH",  dcoPanel);
    setupControl(dco2PWM,    "dco2PWM",    "PWM",    dcoPanel);
    setupControl(dco2Semi,   "dco2Semi",   "SEMI",   dcoPanel);
    setupControl(dco2Cents,  "dco2Cents",  "DETUNE", dcoPanel);
    setupControl(dco2Level,  "dco2Level",  "LEVEL",  dcoPanel);

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

    dco1Octave.slider.textFromValueFunction = [](double v) {
        int idx = juce::jlimit(0, 3, static_cast<int>(std::round(v)));
        const char* octs[] = { "32'", "16'", "8'", "4'" };
        return juce::String(octs[idx]);
    };

    dco1Fold.slider.textFromValueFunction = [](double v) { return juce::String((int)(v * 100.0)) + "%"; };
    dco2Semi.slider.textFromValueFunction = [](double v) { return juce::String((int)v) + " st"; };
    dco2Cents.slider.textFromValueFunction = [](double v) { return juce::String((int)v) + " ct"; };

    // --- Sub / Noise / Glide / X-Mod ---
    setupControl(subLevel,   "subLevel",   "SUB OSC", subPanel);
    setupControl(noiseLevel, "noiseLevel", "NOISE",   subPanel);
    setupControl(glideTime,  "glideTime",  "GLIDE",   subPanel);
    setupControl(xmodAmount, "xmodAmount", "X-MOD",   subPanel);
    xmodAmount.slider.textFromValueFunction = [](double v) { return juce::String((int)(v * 100.0)) + "%"; };

    setupBox(subOctBox, subOctLabel, "subOctave", "SUB OCT", { "-1", "-2" }, subPanel);
    setupBox(xmodModeBox, xmodModeLabel, "xmodMode", "X-TYPE", { "FM", "RING" }, subPanel);

    syncBtn.setClickingTogglesState(true);
    syncBtn.setWantsKeyboardFocus(false);
    syncBtn.setMouseClickGrabsKeyboardFocus(false);
    syncBtn.getProperties().set("s116Compact", true);
    subPanel.addAndMakeVisible(syncBtn);

    auto attachButton = [this](juce::TextButton& button, const juce::String& paramId) {
        if (audioProcessor.apvts.getParameter(paramId) != nullptr) {
            btnAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                audioProcessor.apvts, paramId, button));
            buttonAttachedIds.push_back(paramId);
        }
    };
    attachButton(syncBtn, "syncMode");

    // --- Filter ---
    hpfCutoff.slider.textFromValueFunction = [](double v) { return juce::String((int)v) + " Hz"; };
    lpfCutoff.slider.textFromValueFunction = [](double v) {
        return (v >= 1000.0) ? juce::String(v / 1000.0, 1) + "k" : juce::String((int)v);
    };
    envMod.slider.textFromValueFunction = [](double v) { return juce::String((int)(v * 100.0)) + "%"; };

    setupControl(hpfCutoff, "hpfCutoff", "HPF",    filterPanel);
    setupControl(lpfCutoff, "lpfCutoff", "CUTOFF", filterPanel);
    setupControl(lpfRes,    "lpfRes",    "RESO",   filterPanel);
    setupControl(envMod,    "envMod",    "ENV MOD", filterPanel);

    setupControl(filtA, "filtAttack",  "ATTACK",  filterPanel);
    setupControl(filtD, "filtDecay",   "DECAY",   filterPanel);
    setupControl(filtS, "filtSustain", "SUSTAIN", filterPanel);
    setupControl(filtR, "filtRelease", "RELEASE", filterPanel);

    // --- Envelopes & Master ---
    setupControl(ampA, "ampAttack",  "ATTACK",  envPanel);
    setupControl(ampD, "ampDecay",   "DECAY",   envPanel);
    setupControl(ampS, "ampSustain", "SUSTAIN", envPanel);
    setupControl(ampR, "ampRelease", "RELEASE", envPanel);

    setupBox(playModeBox, playModeLabel, "playMode", "PLAY MODE", { "Poly", "Mono", "Unison" }, envPanel);
    setupControl(masterVol, "masterVolume", "VOLUME", envPanel);

    // --- LFO 1 ---
    setupControl(lfo1Rate,    "lfo1Rate",     "RATE",    lfo1Panel);
    setupControl(lfo1ToFilt,  "lfo1ToFilter", "VCF MOD", lfo1Panel);
    setupControl(lfo1ToPitch, "lfo1ToPitch",  "PITCH",   lfo1Panel);
    setupBox(lfo1ShapeBox, lfo1ShapeLabel, "lfo1Shape", "SHAPE",
             { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H" }, lfo1Panel);

    // --- LFO 2 / 3 ---
    lfo1Rate.slider.textFromValueFunction = [](double v) { return juce::String(v, 1) + " Hz"; };
    lfo2Rate.slider.textFromValueFunction = [](double v) { return juce::String(v, 1) + " Hz"; };
    lfo3Rate.slider.textFromValueFunction = [](double v) { return juce::String(v, 1) + " Hz"; };

    setupControl(lfo2Rate,   "lfo2Rate",   "RATE",  lfo2Panel);
    setupControl(lfo2Amount, "lfo2Amount", "DEPTH", lfo2Panel);
    setupBox(lfo2ShapeBox, lfo2ShapeLabel, "lfo2Shape", "SHAPE", { "Sine", "Triangle", "S&H" }, lfo2Panel);

    setupControl(lfo3Rate,   "lfo3Rate",   "RATE",  lfo3Panel);
    setupControl(lfo3Amount, "lfo3Amount", "DEPTH", lfo3Panel);
    setupBox(lfo3ShapeBox, lfo3ShapeLabel, "lfo3Shape", "SHAPE", { "Sine", "Triangle", "S&H" }, lfo3Panel);

    const juce::StringArray fullTargets = {
        "None", "LPF Cutoff", "LPF Reso", "HPF Cutoff", "Env Mod",
        "Filt Attack", "Filt Decay", "Filt Sustain", "Filt Release",
        "Amp Attack", "Amp Decay", "Amp Sustain", "Amp Release",
        "DCO1 Morph", "DCO1 PWM", "DCO2 Morph", "DCO2 PWM", "DCO2 Semi", "DCO2 Detune",
        "Sub Level", "Noise Level", "Glide Time", "Master Volume",
        "DCO1 Fold", "X-Mod Amount"
    };
    setupBox(lfo2TargetBox, lfo2TargetLabel, "lfo2Target", "TARGET", fullTargets, lfo2Panel);
    setupBox(lfo3TargetBox, lfo3TargetLabel, "lfo3Target", "TARGET", fullTargets, lfo3Panel);

    // --- Voice Variation Matrix ---
    for (int i = 0; i < 6; ++i)
    {
        auto& vk = voiceKnobs[i];
        vk.label.setWantsKeyboardFocus(false);
        vk.label.setMouseClickGrabsKeyboardFocus(false);
        vk.label.setInterceptsMouseClicks(false, false);
        voicePanel.addAndMakeVisible(vk.label);

        vk.slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        vk.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 54, 20);
        vk.slider.setWantsKeyboardFocus(false);
        vk.slider.setMouseClickGrabsKeyboardFocus(false);
        voicePanel.addAndMakeVisible(vk.slider);
    }

    setupBox(voiceVarModeBox, voiceVarLabel, "voiceVarMode", "VOICE VAR", { "PANNING", "TUNING" }, voicePanel);
    voiceVarModeBox.onChange = [this]() { updateVoiceKnobAttachments(); };
    updateVoiceKnobAttachments();

    cycleBtn.setClickingTogglesState(true);
    cycleBtn.setWantsKeyboardFocus(false);
    cycleBtn.setMouseClickGrabsKeyboardFocus(false);
    cycleBtn.getProperties().set("s116Compact", true);
    voicePanel.addAndMakeVisible(cycleBtn);
    attachButton(cycleBtn, "cycleMode");

    voicePanel.isVoiceActive = [this](int i) { return audioProcessor.isVoiceActive(i); };

    // --- Master FX ---
    setupBox(chorusModeBox, chorusLabel, "chorusMode", "STEREO CHORUS",
             { "Chorus OFF", "Chorus I", "Chorus II", "Chorus I+II" }, chorusPanel);

    const juce::StringArray syncTimings = {
        "1/32", "1/32t", "1/32d", "1/16", "1/16t", "1/16d",
        "1/8", "1/8t", "1/8d", "1/4", "1/4t", "1/4d",
        "1/2", "1/2t", "1/2d", "1/1", "FREE"
    };
    setupBox(delaySyncBox, delaySyncLabel, "delaySync", "DELAY TIME / SYNC", syncTimings, delayPanel);

    setupControl(delayTime, "delayTime",     "TIME (FREE)", delayPanel);
    setupControl(delayFb,   "delayFeedback", "FEEDBACK",    delayPanel);
    setupControl(delayDamp, "delayDamp",     "DAMPING",     delayPanel);
    setupControl(delayMix,  "delayMix",      "DELAY MIX",   delayPanel);

    delayPingPongBtn.setClickingTogglesState(true);
    delayPingPongBtn.setWantsKeyboardFocus(false);
    delayPingPongBtn.setMouseClickGrabsKeyboardFocus(false);
    delayPingPongBtn.getProperties().set("s116Compact", true);
    delayPanel.addAndMakeVisible(delayPingPongBtn);
    attachButton(delayPingPongBtn, "delayPingPong");

    setupControl(reverbSize, "reverbSize", "ROOM SIZE", reverbPanel);
    setupControl(reverbDamp, "reverbDamp", "DAMPING",   reverbPanel);
    setupControl(reverbMix,  "reverbMix",  "REVERB MIX", reverbPanel);

    // --- Footer ---
    auto prepFooterButton = [](juce::TextButton& b, bool compact) {
        b.setWantsKeyboardFocus(false);
        b.setMouseClickGrabsKeyboardFocus(false);
        if (compact) b.getProperties().set("s116Compact", true);
    };

    prepFooterButton(seqPlayBtn, true);
    prepFooterButton(seqRecBtn, true);
    prepFooterButton(seqRestBtn, true);
    prepFooterButton(seqClearBtn, true);
    prepFooterButton(arpToggleBtn, true);
    prepFooterButton(chordToggleBtn, true);

    seqPlayBtn.setClickingTogglesState(true);
    seqPlayBtn.onClick = [this]() {
        audioProcessor.sequencer.isPlaying = seqPlayBtn.getToggleState();
    };
    footerPanel.addAndMakeVisible(seqPlayBtn);

    seqRecBtn.setClickingTogglesState(true);
    seqRecBtn.onClick = [this]() {
        audioProcessor.sequencer.isRecording = seqRecBtn.getToggleState();
    };
    footerPanel.addAndMakeVisible(seqRecBtn);

    seqRestBtn.onClick = [this]() {
        if (audioProcessor.sequencer.isRecording)
            audioProcessor.sequencer.recordRest();
    };
    footerPanel.addAndMakeVisible(seqRestBtn);

    seqClearBtn.onClick = [this]() {
        audioProcessor.sequencer.clearPattern();
    };
    footerPanel.addAndMakeVisible(seqClearBtn);

    for (auto* b : { &arpPrevBtn, &arpNextBtn, &chordPrevBtn, &chordNextBtn,
                     &stepsPrevBtn, &stepsNextBtn, &pagePrevBtn, &pageNextBtn })
    {
        b->setWantsKeyboardFocus(false);
        b->setMouseClickGrabsKeyboardFocus(false);
        b->getProperties().set("s116Arrow", true);
        b->setName(b->getButtonText() == "<" ? "ARROW LEFT" : "ARROW RIGHT");
        footerPanel.addAndMakeVisible(*b);
    }

    arpToggleBtn.setClickingTogglesState(true);
    footerPanel.addAndMakeVisible(arpToggleBtn);
    attachButton(arpToggleBtn, "arpEnable");

    arpPrevBtn.onClick = [this]() {
        currentArpIdx = (currentArpIdx + 4) % 5;
        if (auto* param = audioProcessor.apvts.getParameter("arpMode"))
            param->setValueNotifyingHost(static_cast<float>(currentArpIdx) / 4.0f);
        updateSegmentDisplays();
    };

    arpNextBtn.onClick = [this]() {
        currentArpIdx = (currentArpIdx + 1) % 5;
        if (auto* param = audioProcessor.apvts.getParameter("arpMode"))
            param->setValueNotifyingHost(static_cast<float>(currentArpIdx) / 4.0f);
        updateSegmentDisplays();
    };

    chordToggleBtn.setClickingTogglesState(true);
    footerPanel.addAndMakeVisible(chordToggleBtn);
    attachButton(chordToggleBtn, "chordEnable");

    chordPrevBtn.onClick = [this]() {
        currentChordIdx = (currentChordIdx + 8) % 9;
        if (auto* param = audioProcessor.apvts.getParameter("chordType"))
            param->setValueNotifyingHost(static_cast<float>(currentChordIdx) / 8.0f);
        updateSegmentDisplays();
    };

    chordNextBtn.onClick = [this]() {
        currentChordIdx = (currentChordIdx + 1) % 9;
        if (auto* param = audioProcessor.apvts.getParameter("chordType"))
            param->setValueNotifyingHost(static_cast<float>(currentChordIdx) / 8.0f);
        updateSegmentDisplays();
    };

    audioProcessor.sequencer.setNumPages(currentPagesIdx + 1);

    stepsPrevBtn.onClick = [this]() {
        currentPagesIdx = (currentPagesIdx + 3) % 4;
        audioProcessor.sequencer.setNumPages(currentPagesIdx + 1);
        if (auto* param = audioProcessor.apvts.getParameter("seqPages"))
            param->setValueNotifyingHost(static_cast<float>(currentPagesIdx) / 3.0f);
        if (currentSeqPage > currentPagesIdx) currentSeqPage = currentPagesIdx;
        updateSegmentDisplays();
    };

    stepsNextBtn.onClick = [this]() {
        currentPagesIdx = (currentPagesIdx + 1) % 4;
        audioProcessor.sequencer.setNumPages(currentPagesIdx + 1);
        if (auto* param = audioProcessor.apvts.getParameter("seqPages"))
            param->setValueNotifyingHost(static_cast<float>(currentPagesIdx) / 3.0f);
        updateSegmentDisplays();
    };

    pagePrevBtn.onClick = [this]() {
        int maxPages = audioProcessor.sequencer.getNumPages();
        if (maxPages > 0) {
            currentSeqPage = (currentSeqPage + maxPages - 1) % maxPages;
            audioProcessor.sequencer.setRecordStep(currentSeqPage * 16);
            updateSegmentDisplays();
        }
    };

    pageNextBtn.onClick = [this]() {
        int maxPages = audioProcessor.sequencer.getNumPages();
        if (maxPages > 0) {
            currentSeqPage = (currentSeqPage + 1) % maxPages;
            audioProcessor.sequencer.setRecordStep(currentSeqPage * 16);
            updateSegmentDisplays();
        }
    };

    prepLabel(stepsLabel, "STEPS", true);
    stepsLabel.setJustificationType(juce::Justification::centredRight);
    footerPanel.addAndMakeVisible(stepsLabel);

    prepLabel(pageLabel, "PAGE", true);
    pageLabel.setJustificationType(juce::Justification::centredRight);
    footerPanel.addAndMakeVisible(pageLabel);

    arpDisplay.setName("ARP DISPLAY");
    chordDisplay.setName("CHORD DISPLAY");
    stepsDisplay.setName("STEPS DISPLAY");
    pageDisplay.setName("PAGE DISPLAY");
    footerPanel.addAndMakeVisible(arpDisplay);
    footerPanel.addAndMakeVisible(chordDisplay);
    footerPanel.addAndMakeVisible(stepsDisplay);
    footerPanel.addAndMakeVisible(pageDisplay);

    buttonKeyboard.setWantsKeyboardFocus(false);
    buttonKeyboard.setMouseClickGrabsKeyboardFocus(false);
    footerPanel.addAndMakeVisible(buttonKeyboard);

    addChildComponent(saveDialog);

    setWantsKeyboardFocus(true);
    addKeyListener(this);
    addMouseListener(this, true);

    // Every layout component (including JUCE's internal combo/slider children)
    // must not take keyboard focus. The save dialog's TextEditor is the only
    // exception, so exclude that subtree.
    std::function<void(juce::Component&)> clearFocus = [&](juce::Component& c) {
        c.setWantsKeyboardFocus(false);
        c.setMouseClickGrabsKeyboardFocus(false);
        for (auto* child : c.getChildren())
            clearFocus(*child);
    };
    for (auto* child : getChildren())
        if (child != &saveDialog)
            clearFocus(*child);

    S116::skin().addChangeListener(this);
    refreshKeyboardLedTheme();
    updateSegmentDisplays();
    setIdleHeaderLcd();

    setResizable(true, true);
    setResizeLimits(kMinW, kMinH, kMaxW, kMaxH);
    setSize(kMinW, kMinH);
    startTimerHz(60);

    if (std::getenv("S106_DEBUG_AUDIT") != nullptr)
    {
        juce::Component::SafePointer<Simple106AudioProcessorEditor> safe (this);
        juce::Timer::callAfterDelay(250, [safe]() mutable {
            if (safe != nullptr)
                safe->runDebugAudit();
        });
    }
}

Simple106AudioProcessorEditor::~Simple106AudioProcessorEditor() {
    stopTimer();
    S116::skin().removeChangeListener(this);
    removeMouseListener(this);
    removeKeyListener(this);
    setLookAndFeel(nullptr);

    if (--activeEditorInstances == 0)
        S116::skin().clearResources();
}

float Simple106AudioProcessorEditor::getSafeParamValue(const juce::String& paramId, float fallback) const {
    if (auto* p = audioProcessor.apvts.getRawParameterValue(paramId))
        return p->load();
    return fallback;
}

S116::ThemeId Simple106AudioProcessorEditor::mapChassisTheme(int index) const {
    switch (juce::jlimit(0, NUM_CHASSIS_THEMES - 1, index)) {
        case 0:  return S116::ThemeId::ClassicSilver;
        case 1:  return S116::ThemeId::JP8000Blue;
        case 2:  return S116::ThemeId::SH101Red;
        default: return S116::ThemeId::GraphiteRed;
    }
}

void Simple106AudioProcessorEditor::applyThisEditorsSkin()
{
    const auto wantedTheme = mapChassisTheme(currentChassisThemeIdx);
    const auto wantedLed = getActiveTheme().primary;

    const bool themeChanged = S116::skin().themeId() != wantedTheme;
    const bool ledChanged = S116::skin().getLedBaseColour() != wantedLed;

    if (themeChanged) S116::skin().setTheme(wantedTheme);
    if (ledChanged) S116::skin().setLedColour(wantedLed);

    if (themeChanged || ledChanged)
    {
        silverLookAndFeel.applyTheme();
        sendLookAndFeelChange();
        refreshKeyboardLedTheme();
        repaint();
    }
}

bool Simple106AudioProcessorEditor::syncThemeParamsFromParameters()
{
    bool changed = false;

    const int gui = juce::jlimit(0, NUM_CHASSIS_THEMES - 1,
                                 static_cast<int>(getSafeParamValue("guiTheme", 0.0f)));
    if (gui != currentChassisThemeIdx) {
        currentChassisThemeIdx = gui;
        chassisThemeBox.setSelectedId(gui + 1, juce::dontSendNotification);
        changed = true;
    }

    const int led = juce::jlimit(0, 6, static_cast<int>(getSafeParamValue("ledTheme", 0.0f)));
    if (led != currentThemeIdx) {
        currentThemeIdx = led;
        themeBox.setSelectedId(led + 1, juce::dontSendNotification);
        changed = true;
    }

    return changed;
}

void Simple106AudioProcessorEditor::applyChassisTheme() {
    syncThemeParamsFromParameters();
    applyThisEditorsSkin();
}

void Simple106AudioProcessorEditor::applyLedTheme() {
    syncThemeParamsFromParameters();
    applyThisEditorsSkin();
}

void Simple106AudioProcessorEditor::refreshKeyboardLedTheme()
{
    auto base = S116::skin().getLedBaseColour();
    int paletteIdx = juce::jlimit(0, 6, currentThemeIdx);

    for (int i = 0; i < 7; ++i) {
        if (ledThemes[i].primary == base) {
            paletteIdx = i;
            break;
        }
    }

    const auto& t = ledThemes[(size_t) paletteIdx];
    buttonKeyboard.setLEDTheme(t.primary, t.glow, t.highlight, t.unlit);
    arpDisplay.setPalette(t);
    chordDisplay.setPalette(t);
    stepsDisplay.setPalette(t);
    pageDisplay.setPalette(t);
}

void Simple106AudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster*)
{
    refreshKeyboardLedTheme();
}

void Simple106AudioProcessorEditor::updateUIFromParameters() {
    currentChordIdx = juce::jlimit(0, 8, static_cast<int>(getSafeParamValue("chordType", 0.0f)));
    currentArpIdx   = juce::jlimit(0, 4, static_cast<int>(getSafeParamValue("arpMode", 0.0f)));
    currentPagesIdx = juce::jlimit(0, 3, static_cast<int>(getSafeParamValue("seqPages", 3.0f)));
    currentChassisThemeIdx = juce::jlimit(0, NUM_CHASSIS_THEMES - 1,
                                          static_cast<int>(getSafeParamValue("guiTheme", 0.0f)));
    audioProcessor.sequencer.setNumPages(currentPagesIdx + 1);

    const bool themeChanged = syncThemeParamsFromParameters();
    if (themeChanged && (hasKeyboardFocus(true) || activeEditorInstances.load() == 1))
        applyThisEditorsSkin();

    const int varMode = juce::jlimit(0, 1, static_cast<int>(getSafeParamValue("voiceVarMode", 0.0f)));
    voiceVarModeBox.setSelectedId(varMode + 1, juce::dontSendNotification);

    updateVoiceKnobAttachments();
    updateSegmentDisplays();
    repaint();
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

    const char* factoryNames[] = {
        "01. Init Poly", "02. Juno Warm Pad", "03. Poly Drift Keys", "04. Supersaw Hoover",
        "05. Acid 106 Lead", "06. 80s Arp Dream", "07. MonoPoly Drift", "08. Space Pluck"
    };
    for (int i = 0; i < 8; ++i)
        presetBox.addItem(factoryNames[i], i + 1);

    presetBox.addSeparator();

    auto dir = audioProcessor.getPresetsDirectory();
    auto userFiles = dir.findChildFiles(juce::File::findFiles, false, "*.s106");
    int userIdx = 9;
    for (const auto& file : userFiles)
        presetBox.addItem(file.getFileNameWithoutExtension(), userIdx++);

    presetBox.setSelectedId(1, juce::dontSendNotification);
}

void Simple106AudioProcessorEditor::updateSegmentDisplays()
{
    const char* arpNames[] = { "UP", "DOWN", "U-D", "RAND", "PLAY" };
    arpDisplay.setText(arpNames[juce::jlimit(0, 4, currentArpIdx)]);

    const char* chordNames[] = { "MAJ", "MIN", "MAJ7", "MIN7", "DOM7", "SUS4", "DIM", "OCT", "5TH" };
    chordDisplay.setText(chordNames[juce::jlimit(0, 8, currentChordIdx)]);

    const char* stepNames[] = { "16", "32", "48", "64" };
    stepsDisplay.setText(stepNames[juce::jlimit(0, 3, currentPagesIdx)]);

    pageDisplay.setText("PG-" + juce::String(currentSeqPage + 1));
}

void Simple106AudioProcessorEditor::setIdleHeaderLcd()
{
    headerLcd.setLines({ "S-106", "READY" });
}

void Simple106AudioProcessorEditor::showSliderValue(juce::Slider* slider)
{
    if (slider == nullptr) return;
    auto it = sliderNames.find(slider);
    if (it == sliderNames.end()) return;

    headerLcd.setLines({ it->second.toUpperCase(),
                         slider->getTextFromValue(slider->getValue()) });
    headerLcdHoldTicks = 90;
}

juce::Slider* Simple106AudioProcessorEditor::sliderAt(juce::Component* component) const
{
    for (auto* c = component; c != nullptr; c = c->getParentComponent())
        if (auto* s = dynamic_cast<juce::Slider*>(c))
            return s;
    return nullptr;
}

void Simple106AudioProcessorEditor::timerCallback() {
    const bool themeChanged = syncThemeParamsFromParameters();
    if (themeChanged && (hasKeyboardFocus(true) || activeEditorInstances.load() == 1))
        applyThisEditorsSkin();

    if (headerLcdHoldTicks > 0) {
        --headerLcdHoldTicks;
        if (headerLcdHoldTicks == 0)
            setIdleHeaderLcd();
    }

    // Only claim keyboard focus if the native OS window is genuinely focused.
    if (auto* peer = getPeer()) {
        if (peer->isFocused() && !hasKeyboardFocus(true) && !saveDialog.isVisible())
            grabKeyboardFocus();
    }

    buttonKeyboard.setSequencerState(
        audioProcessor.sequencer.isPlaying,
        audioProcessor.sequencer.isRecording,
        audioProcessor.sequencer.getCurrentStep(),
        audioProcessor.sequencer.getRecordStep(),
        currentSeqPage
    );

    std::array<bool, 25> activeKeys { false };
    int currentStart = 48 + buttonKeyboard.getOctaveOffset();

    for (int i = 0; i < 25; ++i) {
        int noteNum = currentStart + i;
        if (audioProcessor.keyboardState.isNoteOn(1, noteNum))
            activeKeys[static_cast<size_t>(i)] = true;
    }

    bool chordOn = getSafeParamValue("chordEnable", 0.0f) > 0.5f;
    int chordIdx = juce::jlimit(0, 8, static_cast<int>(getSafeParamValue("chordType", 0.0f)));
    if (chordOn) {
        for (int i = 0; i < 25; ++i) {
            int noteNum = currentStart + i;
            if (audioProcessor.keyboardState.isNoteOn(1, noteNum)) {
                auto chordNotes = audioProcessor.getChordNotes(noteNum, chordIdx);
                for (int n : chordNotes) {
                    int idx = n - currentStart;
                    if (idx >= 0 && idx < 25)
                        activeKeys[static_cast<size_t>(idx)] = true;
                }
            }
        }
    }

    buttonKeyboard.setActiveNotes(activeKeys);

    if (hasKeyboardFocus(true) && !saveDialog.isVisible()
        && !juce::ModifierKeys::getCurrentModifiers().isAnyMouseButtonDown()) {
        for (size_t i = 0; i < NUM_QWERTY_KEYS; ++i) {
            if (qwertyDownState[i]) {
                int code = qwertyMappings[i].keyCode;
                bool isDown = juce::KeyPress::isKeyCurrentlyDown(code)
                           || juce::KeyPress::isKeyCurrentlyDown(std::tolower(code));
                if (!isDown) {
                    qwertyDownState[i] = false;
                    audioProcessor.keyboardState.noteOff(1, qwertyActiveNote[i], 0.0f);
                }
            }
        }
    }

    if (currentTab == 0)
        voicePanel.repaint();
}

void Simple106AudioProcessorEditor::updateVoiceKnobAttachments() {
    voiceAttachments.clear();
    voiceAttachedIds.clear();

    const bool isTune = (voiceVarModeBox.getSelectedItemIndex() == 1);

    for (int i = 0; i < 6; ++i) {
        juce::String paramId = (isTune ? "vTune" : "vPan") + juce::String(i + 1);
        voiceKnobs[i].label.setText("V" + juce::String(i + 1), juce::dontSendNotification);

        if (audioProcessor.apvts.getParameter(paramId) != nullptr) {
            voiceAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.apvts, paramId, voiceKnobs[i].slider));
            voiceAttachedIds.push_back(paramId);
        }

        // SliderAttachment installs the parameter's own text function, so set
        // the voice formatting afterwards.
        if (isTune) {
            voiceKnobs[i].slider.textFromValueFunction = [](double v) {
                return juce::String((int)v) + " ct";
            };
        } else {
            voiceKnobs[i].slider.textFromValueFunction = [](double v) {
                if (std::abs(v) < 0.05) return juce::String("MID");
                return (v < 0) ? "L" + juce::String((int)(-v * 100))
                               : "R" + juce::String((int)(v * 100));
            };
        }

        sliderNames[&voiceKnobs[i].slider] =
            "VOICE " + juce::String(i + 1) + (isTune ? " TUNE" : " PAN");
        voiceKnobs[i].slider.updateText();
    }
}

void Simple106AudioProcessorEditor::setupControl(LabeledSlider& ctrl, const juce::String& paramId,
                                                 const juce::String& labelText, juce::Component& parent)
{
    ctrl.label.setText(labelText, juce::dontSendNotification);
    S116::layout::styleCaption(ctrl.label);
    ctrl.label.setName(labelText);
    ctrl.label.setWantsKeyboardFocus(false);
    ctrl.label.setMouseClickGrabsKeyboardFocus(false);
    parent.addAndMakeVisible(ctrl.label);

    ctrl.slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    ctrl.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    ctrl.slider.setWantsKeyboardFocus(false);
    ctrl.slider.setMouseClickGrabsKeyboardFocus(false);
    ctrl.slider.setName(labelText);
    parent.addAndMakeVisible(ctrl.slider);

    if (audioProcessor.apvts.getParameter(paramId) != nullptr) {
        sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, paramId, ctrl.slider));
        fixedAttachedIds.push_back(paramId);
    }

    if (auto* p = audioProcessor.apvts.getParameter(paramId))
        sliderNames[&ctrl.slider] = p->getName(64);
    else
        sliderNames[&ctrl.slider] = labelText;
}

void Simple106AudioProcessorEditor::setupBox(juce::ComboBox& box, juce::Label& label,
                                             const juce::String& paramId, const juce::String& text,
                                             const juce::StringArray& items, juce::Component& parent)
{
    S116::layout::styleField(label);
    label.setText(text, juce::dontSendNotification);
    label.setName(text);
    label.setWantsKeyboardFocus(false);
    label.setMouseClickGrabsKeyboardFocus(false);
    parent.addAndMakeVisible(label);

    box.addItemList(items, 1);
    box.setName(text);
    box.setWantsKeyboardFocus(false);
    box.setMouseClickGrabsKeyboardFocus(false);
    parent.addAndMakeVisible(box);

    if (audioProcessor.apvts.getParameter(paramId) != nullptr) {
        boxAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.apvts, paramId, box));
        comboAttachedIds.push_back(paramId);
    }
}

void Simple106AudioProcessorEditor::placeBare(juce::Slider& slider, juce::Label& caption,
                                              juce::Rectangle<int> cell, const juce::String& name)
{
    const int square = S116::layout::placeBareKnob(slider, cell.getX(), cell.getY(),
                                                   cell.getWidth(), kSmallBody);
    caption.setBounds(cell.getX(), cell.getY() + square, cell.getWidth(), 16);
    knobCells.push_back({ &slider, &caption, cell, name });
}

void Simple106AudioProcessorEditor::placeVoice(juce::Slider& slider, juce::Label& caption,
                                               juce::Rectangle<int> cell, const juce::String& name)
{
    S116::layout::placeKnob(slider, caption, cell.getX(), cell.getY(),
                            cell.getWidth(), kSmallBody, false);
    knobCells.push_back({ &slider, &caption, cell, name });
}

void Simple106AudioProcessorEditor::layoutHeader()
{
    const int W = headerBar.getWidth();
    const int extra = juce::jmax(0, W - 1008);
    const int logoRight = (int) std::ceil(headerBar.getLogoRight());
    const int startX = logoRight + 14;

    const int presetW = 180 + extra * 25 / 100;
    const int saveW = 48, initW = 48;
    const int presetX = startX;
    const int saveX = presetX + presetW + 6;
    const int initX = saveX + saveW + 6;

    const int tabW = 90;
    const int lcdW = 160 + extra * 35 / 100;
    const int chassisW = 150, ledW = 110;
    const int tabX = W - 12 - tabW;
    const int lcdX = tabX - 8 - lcdW;
    const int ledX = lcdX - 12 - ledW;
    const int chassisX = ledX - 6 - chassisW;

    presetLabel.setBounds(presetX, 10, presetW, 14);
    presetBox.setBounds(presetX, 30, presetW, 32);
    savePresetBtn.setBounds(saveX, 19, saveW, 34);
    initPresetBtn.setBounds(initX, 19, initW, 34);

    chassisThemeLabel.setBounds(chassisX, 10, chassisW, 14);
    chassisThemeBox.setBounds(chassisX, 30, chassisW, 32);
    ledThemeLabel.setBounds(ledX, 10, ledW, 14);
    themeBox.setBounds(ledX, 30, ledW, 32);

    headerLcd.setBounds(lcdX, 13, lcdW, 46);
    pageTabBtn.setBounds(tabX, 19, tabW, 34);
}

void Simple106AudioProcessorEditor::layoutSynthPage()
{
    const int bodyW = bodyRect.getWidth();
    const int bodyH = bodyRect.getHeight();
    const int extra = juce::jmax(0, bodyH - (kTopRowMin + kRowGap + kBottomRowMin));
    const int topH = kTopRowMin + extra * kTopRowMin / (kTopRowMin + kBottomRowMin);
    const int bottomH = bodyH - kRowGap - topH;

    const int topAvail = bodyW - 20;
    const int topWeights = 360 + 288 + 288;
    const int wDco = topAvail * 360 / topWeights;
    const int wFil = topAvail * 288 / topWeights;
    const int wEnv = topAvail - wDco - wFil;

    const int bottomAvail = bodyW - 40;
    const int bottomWeights = 196 + 216 + 144 + 144 + 240;
    const int wSub = bottomAvail * 196 / bottomWeights;
    const int wL1 = bottomAvail * 216 / bottomWeights;
    const int wL2 = bottomAvail * 144 / bottomWeights;
    const int wL3 = bottomAvail * 144 / bottomWeights;
    const int wVoice = bottomAvail - wSub - wL1 - wL2 - wL3;

    const int topY = 0;
    const int bottomY = topH + kRowGap;

    auto setPanel = [&](S116::SectionPanel& panel, juce::Rectangle<int> editorRect, int idx) {
        panel.setBounds(editorRect.translated(-bodyRect.getX(), -bodyRect.getY()));
        synthPanelRects[(size_t) idx] = editorRect;
        synthPanelContent[(size_t) idx] = { 4, 24,
                                            panel.getWidth() - 8,
                                            panel.getHeight() - 28 };
    };

    setPanel(dcoPanel,   { bodyRect.getX(), bodyRect.getY() + topY, wDco, topH }, 0);
    setPanel(filterPanel,{ bodyRect.getX() + wDco + 10, bodyRect.getY() + topY, wFil, topH }, 1);
    setPanel(envPanel,   { bodyRect.getX() + wDco + 10 + wFil + 10, bodyRect.getY() + topY, wEnv, topH }, 2);

    int bx = bodyRect.getX();
    setPanel(subPanel,   { bx, bodyRect.getY() + bottomY, wSub, bottomH }, 3); bx += wSub + 10;
    setPanel(lfo1Panel,  { bx, bodyRect.getY() + bottomY, wL1, bottomH }, 4); bx += wL1 + 10;
    setPanel(lfo2Panel,  { bx, bodyRect.getY() + bottomY, wL2, bottomH }, 5); bx += wL2 + 10;
    setPanel(lfo3Panel,  { bx, bodyRect.getY() + bottomY, wL3, bottomH }, 6); bx += wL3 + 10;
    setPanel(voicePanel, { bx, bodyRect.getY() + bottomY, wVoice, bottomH }, 7);

    // DCO 1 & 2
    {
        const auto C = synthPanelContent[0];
        auto ys = S116::layout::distribute(C.getHeight(), { 80, 80 }, 4);
        const int cw = C.getWidth() / 5;
        auto cell = [&](int row, int col) {
            const int w = (col == 4) ? C.getWidth() - 4 * cw : cw;
            return juce::Rectangle<int>(C.getX() + col * cw, C.getY() + ys[(size_t) row], w, 80);
        };
        placeBare(dco1Morph.slider,  dco1Morph.label,  cell(0, 0), "DCO1 MORPH");
        placeBare(dco1PWM.slider,    dco1PWM.label,    cell(0, 1), "DCO1 PWM");
        placeBare(dco1Octave.slider, dco1Octave.label, cell(0, 2), "DCO1 OCTAVE");
        placeBare(dco1Fold.slider,   dco1Fold.label,   cell(0, 3), "DCO1 FOLD");
        placeBare(dco1Level.slider,  dco1Level.label,  cell(0, 4), "DCO1 LEVEL");
        placeBare(dco2Morph.slider,  dco2Morph.label,  cell(1, 0), "DCO2 MORPH");
        placeBare(dco2PWM.slider,    dco2PWM.label,    cell(1, 1), "DCO2 PWM");
        placeBare(dco2Semi.slider,   dco2Semi.label,   cell(1, 2), "DCO2 SEMI");
        placeBare(dco2Cents.slider,  dco2Cents.label,  cell(1, 3), "DCO2 DETUNE");
        placeBare(dco2Level.slider,  dco2Level.label,  cell(1, 4), "DCO2 LEVEL");
    }

    // Filter (HPF/VCF + filter envelope)
    {
        const auto C = synthPanelContent[1];
        auto ys = S116::layout::distribute(C.getHeight(), { 80, 80 }, 4);
        const int cw = C.getWidth() / 4;
        auto cell = [&](int row, int col) {
            return juce::Rectangle<int>(C.getX() + col * cw, C.getY() + ys[(size_t) row], cw, 80);
        };
        placeBare(hpfCutoff.slider, hpfCutoff.label, cell(0, 0), "HPF");
        placeBare(lpfCutoff.slider, lpfCutoff.label, cell(0, 1), "CUTOFF");
        placeBare(lpfRes.slider,    lpfRes.label,    cell(0, 2), "RESO");
        placeBare(envMod.slider,    envMod.label,    cell(0, 3), "ENV MOD");
        placeBare(filtA.slider,     filtA.label,     cell(1, 0), "FILT ATTACK");
        placeBare(filtD.slider,     filtD.label,     cell(1, 1), "FILT DECAY");
        placeBare(filtS.slider,     filtS.label,     cell(1, 2), "FILT SUSTAIN");
        placeBare(filtR.slider,     filtR.label,     cell(1, 3), "FILT RELEASE");
    }

    // Envelopes + master
    {
        const auto C = synthPanelContent[2];
        auto ys = S116::layout::distribute(C.getHeight(), { 80, 80 }, 4);
        const int cw = C.getWidth() / 4;
        auto cell = [&](int col) {
            return juce::Rectangle<int>(C.getX() + col * cw, C.getY() + ys[0], cw, 80);
        };
        placeBare(ampA.slider, ampA.label, cell(0), "AMP ATTACK");
        placeBare(ampD.slider, ampD.label, cell(1), "AMP DECAY");
        placeBare(ampS.slider, ampS.label, cell(2), "AMP SUSTAIN");
        placeBare(ampR.slider, ampR.label, cell(3), "AMP RELEASE");

        const int row2 = C.getY() + ys[1];
        S116::layout::placeField(playModeBox, playModeLabel, C.getX(), row2 + 14, 120);
        const int masterW = C.getWidth() - 3 * cw;
        placeBare(masterVol.slider, masterVol.label,
                  { C.getX() + 3 * cw, row2, masterW, 80 }, "MASTER VOLUME");
    }

    // Sub / Noise / Glide / X-Mod
    {
        const auto C = synthPanelContent[3];
        auto ys = S116::layout::distribute(C.getHeight(), { 168, 52, 34 }, 4);
        const int cw = C.getWidth() / 2;
        placeBare(subLevel.slider,   subLevel.label,   { C.getX(),          C.getY() + ys[0], cw, 80 }, "SUB OSC");
        placeBare(noiseLevel.slider, noiseLevel.label, { C.getX() + cw,     C.getY() + ys[0], cw, 80 }, "NOISE");
        placeBare(glideTime.slider,  glideTime.label,  { C.getX(),          C.getY() + ys[0] + 88, cw, 80 }, "GLIDE");
        placeBare(xmodAmount.slider, xmodAmount.label, { C.getX() + cw,     C.getY() + ys[0] + 88, cw, 80 }, "X-MOD");

        const int fieldW = (C.getWidth() - 8) / 2;
        S116::layout::placeField(subOctBox, subOctLabel, C.getX(), C.getY() + ys[1], fieldW);
        S116::layout::placeField(xmodModeBox, xmodModeLabel,
                                 C.getX() + fieldW + 8, C.getY() + ys[1], fieldW);

        syncBtn.setBounds(C.getX() + (C.getWidth() - 60) / 2, C.getY() + ys[2], 60, 34);
    }

    // LFO 1
    {
        const auto C = synthPanelContent[4];
        auto ys = S116::layout::distribute(C.getHeight(), { 52, 80 }, 4);
        S116::layout::placeField(lfo1ShapeBox, lfo1ShapeLabel, C.getX(),
                                 C.getY() + ys[0], juce::jmin(C.getWidth(), 110));
        const int cw = C.getWidth() / 3;
        auto cell = [&](int col) {
            return juce::Rectangle<int>(C.getX() + col * cw, C.getY() + ys[1], cw, 80);
        };
        placeBare(lfo1Rate.slider,    lfo1Rate.label,    cell(0), "LFO1 RATE");
        placeBare(lfo1ToFilt.slider,  lfo1ToFilt.label,  cell(1), "LFO1 VCF MOD");
        placeBare(lfo1ToPitch.slider, lfo1ToPitch.label, cell(2), "LFO1 PITCH");
    }

    // LFO 2
    {
        const auto C = synthPanelContent[5];
        auto ys = S116::layout::distribute(C.getHeight(), { 52, 52, 80 }, 4);
        S116::layout::placeField(lfo2ShapeBox, lfo2ShapeLabel, C.getX(),
                                 C.getY() + ys[0], C.getWidth());
        S116::layout::placeField(lfo2TargetBox, lfo2TargetLabel, C.getX(),
                                 C.getY() + ys[1], C.getWidth());
        const int cw = C.getWidth() / 2;
        placeBare(lfo2Rate.slider,   lfo2Rate.label,
                  { C.getX(), C.getY() + ys[2], cw, 80 }, "LFO2 RATE");
        placeBare(lfo2Amount.slider, lfo2Amount.label,
                  { C.getX() + cw, C.getY() + ys[2], cw, 80 }, "LFO2 DEPTH");
    }

    // LFO 3
    {
        const auto C = synthPanelContent[6];
        auto ys = S116::layout::distribute(C.getHeight(), { 52, 52, 80 }, 4);
        S116::layout::placeField(lfo3ShapeBox, lfo3ShapeLabel, C.getX(),
                                 C.getY() + ys[0], C.getWidth());
        S116::layout::placeField(lfo3TargetBox, lfo3TargetLabel, C.getX(),
                                 C.getY() + ys[1], C.getWidth());
        const int cw = C.getWidth() / 2;
        placeBare(lfo3Rate.slider,   lfo3Rate.label,
                  { C.getX(), C.getY() + ys[2], cw, 80 }, "LFO3 RATE");
        placeBare(lfo3Amount.slider, lfo3Amount.label,
                  { C.getX() + cw, C.getY() + ys[2], cw, 80 }, "LFO3 DEPTH");
    }

    // Voice Variation Matrix
    {
        const auto C = synthPanelContent[7];
        auto ys = S116::layout::distribute(C.getHeight(), { 52, 107, 107 }, 4);
        S116::layout::placeField(voiceVarModeBox, voiceVarLabel, C.getX(),
                                 C.getY() + ys[0], 120);
        cycleBtn.setBounds(C.getX() + 128, C.getY() + ys[0] + 9, 60, 34);

        const int cw = C.getWidth() / 3;
        auto cell = [&](int row, int col) {
            const int w = (col == 2) ? C.getWidth() - 2 * cw : cw;
            return juce::Rectangle<int>(C.getX() + col * cw,
                                        C.getY() + ys[(size_t) row], w, 107);
        };

        for (int i = 0; i < 3; ++i) {
            const auto c = cell(1, i);
            placeVoice(voiceKnobs[i].slider, voiceKnobs[i].label, c, "VOICE " + juce::String(i + 1));
            voicePanel.ledRects[(size_t) i] = { c.getRight() - 12,
                                                voiceKnobs[i].label.getY() + 5, 6, 6 };
        }
        for (int i = 0; i < 3; ++i) {
            const auto c = cell(2, i);
            placeVoice(voiceKnobs[3 + i].slider, voiceKnobs[3 + i].label, c,
                       "VOICE " + juce::String(4 + i));
            voicePanel.ledRects[(size_t) (3 + i)] = { c.getRight() - 12,
                                                      voiceKnobs[3 + i].label.getY() + 5, 6, 6 };
        }
    }
}

void Simple106AudioProcessorEditor::layoutFxPage()
{
    const int bodyW = bodyRect.getWidth();
    const int bodyH = bodyRect.getHeight();

    const int avail = bodyW - 20;
    const int weights = 148 + 288 + 216;
    const int wCh = avail * 148 / weights;
    const int wDelay = avail * 288 / weights;
    const int wRev = avail - wCh - wDelay;

    auto setPanel = [&](S116::SectionPanel& panel, juce::Rectangle<int> editorRect, int idx) {
        panel.setBounds(editorRect.translated(-bodyRect.getX(), -bodyRect.getY()));
        fxPanelRects[(size_t) idx] = editorRect;
        fxPanelContent[(size_t) idx] = { 4, 24,
                                         panel.getWidth() - 8,
                                         panel.getHeight() - 28 };
    };

    setPanel(chorusPanel, { bodyRect.getX(), bodyRect.getY(), wCh, bodyH }, 0);
    setPanel(delayPanel,  { bodyRect.getX() + wCh + 10, bodyRect.getY(), wDelay, bodyH }, 1);
    setPanel(reverbPanel, { bodyRect.getX() + wCh + 10 + wDelay + 10, bodyRect.getY(), wRev, bodyH }, 2);

    {
        const auto C = fxPanelContent[0];
        const int fieldW = juce::jmin(200, C.getWidth());
        S116::layout::placeField(chorusModeBox, chorusLabel,
                                 C.getX() + (C.getWidth() - fieldW) / 2,
                                 C.getY() + (C.getHeight() - 52) / 2, fieldW);
    }

    {
        const auto C = fxPanelContent[1];
        auto ys = S116::layout::distribute(C.getHeight(), { 52, 80, 34 }, 4);
        const int fieldW = juce::jmin(220, C.getWidth());
        S116::layout::placeField(delaySyncBox, delaySyncLabel,
                                 C.getX() + (C.getWidth() - fieldW) / 2,
                                 C.getY() + ys[0], fieldW);
        const int cw = C.getWidth() / 4;
        auto cell = [&](int col) {
            return juce::Rectangle<int>(C.getX() + col * cw, C.getY() + ys[1], cw, 80);
        };
        placeBare(delayTime.slider, delayTime.label, cell(0), "DELAY TIME");
        placeBare(delayFb.slider,   delayFb.label,   cell(1), "DELAY FEEDBACK");
        placeBare(delayDamp.slider, delayDamp.label, cell(2), "DELAY DAMPING");
        placeBare(delayMix.slider,  delayMix.label,  cell(3), "DELAY MIX");
        delayPingPongBtn.setBounds(C.getX() + (C.getWidth() - 100) / 2,
                                   C.getY() + ys[2], 100, 34);
    }

    {
        const auto C = fxPanelContent[2];
        const int cw = C.getWidth() / 3;
        const int rowY = C.getY() + (C.getHeight() - 80) / 2;
        auto cell = [&](int col) {
            return juce::Rectangle<int>(C.getX() + col * cw, rowY, cw, 80);
        };
        placeBare(reverbSize.slider, reverbSize.label, cell(0), "REVERB SIZE");
        placeBare(reverbDamp.slider, reverbDamp.label, cell(1), "REVERB DAMPING");
        placeBare(reverbMix.slider,  reverbMix.label,  cell(2), "REVERB MIX");
    }
}

void Simple106AudioProcessorEditor::layoutFooter()
{
    const auto C = juce::Rectangle<int>(4, 24,
                                        footerPanel.getWidth() - 8,
                                        footerPanel.getHeight() - 28);
    footerContent = C;

    int x = C.getX();
    const int y = C.getY();

    auto button = [&](juce::TextButton& b, int w) {
        b.setBounds(x, y, w, 34);
        x += w;
    };
    auto gap = [&](int g) { x += g; };

    button(seqPlayBtn, 52);  gap(6);
    button(seqRecBtn, 52);   gap(6);
    button(seqRestBtn, 52);  gap(6);
    button(seqClearBtn, 48); gap(10);

    button(arpToggleBtn, 52); gap(4);
    button(arpPrevBtn, 32);   gap(4);
    arpDisplay.setBounds(x, y + 4, 58, 26); x += 58; gap(4);
    button(arpNextBtn, 32);   gap(10);

    button(chordToggleBtn, 64); gap(4);
    button(chordPrevBtn, 32);   gap(4);
    chordDisplay.setBounds(x, y + 4, 68, 26); x += 68; gap(4);
    button(chordNextBtn, 32);   gap(10);

    stepsLabel.setBounds(x, y + 9, 34, 16); x += 34; gap(4);
    button(stepsPrevBtn, 32); gap(4);
    stepsDisplay.setBounds(x, y + 4, 36, 26); x += 36; gap(4);
    button(stepsNextBtn, 32); gap(10);

    pageLabel.setBounds(x, y + 9, 28, 16); x += 28; gap(4);
    button(pagePrevBtn, 32); gap(4);
    pageDisplay.setBounds(x, y + 4, 54, 26); x += 54; gap(4);
    button(pageNextBtn, 32);

    buttonKeyboard.setBounds(C.getX(), C.getY() + 40, C.getWidth(), 114);
}

void Simple106AudioProcessorEditor::layout()
{
    if (getWidth() <= 0 || getHeight() <= 0) return;

    headerRect = { kSideMargin, kTopMargin, getWidth() - 2 * kSideMargin, kHeaderH };
    const int bodyTop = headerRect.getBottom() + kHeaderBodyGap;
    const int footerTop = getHeight() - kBottomMargin - kFooterH;
    bodyRect = { kSideMargin, bodyTop, getWidth() - 2 * kSideMargin,
                 footerTop - kBodyFooterGap - bodyTop };
    footerRect = { kSideMargin, footerTop, getWidth() - 2 * kSideMargin, kFooterH };

    chassisBg.setBounds(getLocalBounds());
    headerBar.setBounds(headerRect);
    synthPage.setBounds(bodyRect);
    fxPage.setBounds(bodyRect);
    footerPanel.setBounds(footerRect);
    saveDialog.setBounds(getLocalBounds());

    synthPage.setVisible(currentTab == 0);
    fxPage.setVisible(currentTab == 1);

    knobCells.clear();

    layoutHeader();
    layoutSynthPage();
    layoutFxPage();
    layoutFooter();

    chassisBg.toBack();
    if (saveDialog.isVisible())
        saveDialog.toFront(false);

    // Some JUCE LAF children (notably combo labels) are created/positioned
    // during resized(); clear their focus flags after every layout.
    std::function<void(juce::Component&)> clearFocus = [&](juce::Component& c) {
        c.setWantsKeyboardFocus(false);
        c.setMouseClickGrabsKeyboardFocus(false);
        for (auto* child : c.getChildren())
            clearFocus(*child);
    };
    for (auto* child : getChildren())
        if (child != &saveDialog)
            clearFocus(*child);
}

void Simple106AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(S116::skin().theme().chassisBottom);
}

void Simple106AudioProcessorEditor::resized()
{
    layout();
}

void Simple106AudioProcessorEditor::focusGained(FocusChangeType)
{
    applyThisEditorsSkin();
}

void Simple106AudioProcessorEditor::focusLost(FocusChangeType) {
    if (hasKeyboardFocus(true))
        return;

    for (size_t i = 0; i < NUM_QWERTY_KEYS; ++i) {
        if (qwertyDownState[i]) {
            qwertyDownState[i] = false;
            audioProcessor.keyboardState.noteOff(1, qwertyActiveNote[i], 0.0f);
        }
    }
}

void Simple106AudioProcessorEditor::parentHierarchyChanged() {
    if (isShowing() && !saveDialog.isVisible())
        grabKeyboardFocus();
}

void Simple106AudioProcessorEditor::visibilityChanged() {
    if (isShowing() && !saveDialog.isVisible())
        grabKeyboardFocus();
}

void Simple106AudioProcessorEditor::mouseDown(const juce::MouseEvent& e) {
    if (!saveDialog.isVisible())
        grabKeyboardFocus();
    if (auto* s = sliderAt(e.eventComponent))
        showSliderValue(s);
}

void Simple106AudioProcessorEditor::mouseUp(const juce::MouseEvent&) {
    if (!saveDialog.isVisible())
        grabKeyboardFocus();
}

void Simple106AudioProcessorEditor::mouseDrag(const juce::MouseEvent& e) {
    if (auto* s = sliderAt(e.eventComponent))
        showSliderValue(s);
}

void Simple106AudioProcessorEditor::mouseMove(const juce::MouseEvent& e) {
    if (auto* s = sliderAt(e.eventComponent))
        showSliderValue(s);
}

void Simple106AudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& e,
                                                    const juce::MouseWheelDetails&) {
    if (auto* s = sliderAt(e.eventComponent))
        showSliderValue(s);
}

bool Simple106AudioProcessorEditor::keyPressed(const juce::KeyPress& key) {
    return handleKeyPress(key);
}

bool Simple106AudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component*) {
    return handleKeyPress(key);
}

bool Simple106AudioProcessorEditor::keyStateChanged(bool isKeyDown) {
    return handleKeyStateChanged(isKeyDown);
}

bool Simple106AudioProcessorEditor::keyStateChanged(bool isKeyDown, juce::Component*) {
    return handleKeyStateChanged(isKeyDown);
}

bool Simple106AudioProcessorEditor::handleKeyPress(const juce::KeyPress& key) {
    if (saveDialog.isVisible()) return false;

    int keyCode = key.getKeyCode();
    juce::juce_wchar textChar = key.getTextCharacter();

    if (keyCode == 'Z' || keyCode == 'z' || textChar == 'Z' || textChar == 'z') {
        buttonKeyboard.shiftOctave(-1);
        return true;
    }
    if (keyCode == 'X' || keyCode == 'x' || textChar == 'X' || textChar == 'x') {
        buttonKeyboard.shiftOctave(1);
        return true;
    }

    int upperCode = std::toupper(keyCode);
    juce::juce_wchar upperChar = (textChar != 0)
        ? static_cast<juce::juce_wchar>(std::toupper(textChar)) : 0;
    int baseMidi = 48 + buttonKeyboard.getOctaveOffset();

    for (size_t i = 0; i < NUM_QWERTY_KEYS; ++i) {
        int target = qwertyMappings[i].keyCode;
        if (upperCode == target || upperChar == static_cast<juce::juce_wchar>(target)) {
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

bool Simple106AudioProcessorEditor::handleKeyStateChanged(bool) {
    if (saveDialog.isVisible()) return false;

    for (size_t i = 0; i < NUM_QWERTY_KEYS; ++i) {
        if (qwertyDownState[i]) {
            int code = qwertyMappings[i].keyCode;
            bool isDown = juce::KeyPress::isKeyCurrentlyDown(code)
                       || juce::KeyPress::isKeyCurrentlyDown(std::tolower(code));
            if (!isDown) {
                qwertyDownState[i] = false;
                audioProcessor.keyboardState.noteOff(1, qwertyActiveNote[i], 0.0f);
            }
        }
    }
    return true;
}

// ==============================================================================
// Debug audit
// ==============================================================================
namespace {
    juce::Font auditLabelFont (const juce::Label& l)
    {
        const auto style = l.getProperties().getWithDefault ("style", "caption").toString();
        float size = 13.0f, kern = 0.085f;
        if (style == "field" || style == "sub") { size = 12.0f; kern = 0.10f; }
        else if (style == "accent") {
            size = (float) (double) l.getProperties().getWithDefault ("captionSize", 13.0);
            kern = 0.085f;
        }
        return S116::skin().ui (size, kern);
    }

    std::set<std::string> toStringSet (const std::vector<juce::String>& v)
    {
        std::set<std::string> out;
        for (const auto& s : v) out.insert (s.toStdString());
        return out;
    }
}

bool Simple106AudioProcessorEditor::collectBounds (juce::Component& parent,
                                                   const juce::String& name,
                                                   bool verbose, bool& ok) const
{
    auto isBackdrop = [this](juce::Component* c) {
        return c == &chassisBg || c == &saveDialog;
    };
    auto isPagePair = [this](juce::Component* a, juce::Component* b) {
        return (a == &synthPage && b == &fxPage) || (a == &fxPage && b == &synthPage);
    };
    auto isGrip = [](juce::Component* c) {
        return dynamic_cast<juce::ResizableCornerComponent*> (c) != nullptr;
    };

    const auto parentBounds = parent.getLocalBounds();
    auto& kids = parent.getChildren();

    if (verbose)
        std::cout << "  " << name << " " << parentBounds.getWidth()
                  << "x" << parentBounds.getHeight() << std::endl;

    for (int i = 0; i < kids.size(); ++i)
    {
        auto* c = kids[i];
        const auto b = c->getBounds();
        const auto childName = c->getName().isNotEmpty() ? c->getName()
                                                        : juce::String ("child ") + juce::String (i);

        if (!parentBounds.contains (b))
        {
            std::cout << "  FAIL " << name << " / " << childName
                      << " leaves parent: " << b.toString() << std::endl;
            ok = false;
        }

        // The modal save dialog is a hidden overlay with its own internal
        // text-editor components; it is audited by the focus check, not layout.
        if (c == &saveDialog) continue;

        if (auto* label = dynamic_cast<juce::Label*> (c))
        {
            const auto tw = S116::textWidth (auditLabelFont (*label), label->getText());
            if (tw > (float) label->getWidth() + 0.5f)
            {
                std::cout << "  FAIL " << name << " / " << childName << " text width " << tw
                          << " > cell " << label->getWidth()
                          << " (" << label->getText() << ")" << std::endl;
                ok = false;
            }
        }

        for (int j = i + 1; j < kids.size(); ++j)
        {
            auto* other = kids[j];
            if (isBackdrop (c) || isBackdrop (other)) continue;
            if (isPagePair (c, other)) continue;
            if (isGrip (c) || isGrip (other)) continue;
            if (b.intersects (other->getBounds()))
            {
                std::cout << "  FAIL " << name << " / " << childName << " overlaps "
                          << (other->getName().isNotEmpty() ? other->getName()
                                                            : juce::String ("child ") + juce::String (j))
                          << " " << b.toString() << " vs " << other->getBounds().toString() << std::endl;
                ok = false;
            }
        }

        if (verbose)
            std::cout << "    " << childName << " " << b.toString() << std::endl;

        collectBounds (*c, childName, verbose, ok);
    }

    return ok;
}

bool Simple106AudioProcessorEditor::auditFocus (bool verbose) const
{
    bool ok = true;

    std::function<void(juce::Component&)> visit = [&](juce::Component& c) {
        const bool wants = c.getWantsKeyboardFocus();
        const bool grabs = c.getMouseClickGrabsKeyboardFocus();

        if (wants || grabs)
        {
            bool insideTextEditor = dynamic_cast<juce::TextEditor*> (&c) != nullptr;
            for (auto* p = c.getParentComponent(); p != nullptr && !insideTextEditor;
                 p = p->getParentComponent())
                if (dynamic_cast<juce::TextEditor*> (p) != nullptr)
                    insideTextEditor = true;

            const bool isRoot = (&c == this);
            const bool isGrip = dynamic_cast<juce::ResizableCornerComponent*> (&c) != nullptr;
            const bool allowed = isRoot || isGrip || insideTextEditor;

            if (verbose) {
                auto* parent = c.getParentComponent();
                const auto parentName = parent == nullptr ? juce::String ("none")
                    : (parent->getName().isNotEmpty() ? parent->getName() : juce::String ("unnamed parent"));
                std::cout << "  FOCUS " << (c.getName().isNotEmpty() ? c.getName() : juce::String ("unnamed"))
                          << " parent=" << parentName
                          << " wants=" << (wants ? 1 : 0)
                          << " grabs=" << (grabs ? 1 : 0)
                          << (allowed ? " ALLOWED" : " FAIL") << std::endl;
            }
            if (!allowed) ok = false;
        }

        for (auto* child : c.getChildren())
            visit (*child);
    };

    visit (*const_cast<Simple106AudioProcessorEditor*> (this));
    return ok;
}

bool Simple106AudioProcessorEditor::auditAttachments (bool verbose) const
{
    bool ok = true;
    auto fail = [&ok](const juce::String& msg) {
        std::cout << "  ATTACH FAIL " << msg << std::endl;
        ok = false;
    };

    if (sliderAttachments.size() != 41) fail ("fixed slider attachments " + juce::String ((int) sliderAttachments.size()) + " != 41");
    if (voiceAttachments.size() != 6)   fail ("voice attachments " + juce::String ((int) voiceAttachments.size()) + " != 6");
    if (boxAttachments.size() != 11)    fail ("combo attachments " + juce::String ((int) boxAttachments.size()) + " != 11");
    if (btnAttachments.size() != 5)     fail ("button attachments " + juce::String ((int) btnAttachments.size()) + " != 5");

    std::set<std::string> baseline;
    for (const auto* id : kBaselineParamIds) baseline.insert (id);

    std::set<std::string> comboIds, buttonIds, manualIds, allVoice;
    const char* combos[] = { "subOctave", "xmodMode", "voiceVarMode", "lfo1Shape", "lfo2Shape",
                             "lfo2Target", "lfo3Shape", "lfo3Target", "playMode", "chorusMode", "delaySync" };
    const char* buttons[] = { "syncMode", "cycleMode", "delayPingPong", "arpEnable", "chordEnable" };
    const char* manuals[] = { "guiTheme", "ledTheme", "arpMode", "chordType", "seqPages" };
    for (const auto* id : combos) comboIds.insert (id);
    for (const auto* id : buttons) buttonIds.insert (id);
    for (const auto* id : manuals) manualIds.insert (id);
    for (int i = 1; i <= 6; ++i) {
        allVoice.insert ("vPan" + std::to_string (i));
        allVoice.insert ("vTune" + std::to_string (i));
    }

    std::set<std::string> expectedFixed = baseline;
    for (const auto& id : allVoice) expectedFixed.erase (id);
    for (const auto& id : comboIds) expectedFixed.erase (id);
    for (const auto& id : buttonIds) expectedFixed.erase (id);
    for (const auto& id : manualIds) expectedFixed.erase (id);

    const auto fixed = toStringSet (fixedAttachedIds);
    const auto comboSet = toStringSet (comboAttachedIds);
    const auto buttonSet = toStringSet (buttonAttachedIds);
    const auto voiceSet = toStringSet (voiceAttachedIds);

    if (fixed != expectedFixed) fail ("fixed slider IDs differ from expected 41");
    if (comboSet != comboIds) fail ("combo IDs differ from expected 11");
    if (buttonSet != buttonIds) fail ("button IDs differ from expected 5");

    std::set<std::string> currentVoice;
    const bool tuning = voiceVarModeBox.getSelectedItemIndex() == 1;
    for (int i = 1; i <= 6; ++i)
        currentVoice.insert ((tuning ? "vTune" : "vPan") + std::to_string (i));
    if (voiceSet != currentVoice) fail ("voice IDs do not match current matrix mode");

    std::set<std::string> ui = fixed;
    ui.insert (comboSet.begin(), comboSet.end());
    ui.insert (buttonSet.begin(), buttonSet.end());
    ui.insert (manualIds.begin(), manualIds.end());
    ui.insert (allVoice.begin(), allVoice.end());
    if (ui != baseline) fail ("UI ID union differs from 74-parameter baseline");

    std::set<std::string> runtime;
    auto state = audioProcessor.apvts.copyState();
    for (auto child : state) {
        const auto id = child.getProperty ("id").toString().toStdString();
        if (!id.empty()) runtime.insert (id);
    }
    if (runtime != baseline) {
        fail ("APVTS parameter IDs differ from baseline");
        for (const auto& id : baseline) if (runtime.find (id) == runtime.end())
            std::cout << "    missing: " << id << std::endl;
        for (const auto& id : runtime) if (baseline.find (id) == baseline.end())
            std::cout << "    extra: " << id << std::endl;
    }

    if (verbose)
    {
        std::cout << "  ATTACH fixed=" << sliderAttachments.size()
                  << " voice=" << voiceAttachments.size()
                  << " combo=" << boxAttachments.size()
                  << " button=" << btnAttachments.size()
                  << " ids=" << ui.size() << " baseline=" << baseline.size()
                  << " runtime=" << runtime.size() << std::endl;
        std::cout << "  ATTACH fixed IDs: " << fixed.size()
                  << ", combo: " << comboSet.size()
                  << ", button: " << buttonSet.size()
                  << ", voice: " << voiceSet.size() << std::endl;
    }

    return ok;
}

bool Simple106AudioProcessorEditor::auditVoiceMatrix (bool verbose)
{
    bool ok = true;
    auto fail = [&ok](const juce::String& msg) {
        std::cout << "  VOICE FAIL " << msg << std::endl;
        ok = false;
    };

    auto* modeParam = audioProcessor.apvts.getParameter ("voiceVarMode");
    if (modeParam == nullptr) {
        fail ("voiceVarMode parameter missing");
        return false;
    }

    const int originalMode = juce::jlimit (0, 1, (int) getSafeParamValue ("voiceVarMode", 0.0f));
    std::array<double, 6> savedSliders {};
    for (int i = 0; i < 6; ++i) savedSliders[(size_t) i] = voiceKnobs[i].slider.getValue();

    std::vector<std::pair<juce::String, float>> savedParams;
    for (int i = 1; i <= 6; ++i) {
        savedParams.emplace_back ("vPan" + juce::String (i), getSafeParamValue ("vPan" + juce::String (i)));
        savedParams.emplace_back ("vTune" + juce::String (i), getSafeParamValue ("vTune" + juce::String (i)));
    }

    auto setMode = [&](int mode) {
        modeParam->setValueNotifyingHost (modeParam->convertTo0to1 ((float) mode));
        voiceVarModeBox.setSelectedId (mode + 1, juce::dontSendNotification);
        updateVoiceKnobAttachments();
    };

    auto checkVoiceIds = [&](bool tune) {
        for (int i = 0; i < 6; ++i) {
            const auto expected = (tune ? "vTune" : "vPan") + juce::String (i + 1);
            if (i >= (int) voiceAttachedIds.size() || voiceAttachedIds[(size_t) i] != expected) {
                fail ("voice attachment " + juce::String (i + 1) + " is not " + expected);
                return;
            }
        }
    };

    setMode (0);
    checkVoiceIds (false);
    voiceKnobs[0].slider.setValue (0.0, juce::dontSendNotification);
    const auto mid = voiceKnobs[0].slider.getTextFromValue (0.0);
    voiceKnobs[0].slider.setValue (-0.5, juce::dontSendNotification);
    const auto left = voiceKnobs[0].slider.getTextFromValue (-0.5);
    if (mid != "MID") fail ("PANNING 0 is '" + mid + "', expected MID");
    if (left != "L50") fail ("PANNING -0.5 is '" + left + "', expected L50");

    setMode (1);
    checkVoiceIds (true);
    voiceKnobs[0].slider.setValue (12.0, juce::dontSendNotification);
    const auto tuneText = voiceKnobs[0].slider.getTextFromValue (12.0);
    if (! tuneText.endsWith ("ct")) fail ("TUNING text '" + tuneText + "' does not end in ct");

    setMode (originalMode);
    for (const auto& [id, value] : savedParams)
        if (auto* p = audioProcessor.apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    updateVoiceKnobAttachments();
    for (int i = 0; i < 6; ++i)
        voiceKnobs[i].slider.setValue (savedSliders[(size_t) i], juce::dontSendNotification);
    voiceVarModeBox.setSelectedId (originalMode + 1, juce::dontSendNotification);

    if (verbose)
        std::cout << "  VOICE MATRIX pan0=" << mid << " pan-0.5=" << left
                  << " tune12=" << tuneText
                  << " ids=" << voiceAttachedIds.size() << " " << (ok ? "PASS" : "FAIL") << std::endl;

    return ok;
}

bool Simple106AudioProcessorEditor::auditRepaintTargets (bool verbose) const
{
    bool ok = true;
    auto check = [&](const juce::Component* c, const juce::Component* parent, const char* label) {
        if (c == nullptr || c->getParentComponent() != parent) {
            std::cout << "  REPAINT FAIL " << label << " has wrong parent" << std::endl;
            ok = false;
        } else if (! parent->getLocalBounds().contains (c->getBounds())) {
            std::cout << "  REPAINT FAIL " << label << " leaves parent" << std::endl;
            ok = false;
        } else if (verbose) {
            std::cout << "  REPAINT " << label << " " << c->getBounds().toString()
                      << " component-local" << std::endl;
        }
    };

    check (&buttonKeyboard, &footerPanel, "keyboard");
    check (&voicePanel, &synthPage, "voice LEDs");
    check (&arpDisplay, &footerPanel, "ARP display");
    check (&chordDisplay, &footerPanel, "CHORD display");
    check (&stepsDisplay, &footerPanel, "STEPS display");
    check (&pageDisplay, &footerPanel, "PAGE display");

    if (verbose)
        std::cout << "  REPAINT no absolute editor repaint rectangles; timer uses voicePanel.repaint(),"
                  << " keyboard setters and display setText()" << std::endl;

    return ok;
}

void Simple106AudioProcessorEditor::runDebugAudit()
{
    const std::array<std::pair<int, int>, 3> sizes = {
        std::make_pair (1080, 840),
        std::make_pair (1200, 900),
        std::make_pair (1280, 960)
    };

    const int savedW = getWidth();
    const int savedH = getHeight();
    bool allOk = true;

    for (const auto& size : sizes)
    {
        setSize (size.first, size.second);
        resized();

        std::cout << "\n=== C1 AUDIT " << size.first << "x" << size.second << " ===" << std::endl;

        bool ok = true;
        collectBounds (*this, "EDITOR", true, ok);

        for (const auto& pt : panelTitles) {
            const auto tw = S116::textWidth (S116::skin().ui ((float) kPanelTitleFont, 0.15f), pt.second);
            const bool fits = tw + 12.0f <= (float) pt.first->getWidth() + 0.5f;
            std::cout << "  TITLE " << pt.second << ": " << juce::String (tw, 1)
                          << " + 12 <= " << pt.first->getWidth()
                          << " " << (fits ? "PASS" : "FAIL") << std::endl;
            if (!fits) ok = false;
        }

        for (const auto& k : knobCells) {
            if (k.slider == nullptr) continue;
            const int sx = k.slider->getBounds().getCentreX();
            const int cx = k.cell.getCentreX();
            const bool centred = sx == cx;
            const bool captionInside = k.caption == nullptr || k.cell.contains (k.caption->getBounds());
            std::cout << "  KNOB " << k.name << ": centre " << sx << " == " << cx
                          << " " << (centred ? "PASS" : "FAIL")
                          << ", caption " << (captionInside ? "PASS" : "FAIL") << std::endl;
            if (!centred || !captionInside) ok = false;
        }

        ok = auditFocus (true) && ok;
        ok = auditAttachments (true) && ok;
        ok = auditVoiceMatrix (true) && ok;
        ok = auditRepaintTargets (true) && ok;

        std::cout << "=== C1 AUDIT " << size.first << "x" << size.second
                  << (ok ? " PASS" : " FAIL") << " ===" << std::endl;
        jassert (ok);
        allOk = allOk && ok;
    }

    setSize (savedW, savedH);
    resized();

    // Behaviour smoke checks: exercise the preserved handlers after the layout
    // audits, then restore all touched state.
    bool behaviourOk = true;
    auto check = [&](const char* label, bool pass) {
        std::cout << "  BEHAVIOUR " << label << " " << (pass ? "PASS" : "FAIL") << std::endl;
        behaviourOk = behaviourOk && pass;
    };

    {
        const int oct0 = buttonKeyboard.getOctaveOffset();
        handleKeyPress (juce::KeyPress ('X'));
        const int oct1 = buttonKeyboard.getOctaveOffset();
        handleKeyPress (juce::KeyPress ('Z'));
        const int oct2 = buttonKeyboard.getOctaveOffset();
        check ("Z/X OCTAVE", oct1 == oct0 + 12 && oct2 == oct0);

        const int base = 48 + oct0;
        const bool noteWasOn = audioProcessor.keyboardState.isNoteOn (1, base);
        handleKeyPress (juce::KeyPress ('A'));
        const bool noteOn = audioProcessor.keyboardState.isNoteOn (1, base);
        handleKeyStateChanged (true);
        const bool noteOff = ! audioProcessor.keyboardState.isNoteOn (1, base);
        check ("QWERTY A", !noteWasOn && noteOn && noteOff);
    }

    {
        const int tab0 = currentTab;
        pageTabBtn.onClick();
        const bool tab1 = (currentTab != tab0)
            && (currentTab == 0 ? synthPage.isVisible() : fxPage.isVisible());
        pageTabBtn.onClick();
        check ("TAB SWITCH", tab1 && currentTab == tab0);

        showSaveDialog();
        const bool open = saveDialog.isVisible();
        hideSaveDialog();
        check ("SAVE DIALOG", open && ! saveDialog.isVisible());

        const bool play0 = audioProcessor.sequencer.isPlaying;
        seqPlayBtn.setToggleState (! play0, juce::dontSendNotification);
        seqPlayBtn.onClick();
        const bool play1 = audioProcessor.sequencer.isPlaying;
        seqPlayBtn.setToggleState (play0, juce::dontSendNotification);
        seqPlayBtn.onClick();
        check ("PLAY", play1 == ! play0 && audioProcessor.sequencer.isPlaying == play0);

        const bool rec0 = audioProcessor.sequencer.isRecording;
        seqRecBtn.setToggleState (! rec0, juce::dontSendNotification);
        seqRecBtn.onClick();
        const bool rec1 = audioProcessor.sequencer.isRecording;
        seqRecBtn.setToggleState (rec0, juce::dontSendNotification);
        seqRecBtn.onClick();
        check ("REC", rec1 == ! rec0 && audioProcessor.sequencer.isRecording == rec0);

        check ("PRESET LIST", presetBox.getNumItems() >= 8
                             && presetBox.getItemId (0) == 1
                             && presetBox.getItemText (0).startsWith ("01."));

        const char* arpNames[] = { "UP", "DOWN", "U-D", "RAND", "PLAY" };
        const int arp0 = currentArpIdx;
        arpNextBtn.onClick();
        const int arp1 = currentArpIdx;
        const bool arpDisplayOk = arpDisplay.getText() == arpNames[juce::jlimit (0, 4, arp1)];
        if (auto* p = audioProcessor.apvts.getParameter ("arpMode"))
            p->setValueNotifyingHost (static_cast<float> (arp0) / 4.0f);
        currentArpIdx = arp0;
        updateSegmentDisplays();
        check ("ARP READOUT", arp1 == (arp0 + 1) % 5 && arpDisplayOk);

        const char* chordNames[] = { "MAJ", "MIN", "MAJ7", "MIN7", "DOM7", "SUS4", "DIM", "OCT", "5TH" };
        const int chord0 = currentChordIdx;
        chordNextBtn.onClick();
        const int chord1 = currentChordIdx;
        const bool chordDisplayOk = chordDisplay.getText() == chordNames[juce::jlimit (0, 8, chord1)];
        if (auto* p = audioProcessor.apvts.getParameter ("chordType"))
            p->setValueNotifyingHost (static_cast<float> (chord0) / 8.0f);
        currentChordIdx = chord0;
        updateSegmentDisplays();
        check ("CHORD READOUT", chord1 == (chord0 + 1) % 9 && chordDisplayOk);

        const char* stepNames[] = { "16", "32", "48", "64" };
        const int pages0 = currentPagesIdx;
        stepsNextBtn.onClick();
        const int pages1 = currentPagesIdx;
        const bool stepsDisplayOk = stepsDisplay.getText() == stepNames[juce::jlimit (0, 3, pages1)];
        currentPagesIdx = pages0;
        audioProcessor.sequencer.setNumPages (pages0 + 1);
        if (auto* p = audioProcessor.apvts.getParameter ("seqPages"))
            p->setValueNotifyingHost (static_cast<float> (pages0) / 3.0f);
        updateSegmentDisplays();
        check ("STEPS READOUT", pages1 == (pages0 + 1) % 4 && stepsDisplayOk);

        const int page0 = currentSeqPage;
        const int maxPages = audioProcessor.sequencer.getNumPages();
        pageNextBtn.onClick();
        const int page1 = currentSeqPage;
        const bool pageDisplayOk = pageDisplay.getText() == "PG-" + juce::String (page1 + 1);
        currentSeqPage = page0;
        audioProcessor.sequencer.setRecordStep (page0 * 16);
        updateSegmentDisplays();
        check ("PAGE READOUT", page1 == (page0 + 1) % juce::jmax (1, maxPages) && pageDisplayOk);
    }

    std::cout << (behaviourOk ? "C1 BEHAVIOUR PASS" : "C1 BEHAVIOUR FAIL") << std::endl;
    allOk = allOk && behaviourOk;
    std::cout << (allOk ? "C1 AUDIT PASS" : "C1 AUDIT FAIL") << std::endl;
}
