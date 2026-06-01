#include "ModeSelector.h"
#include "../params/Parameters.h"
#include "Theme.h"

namespace arch
{
ModeSelector::ModeSelector (juce::AudioProcessorValueTreeState& apvts) : apvts_ (apvts)
{
    modeParam_ = apvts_.getParameter (pid::mode);

    static const char* blurb[(size_t) Mode::Count] = {
        "sparse \xc2\xb7 gentle \xc2\xb7 long fades",
        "angular \xc2\xb7 glitch \xc2\xb7 unstable",
        "tonal \xc2\xb7 uncanny \xc2\xb7 music-box",
        "wide \xc2\xb7 soft \xc2\xb7 cool air",
        "autonomous \xc2\xb7 it answers you"
    };

    for (int i = 0; i < (int) Mode::Count; ++i)
    {
        auto& b = buttons_[(size_t) i];
        b.setButtonText (juce::String::fromUTF8 (modeName ((Mode) i)));
        b.setClickingTogglesState (false);
        b.setTooltip (juce::String::fromUTF8 (blurb[(size_t) i]));
        b.onClick = [this, i]
        {
            if (modeParam_ != nullptr)
            {
                modeParam_->beginChangeGesture();
                modeParam_->setValueNotifyingHost (
                    modeParam_->getNormalisableRange().convertTo0to1 ((float) i));
                modeParam_->endChangeGesture();
            }
            refresh();
        };
        addAndMakeVisible (b);
    }
    refresh();
}

void ModeSelector::refresh()
{
    if (modeParam_ == nullptr) return;
    const int cur = (int) modeParam_->getNormalisableRange()
                        .convertFrom0to1 (modeParam_->getValue());
    for (int i = 0; i < (int) Mode::Count; ++i)
        buttons_[(size_t) i].setToggleState (i == cur, juce::dontSendNotification);
    repaint();
}

void ModeSelector::resized()
{
    auto r = getLocalBounds().reduced (2);
    const int n = (int) Mode::Count;
    const int gap = 6;
    const int w = (r.getWidth() - gap * (n - 1)) / n;
    for (int i = 0; i < n; ++i)
        buttons_[(size_t) i].setBounds (r.getX() + i * (w + gap), r.getY(), w, r.getHeight());
}

void ModeSelector::paint (juce::Graphics&) {}
} // namespace arch
