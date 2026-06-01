#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arch
{
/** Quiet, tactile museum-instrument look: brass rings, smoked panels, ink type. */
class ArchaeologistLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ArchaeologistLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle,
                           juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& bg, bool over, bool down) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool over, bool down) override;

    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getComboFont (juce::ComboBox&);

    void drawComboBox (juce::Graphics&, int w, int h, bool down,
                       int bx, int by, int bw, int bh, juce::ComboBox&) override;
};
} // namespace arch
