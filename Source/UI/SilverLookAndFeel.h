#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class SilverLookAndFeel : public juce::LookAndFeel_V4 {
public:
    SilverLookAndFeel() {
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff14171a));
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x00000000));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
        setColour(juce::Slider::textBoxHighlightColourId, juce::Colour(0x402980b9));

        setColour(juce::Label::textColourId, juce::Colour(0xff14171a));
        setColour(juce::Label::backgroundColourId, juce::Colour(0x00000000));
        setColour(juce::Label::outlineColourId, juce::Colour(0x00000000));

        setColour(juce::TextEditor::textColourId, juce::Colour(0xff14171a));
        setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xffe2e5ea));
        setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff7a828e));

        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xffe2e5ea));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xff14171a));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff7a828e));
        setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff22272e));

        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xffe6e9ee));
        setColour(juce::PopupMenu::textColourId, juce::Colour(0xff14171a));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xffb8c0cc));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    }

    void setAccentColour(juce::Colour primary, juce::Colour glow, juce::Colour highlight) {
        accentPrimary = primary;
        accentGlow = glow;
        accentHighlight = highlight;
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override
                          {
                              auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
                              auto center = bounds.getCentre();
                              auto maxRadius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;

                              auto knobRadius = maxRadius - 7.0f;
                              if (knobRadius < 8.0f) knobRadius = 8.0f;

                              auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

                              // Radial Ticks
                              const int numTicks = 11;
                              float angleStep = (rotaryEndAngle - rotaryStartAngle) / static_cast<float>(numTicks - 1);

                              for (int i = 0; i < numTicks; ++i) {
                                  float angle = rotaryStartAngle + static_cast<float>(i) * angleStep;
                                  bool isMajor = (i == 0 || i == 5 || i == 10);

                                  float rInner = knobRadius + 2.0f;
                                  float rOuter = knobRadius + (isMajor ? 5.5f : 3.5f);

                                  float sinA = std::sin(angle);
                                  float cosA = -std::cos(angle);

                                  g.setColour(isMajor ? juce::Colour(0xff1a1d22) : juce::Colour(0xff555d6b));
                                  g.drawLine(center.x + rInner * sinA, center.y + rInner * cosA,
                                             center.x + rOuter * sinA, center.y + rOuter * cosA,
                                             isMajor ? 1.4f : 1.0f);
                              }

                              // Drop Shadow
                              g.setColour(juce::Colour(0x35000000));
                              g.fillEllipse(center.x - knobRadius + 1.0f, center.y - knobRadius + 2.0f, knobRadius * 2.0f, knobRadius * 2.0f);

                              // Bezel
                              juce::ColourGradient bezelGrad(juce::Colour(0xfff6f8fb), center.x - knobRadius, center.y - knobRadius,
                                                             juce::Colour(0xff6e7682), center.x + knobRadius, center.y + knobRadius, false);
                              g.setGradientFill(bezelGrad);
                              g.fillEllipse(center.x - knobRadius, center.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

                              // Dial Cap
                              auto innerRadius = knobRadius * 0.82f;
                              juce::ColourGradient dialGrad(juce::Colour(0xffe8ebf0), center.x, center.y - innerRadius,
                                                            juce::Colour(0xff9aa1ac), center.x, center.y + innerRadius, false);
                              g.setGradientFill(dialGrad);
                              g.fillEllipse(center.x - innerRadius, center.y - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

                              g.setColour(juce::Colour(0x40ffffff));
                              g.drawEllipse(center.x - innerRadius + 1.0f, center.y - innerRadius + 1.0f,
                                            (innerRadius - 1.0f) * 2.0f, (innerRadius - 1.0f) * 2.0f, 1.0f);

                              // Pointer
                              juce::Path p;
                              auto pointerLength = innerRadius * 0.90f;
                              auto pointerThickness = 2.2f;
                              p.addRoundedRectangle(-pointerThickness * 0.5f, -innerRadius, pointerThickness, pointerLength * 0.65f, 1.0f);
                              p.applyTransform(juce::AffineTransform::rotation(toAngle).translated(center.x, center.y));

                              g.setColour(juce::Colour(0xff14171a));
                              g.fillPath(p);
                          }

                          // Dynamic, Unclipped Themed Button Renderer
                          void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                    const juce::Colour& /*backgroundColour*/,
                                                    bool shouldDrawButtonAsHighlighted,
                                                    bool shouldDrawButtonAsDown) override
                                                    {
                                                        // Reduced by 1.5px to guarantee all strokes and glows stay strictly inside bounds
                                                        auto bounds = button.getLocalBounds().toFloat().reduced(1.5f);
                                                        bool isDown = shouldDrawButtonAsDown || button.getToggleState();

                                                        if (!isDown) {
                                                            // Unpressed 3D Drop Shadow
                                                            g.setColour(juce::Colour(0x35000000));
                                                            g.fillRoundedRectangle(bounds.translated(0.0f, 1.2f), 3.5f);

                                                            // Tactile Metallic Body
                                                            juce::Colour topCol = shouldDrawButtonAsHighlighted ? juce::Colour(0xff555e6c) : juce::Colour(0xff444c58);
                                                            juce::Colour botCol = shouldDrawButtonAsHighlighted ? juce::Colour(0xff333a44) : juce::Colour(0xff282f38);
                                                            juce::ColourGradient grad(topCol, bounds.getCentreX(), bounds.getY(),
                                                                                      botCol, bounds.getCentreX(), bounds.getBottom(), false);
                                                            g.setGradientFill(grad);
                                                            g.fillRoundedRectangle(bounds, 3.5f);

                                                            // Metallic Bevel Outline
                                                            g.setColour(juce::Colour(0xff6e7888));
                                                            g.drawRoundedRectangle(bounds, 3.5f, 1.2f);
                                                        } else {
                                                            // Distinct Active Inset with Themed Glow (100% Unclipped)
                                                            auto pressedBounds = bounds.translated(0.0f, 0.5f);

                                                            // Deep Inset Body
                                                            juce::ColourGradient activeGrad(juce::Colour(0xff15181e), pressedBounds.getCentreX(), pressedBounds.getY(),
                                                                                            juce::Colour(0xff222730), pressedBounds.getCentreX(), pressedBounds.getBottom(), false);
                                                            g.setGradientFill(activeGrad);
                                                            g.fillRoundedRectangle(pressedBounds, 3.5f);

                                                            // Themed Subtle Inner Tint
                                                            g.setColour(accentPrimary.withAlpha(0.15f));
                                                            g.fillRoundedRectangle(pressedBounds.reduced(1.0f), 2.5f);

                                                            // Themed Outer Glow
                                                            g.setColour(accentGlow);
                                                            g.drawRoundedRectangle(pressedBounds, 3.5f, 2.0f);

                                                            // Themed Crisp Active Outline
                                                            g.setColour(accentPrimary);
                                                            g.drawRoundedRectangle(pressedBounds, 3.5f, 1.2f);
                                                        }
                                                    }

                                                    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                                                        bool /*shouldDrawButtonAsHighlighted*/, bool shouldDrawButtonAsDown) override
                                                                        {
                                                                            auto bounds = button.getLocalBounds().toFloat();
                                                                            bool isDown = shouldDrawButtonAsDown || button.getToggleState();

                                                                            g.setColour(isDown ? juce::Colour(0xffffffff) : juce::Colour(0xffdcdfe5));
                                                                            g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
                                                                            g.drawText(button.getButtonText(), bounds.translated(0.0f, isDown ? 0.5f : 0.0f), juce::Justification::centred, false);
                                                                        }

                                                                        void drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                                                                          int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                                                                          juce::ComboBox& box) override
                                                                                          {
                                                                                              auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height)).reduced(1.0f);

                                                                                              juce::ColourGradient bgGrad(juce::Colour(0xfff3f5f8), 0, 0,
                                                                                                                          juce::Colour(0xffcdd2da), 0, static_cast<float>(height), false);
                                                                                              g.setGradientFill(bgGrad);
                                                                                              g.fillRoundedRectangle(bounds, 3.5f);

                                                                                              g.setColour(juce::Colour(0xff68707d));
                                                                                              g.drawRoundedRectangle(bounds, 3.5f, 1.2f);

                                                                                              auto arrowZone = juce::Rectangle<float>(static_cast<float>(width) - 20.0f, 0.0f, 16.0f, static_cast<float>(height));
                                                                                              juce::Path path;
                                                                                              path.addTriangle(arrowZone.getCentreX() - 4.0f, arrowZone.getCentreY() - 2.0f,
                                                                                                               arrowZone.getCentreX() + 4.0f, arrowZone.getCentreY() - 2.0f,
                                                                                                               arrowZone.getCentreX(), arrowZone.getCentreY() + 3.0f);
                                                                                              g.setColour(box.findColour(juce::ComboBox::arrowColourId));
                                                                                              g.fillPath(path);
                                                                                          }

                                                                                          juce::Label* createSliderTextBox(juce::Slider& slider) override {
                                                                                              auto* l = LookAndFeel_V4::createSliderTextBox(slider);
                                                                                              l->setColour(juce::Label::textColourId, juce::Colour(0xff14171a));
                                                                                              l->setColour(juce::Label::backgroundColourId, juce::Colour(0x00000000));
                                                                                              l->setColour(juce::Label::outlineColourId, juce::Colour(0x00000000));
                                                                                              l->setFont(juce::FontOptions(11.0f).withStyle("Bold"));
                                                                                              l->setJustificationType(juce::Justification::centred);
                                                                                              return l;
                                                                                          }

private:
    juce::Colour accentPrimary   { 0xffff2d55 };
    juce::Colour accentGlow      { 0x60ff2d55 };
    juce::Colour accentHighlight { 0xffffa2b0 };
};
