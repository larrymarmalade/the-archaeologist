#include "TransportBar.h"
#include "../PluginProcessor.h"
#include "../params/Parameters.h"
#include "../presets/Presets.h"
#include "Theme.h"

namespace arch
{
TransportBar::TransportBar (ArchaeologistProcessor& proc, juce::AudioProcessorValueTreeState& apvts)
    : proc_ (proc), apvts_ (apvts)
{
    digBtn_.onClick   = [this] { pulse (pid::digNow); };
    clearBtn_.onClick = [this] { pulse (pid::clearArchive); };
    addAndMakeVisible (digBtn_);
    addAndMakeVisible (clearBtn_);

    auto setupToggle = [this] (juce::ToggleButton& b, const juce::String& text)
    {
        b.setButtonText (text);
        addAndMakeVisible (b);
    };
    setupToggle (freezeBtn_,  "Freeze");
    setupToggle (silenceBtn_, "Silence Favors Ghosts");
    setupToggle (syncBtn_,    "Sync");
    setupToggle (choirBtn_,   "Choir");
    setupToggle (relicBtn_,   "Relic Stack");

    freezeAtt_  = std::make_unique<BA> (apvts_, pid::freeze,       freezeBtn_);
    silenceAtt_ = std::make_unique<BA> (apvts_, pid::favorSilence, silenceBtn_);
    syncAtt_    = std::make_unique<BA> (apvts_, pid::syncToHost,   syncBtn_);
    choirAtt_   = std::make_unique<BA> (apvts_, pid::choir,        choirBtn_);
    relicAtt_   = std::make_unique<BA> (apvts_, pid::relicStack,   relicBtn_);

    windowBox_.addItemList ({ "30 s", "2 min", "5 min", "15 min" }, 1);
    quantizeBox_.addItemList ({ "Off", "Loose", "Bar", "Phrase" }, 1);
    addAndMakeVisible (windowBox_);
    addAndMakeVisible (quantizeBox_);
    windowAtt_   = std::make_unique<CA> (apvts_, pid::memoryWindow, windowBox_);
    quantizeAtt_ = std::make_unique<CA> (apvts_, pid::quantize,     quantizeBox_);

    for (int i = 0; i < presets::count(); ++i)
        presetBox_.addItem (presets::name (i), i + 1);
    presetBox_.setTextWhenNothingSelected ("Presets");
    presetBox_.onChange = [this]
    {
        const int idx = presetBox_.getSelectedId() - 1;
        if (idx >= 0) proc_.setCurrentProgram (idx);
    };
    addAndMakeVisible (presetBox_);

    auto setupKnob = [this] (juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (juce::FontOptions (10.5f)));
        addAndMakeVisible (l);
    };
    setupKnob (wetSlider_, wetLabel_, "WET");
    setupKnob (gainSlider_, gainLabel_, "OUT");
    wetAtt_  = std::make_unique<SA> (apvts_, pid::wetDry,     wetSlider_);
    gainAtt_ = std::make_unique<SA> (apvts_, pid::outputGain, gainSlider_);

    refreshPreset();
}

void TransportBar::pulse (const char* paramId)
{
    if (auto* p = apvts_.getParameter (paramId))
    {
        p->setValueNotifyingHost (1.0f);
        juce::Component::SafePointer<TransportBar> safe (this);
        juce::Timer::callAfterDelay (120, [safe, paramId]
        {
            if (safe != nullptr)
                if (auto* pp = safe->apvts_.getParameter (paramId))
                    pp->setValueNotifyingHost (0.0f);
        });
    }
}

void TransportBar::refreshPreset()
{
    const int prog = proc_.getCurrentProgram();
    if (prog >= 0 && prog < presets::count())
        presetBox_.setSelectedId (prog + 1, juce::dontSendNotification);
}

void TransportBar::resized()
{
    auto r = getLocalBounds().reduced (8, 6);

    // right cluster: wet / out knobs
    auto right = r.removeFromRight (140);
    auto outArea = right.removeFromRight (66);
    auto wetArea = right;
    gainSlider_.setBounds (outArea.removeFromTop (outArea.getHeight() - 14).reduced (4));
    gainLabel_.setBounds (outArea);
    wetSlider_.setBounds (wetArea.removeFromTop (wetArea.getHeight() - 14).reduced (4));
    wetLabel_.setBounds (wetArea);

    r.removeFromRight (10);

    // left: actions + toggles in two rows
    auto top = r.removeFromTop (r.getHeight() / 2).reduced (0, 1);
    auto bot = r.reduced (0, 1);

    auto place = [] (juce::Rectangle<int>& row, juce::Component& c, int w)
    {
        c.setBounds (row.removeFromLeft (w).reduced (3, 2));
    };

    place (top, digBtn_, 96);
    place (top, clearBtn_, 110);
    place (top, freezeBtn_, 84);
    place (top, silenceBtn_, 168);

    place (bot, presetBox_, 180);
    place (bot, windowBox_, 78);
    place (bot, quantizeBox_, 84);
    place (bot, syncBtn_, 70);
    place (bot, choirBtn_, 74);
    place (bot, relicBtn_, 100);
}

void TransportBar::paint (juce::Graphics& g)
{
    g.setColour (theme::paperDeep);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
    g.setColour (theme::brass.withAlpha (0.3f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);
}
} // namespace arch
