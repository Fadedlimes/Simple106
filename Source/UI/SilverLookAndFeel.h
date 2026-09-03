#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

struct GuiThemePalette {
    juce::Colour panelBg         { 0xffcbcfd6 };
    juce::Colour sectionBorder  { 0xffbac0cc };
    juce::Colour labelText       { 0xff14171a };
    
    // Knobs
    juce::Colour knobBezelTop    { 0xfff6f8fb };
    juce::Colour knobBezelBottom { 0xff6e7682 };
    juce::Colour knobCapTop      { 0xffe8ebf0 };
    juce::Colour knobCapBottom   { 0xff9aa1ac };
    juce::Colour knobPointer     { 0xff14171a };
    juce::Colour majorTick       { 0xff1a1d22 };
    juce::Colour minorTick       { 0xff555d6b };

    // Dropdowns & Controls
    juce::Colour comboBgTop      { 0xfff3f5f8 };
    juce::Colour comboBgBottom   { 0xffcdd2da };
    juce::Colour comboBorder     { 0xff68707d };
    juce::Colour comboText       { 0xff14171a };
    juce::Colour comboArrow      { 0xff22272e };

    juce::Colour popupBg         { 0xffe6e9ee };
    juce::Colour popupText       { 0xff14171a };
    juce::Colour popupHighlight  { 0xffb8c0cc };
};

class SilverLookAndFeel : public juce::LookAndFeel_V4 {
public:
    SilverLookAndFeel() {
        applyThemePalette(palette);
    }

    void setThemePalette(const GuiThemePalette& p) {
        palette = p;
        applyThemePalette(palette);
    }

    void setAccentColour(juce::Colour primary, juce::Colour glow, juce::Colour highlight) {
        accentPrimary = primary;
        accentGlow = glow;
        accentHighlight = highlight;
    }

    void applyThemePalette(const GuiThemePalette& p) {
        setColour(juce::Slider::textBoxTextColourId, p.comboText);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x00000000));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
        setColour(juce::Slider::textBoxHighlightColourId, juce::Colour(0x402980b9));

        setColour(juce::Label::textColourId, p.labelText);
        setColour(juce::Label::backgroundColourId, juce::Colour(0x00000000));
        setColour(juce::Label::outlineColourId, juce::Colour(0x00000000));

        setColour(juce::TextEditor::textColourId, p.comboText);
        setColour(juce::TextEditor::backgroundColourId, p.comboBgTop);
        setColour(juce::TextEditor::outlineColourId, p.comboBorder);

        setColour(juce::ComboBox::backgroundColourId, p.comboBgTop);
        setColour(juce::ComboBox::textColourId, p.comboText);
        setColour(juce::ComboBox::outlineColourId, p.comboBorder);
        setColour(juce::ComboBox::arrowColourId, p.comboArrow);

        setColour(juce::PopupMenu::backgroundColourId, p.popupBg);
        setColour(juce::PopupMenu::textColourId, p.popupText);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, p.popupHighlight);
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    }

    void drawRotarySlider(juce::Graphics& g,