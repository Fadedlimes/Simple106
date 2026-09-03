#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class SilverLookAndFeel : public juce::LookAndFeel_V4 {
public:
    SilverLookAndFeel() {
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffff2d55));
        setColour(juce::Slider::thumbColourId, juce::Colour(0xfff0f2f5));
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff14171a));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xffdcdfe4));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffcbd0d8));
        setColour(juce::TextButton::textColourOffId, juce::Colour(0xff14171a));
        setColour(juce::TextButton::textColourOnId, juce::Colour(0xff14171a));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff181b20));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xfff0f2f5));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff4a5260));
        setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffbac0cc));
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1c2026));
        setColour(juce::PopupMenu::textColourId, juce::Colour(0xfff0f2f5));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff333a46));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xffffffff));
    }

    void setAccentColour(juce::Colour primary, juce::Colour glow, juce::Colour highlight) {
        accentPrimary = primary;
        accentGlow = glow;
        accentHighlight = highlight;
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override {
                              auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                                   static_cast<float>(width), static_cast<float>(height));
                              float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 6.0f;
                              if (diameter <= 0.0f) return;

                              float cx = bounds.getCentreX();
                              float cy = bounds.getCentreY();
                              float radius = diameter * 0.5f;

                              // Outer Drop Shadow
                              g.setColour(juce::Colour(0x35000000));
                              g.fillEllipse(cx - radius + 1.0f, cy - radius + 2.0f, radius * 2.0f, radius * 2.0f);

                              // Machined Dark Outer Ring
                              g.setColour(juce::Colour(0xff3a404a));
                              g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
                              g.setColour(juce::Colour(0xff1a1d22));
                              g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.0f);

                              // Turned-Metal Brushed Dial Body
                              float dialRadius = radius - 2.5f;
                              juce::ColourGradient dialGrad(juce::Colour(0xfff8f9fb), cx - dialRadius, cy - dialRadius,
                                                            juce::Colour(0xffb0b5be), cx + dialRadius, cy + dialRadius, true);
                              dialGrad.addColour(0.3, juce::Colour(0xffe6e9ee));
                              dialGrad.addColour(0.7, juce::Colour(0xffc2c7d0));
                              g.setGradientFill(dialGrad);
                              g.fillEllipse(cx - dialRadius, cy - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f);

                              // Radial Micro-Notches (Turned Aluminum Finish)
                              g.setColour(juce::Colour(0x28000000));
                              for (int i = 0; i < 16; ++i) {
                                  float a = static_cast<float>(i) * (juce::MathConstants<float>::twoPi / 16.0f);
                                  float innerX = cx + std::cos(a) * (dialRadius - 3.5f);
                                  float innerY = cy + std::sin(a) * (dialRadius - 3.5f);
                                  float outerX = cx + std::cos(a) * dialRadius;
                                  float outerY = cy + std::sin(a) * dialRadius;
                                  g.drawLine(innerX, innerY, outerX, outerY, 0.8f);
                              }

                              // Inner Beveled Cap
                              float capRadius = dialRadius * 0.58f;
                              juce::ColourGradient capGrad(juce::Colour(0xffdcdfe5), cx - capRadius, cy - capRadius,
                                                           juce::Colour(0xff8c93a0), cx + capRadius, cy + capRadius, false);
                              g.setGradientFill(capGrad);
                              g.fillEllipse(cx - capRadius, cy - capRadius, capRadius * 2.0f, capRadius * 2.0f);
                              g.setColour(juce::Colour(0xff68707d));
                              g.drawEllipse(cx - capRadius, cy - capRadius, capRadius * 2.0f, capRadius * 2.0f, 0.8f);

                              // Pointer Angle Calculation
                              float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

                              // Accent Glowing Pointer Line
                              float startPointerR = capRadius - 1.0f;
                              float endPointerR = dialRadius - 1.0f;
                              float px1 = cx + std::sin(angle) * startPointerR;
                              float py1 = cy - std::cos(angle) * startPointerR;
                              float px2 = cx + std::sin(angle) * endPointerR;
                              float py2 = cy - std::cos(angle) * endPointerR;

                              g.setColour(accentGlow);
                              g.drawLine(px1, py1, px2, py2, 3.8f);
                              g.setColour(accentPrimary);
                              g.drawLine(px1, py1, px2, py2, 2.2f);
                              g.setColour(accentHighlight);
                              g.drawLine(px1, py1, px2, py2, 1.0f);
                          }

                          void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                    const juce::Colour& backgroundColour,
                                                    bool shouldDrawButtonAsHighlighted,
                                                    bool shouldDrawButtonAsDown) override {
                                                        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
                                                        float corner = 3.5f;

                                                        bool isDown = shouldDrawButtonAsDown || button.getToggleState();

                                                        // Button Outer Chassis Frame
                                                        g.setColour(juce::Colour(0xff23272e));
                                                        g.fillRoundedRectangle(bounds, corner);

                                                        auto inner = bounds.reduced(1.0f);

                                                        if (isDown) {
                                                            juce::ColourGradient downGrad(juce::Colour(0xffb0b5be), inner.getX(), inner.getY(),
                                                                                          juce::Colour(0xffdcdfe5), inner.getX(), inner.getBottom(), false);
                                                            g.setGradientFill(downGrad);
                                                            g.fillRoundedRectangle(inner, corner - 0.5f);
                                                            g.setColour(juce::Colour(0xff4a5260));
                                                            g.drawRoundedRectangle(inner, corner - 0.5f, 1.0f);
                                                        } else {
                                                            juce::Colour base = shouldDrawButtonAsHighlighted ? juce::Colour(0xfff0f2f6) : juce::Colour(0xffe0e3e8);
                                                            juce::ColourGradient upGrad(base, inner.getX(), inner.getY(),
                                                                                        juce::Colour(0xffcbd0d8), inner.getX(), inner.getBottom(), false);
                                                            g.setGradientFill(upGrad);
                                                            g.fillRoundedRectangle(inner, corner - 0.5f);

                                                            // Bevel Highlight
                                                            g.setColour(juce::Colour(0x90ffffff));
                                                            g.drawLine(inner.getX() + 2.0f, inner.getY() + 1.0f, inner.getRight() - 2.0f, inner.getY() + 1.0f, 1.0f);

                                                            g.setColour(juce::Colour(0xff717986));
                                                            g.drawRoundedRectangle(inner, corner - 0.5f, 1.0f);
                                                        }
                                                    }

                                                    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                                                        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
                                                                            juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
                                                                            g.setFont(juce::FontOptions(10.5f).withStyle("Bold"));
                                                                            g.setColour(juce::Colour(0xff14171a));

                                                                            auto bounds = button.getLocalBounds();
                                                                            if (button.isToggleable()) {
                                                                                bounds.removeFromLeft(12); // Leave room for status LED
                                                                            }
                                                                            g.drawFittedText(button.getButtonText(), bounds, juce::Justification::centred, 1);
                                                                        }

                                                                        void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                                                                          int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override {
                                                                                              juce::ignoreUnused(isButtonDown, buttonX, buttonY, buttonW, buttonH);
                                                                                              auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));

                                                                                              g.setColour(juce::Colour(0xff121519));
                                                                                              g.fillRoundedRectangle(bounds, 3.0f);
                                                                                              g.setColour(juce::Colour(0xff4a5260));
                                                                                              g.drawRoundedRectangle(bounds, 3.0f, 1.2f);

                                                                                              // Arrow
                                                                                              auto arrowBox = juce::Rectangle<float>(static_cast<float>(width - 18), 0.0f, 14.0f, static_cast<float>(height));
                                                                                              juce::Path p;
                                                                                              p.startNewSubPath(arrowBox.getCentreX() - 3.5f, arrowBox.getCentreY() - 2.0f);
                                                                                              p.lineTo(arrowBox.getCentreX() + 3.5f, arrowBox.getCentreY() - 2.0f);
                                                                                              p.lineTo(arrowBox.getCentreX(), arrowBox.getCentreY() + 2.5f);
                                                                                              p.closeSubPath();
                                                                                              g.setColour(juce::Colour(0xffa5b1c2));
                                                                                              g.fillPath(p);
                                                                                          }

private:
    juce::Colour accentPrimary { 0xffff2d55 };
    juce::Colour accentGlow { 0x60ff2d55 };
    juce::Colour accentHighlight { 0xffffa2b0 };
};
