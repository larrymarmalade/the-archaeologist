#include "ArchaeologistLookAndFeel.h"
#include "Theme.h"

namespace arch
{
ArchaeologistLookAndFeel::ArchaeologistLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, theme::paper);
    setColour (juce::Slider::textBoxTextColourId, theme::ink);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, theme::ink);
    setColour (juce::ComboBox::backgroundColourId, theme::smoke);
    setColour (juce::ComboBox::textColourId, theme::paper);
    setColour (juce::ComboBox::outlineColourId, theme::brassDim);
    setColour (juce::PopupMenu::backgroundColourId, theme::smoke);
    setColour (juce::PopupMenu::textColourId, theme::paper);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, theme::brassDim);
}

void ArchaeologistLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                                 float pos, float a0, float a1, juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> (x, y, w, h).reduced (6.0f);
    const auto centre = bounds.getCentre();
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float ang = a0 + pos * (a1 - a0);

    // smoked glass well
    g.setColour (theme::smoke.withAlpha (0.85f));
    g.fillEllipse (bounds);
    g.setColour (theme::glassEdge);
    g.drawEllipse (bounds, 1.0f);

    // brass arc track
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f, a0, a1, true);
    g.setColour (theme::brassDim.withAlpha (0.45f));
    g.strokePath (track, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f, a0, ang, true);
    g.setColour (theme::amber);
    g.strokePath (value, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // pointer
    const float px = centre.x + std::cos (ang - juce::MathConstants<float>::halfPi) * (radius - 8.0f);
    const float py = centre.y + std::sin (ang - juce::MathConstants<float>::halfPi) * (radius - 8.0f);
    g.setColour (theme::paper);
    g.drawLine (centre.x, centre.y, px, py, 2.2f);
    g.setColour (theme::brass);
    g.fillEllipse (centre.x - 3.0f, centre.y - 3.0f, 6.0f, 6.0f);
}

void ArchaeologistLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                                     const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const float corner = 4.0f;
    auto base = b.getToggleState() ? theme::brassDim : theme::smoke;
    if (down) base = base.brighter (0.15f);
    else if (over) base = base.brighter (0.08f);
    g.setColour (base);
    g.fillRoundedRectangle (r, corner);
    g.setColour (b.getToggleState() ? theme::amber : theme::brassDim.withAlpha (0.7f));
    g.drawRoundedRectangle (r, corner, 1.2f);
}

void ArchaeologistLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                                 bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    auto base = b.getToggleState() ? theme::brassDim : theme::smoke.withAlpha (0.8f);
    if (down) base = base.brighter (0.12f); else if (over) base = base.brighter (0.06f);
    g.setColour (base);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (b.getToggleState() ? theme::amber : theme::brassDim.withAlpha (0.6f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    g.setColour (b.getToggleState() ? theme::paper : theme::paper.withAlpha (0.7f));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText (b.getButtonText(), r.reduced (8.0f, 0.0f), juce::Justification::centredLeft, true);
}

juce::Font ArchaeologistLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (juce::FontOptions (13.0f));
}

juce::Font ArchaeologistLookAndFeel::getComboFont (juce::ComboBox&)
{
    return juce::Font (juce::FontOptions (13.0f));
}

void ArchaeologistLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool,
                                             int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    g.setColour (theme::smoke);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (theme::brassDim.withAlpha (0.7f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    juce::Path tri;
    const float cx = (float) w - 14.0f, cy = (float) h * 0.5f;
    tri.addTriangle (cx - 4, cy - 2, cx + 4, cy - 2, cx, cy + 3);
    g.setColour (theme::amber);
    g.fillPath (tri);
    juce::ignoreUnused (box);
}
} // namespace arch
