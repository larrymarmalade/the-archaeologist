#include "MacroPanel.h"
#include "../params/Parameters.h"
#include "Theme.h"

namespace arch
{
MacroPanel::MacroPanel (juce::AudioProcessorValueTreeState& apvts)
{
    // Six big performance macros
    addKnob (apvts, pid::memoryDepth,    "DEPTH",   true);
    addKnob (apvts, pid::excavationRate, "RATE",    true);
    addKnob (apvts, pid::ruin,           "RUIN",    true);
    addKnob (apvts, pid::beauty,         "BEAUTY",  true);
    addKnob (apvts, pid::density,        "DENSITY", true);
    addKnob (apvts, pid::forgetting,     "MEMORY",  true);
    bigCount_ = (int) knobs_.size();

    // Secondary finer controls
    addKnob (apvts, pid::recognition,    "RECOGNITION", false);
    addKnob (apvts, pid::artifactLength, "LENGTH",      false);
    addKnob (apvts, pid::fidelityDecay,  "FIDELITY",    false);
    addKnob (apvts, pid::continuity,     "CONTINUITY",  false);
}

void MacroPanel::addKnob (juce::AudioProcessorValueTreeState& apvts, const char* paramId,
                          const juce::String& name, bool big)
{
    auto k = std::make_unique<Knob>();
    k->big = big;
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k->slider.setTooltip (apvts.getParameter (paramId)->getName (64));
    addAndMakeVisible (k->slider);

    k->label.setText (name, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setColour (juce::Label::textColourId, big ? theme::ink : theme::inkSoft);
    k->label.setFont (juce::Font (juce::FontOptions (big ? 13.0f : 10.5f)).withExtraKerningFactor (0.08f));
    addAndMakeVisible (k->label);

    k->attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, paramId, k->slider);

    knobs_.push_back (std::move (k));
}

void MacroPanel::resized()
{
    auto r = getLocalBounds().reduced (6);
    const int bigRowH = (int) (r.getHeight() * 0.62f);
    auto bigRow = r.removeFromTop (bigRowH);
    r.removeFromTop (4);
    auto smallRow = r;

    auto layout = [] (juce::Rectangle<int> area, std::vector<Knob*>& ks, float labelH)
    {
        if (ks.empty()) return;
        const int gap = 8;
        const int w = (area.getWidth() - gap * ((int) ks.size() - 1)) / (int) ks.size();
        for (size_t i = 0; i < ks.size(); ++i)
        {
            juce::Rectangle<int> cell (area.getX() + (int) i * (w + gap), area.getY(), w, area.getHeight());
            auto lbl = cell.removeFromBottom ((int) labelH);
            ks[i]->slider.setBounds (cell.reduced (2));
            ks[i]->label.setBounds (lbl);
        }
    };

    std::vector<Knob*> big, small;
    for (int i = 0; i < (int) knobs_.size(); ++i)
        (i < bigCount_ ? big : small).push_back (knobs_[(size_t) i].get());

    layout (bigRow, big, 18.0f);
    layout (smallRow, small, 14.0f);
}

void MacroPanel::paint (juce::Graphics& g)
{
    g.setColour (theme::paperDeep);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
    g.setColour (theme::brass.withAlpha (0.3f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);
}
} // namespace arch
