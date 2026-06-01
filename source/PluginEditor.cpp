#include "PluginEditor.h"
#include "ui/Theme.h"

namespace arch
{
ArchaeologistEditor::ArchaeologistEditor (ArchaeologistProcessor& p)
    : juce::AudioProcessorEditor (p), proc_ (p),
      modes_ (p.apvts()),
      macros_ (p.apvts()),
      transport_ (p, p.apvts())
{
    setLookAndFeel (&lnf_);

    addAndMakeVisible (archiveMap_);
    addAndMakeVisible (modes_);
    addAndMakeVisible (macros_);
    addAndMakeVisible (detail_);
    addAndMakeVisible (transport_);

    setResizable (true, true);
    setResizeLimits (820, 560, 1600, 1100);
    setSize (960, 640);

    lastTickMs_ = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (30);
}

ArchaeologistEditor::~ArchaeologistEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ArchaeologistEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float dt = juce::jlimit (0.0f, 0.2f, (float) ((now - lastTickMs_) * 0.001));
    lastTickMs_ = now;

    auto& tb = proc_.snapshot();
    if (tb.hasNew())
    {
        const Snapshot& s = tb.read();
        archiveMap_.update (s, dt);
        detail_.update (s.focus, s.hasFocus);
        archiveMap_.repaint();
    }
    else
    {
        archiveMap_.repaint();   // keep the field gently alive at idle
    }
    modes_.refresh();
}

void ArchaeologistEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::paper);

    // title
    auto top = getLocalBounds().removeFromTop (44).reduced (16, 6);
    g.setColour (theme::ink);
    g.setFont (juce::Font (juce::FontOptions (24.0f)).withExtraKerningFactor (0.16f));
    g.drawText ("THE ARCHAEOLOGIST", top, juce::Justification::centredLeft, true);
    g.setColour (theme::inkSoft);
    g.setFont (juce::Font (juce::FontOptions (12.0f)).withExtraKerningFactor (0.1f));
    g.drawText (juce::String::fromUTF8 ("a generative resampler \xc2\xb7 the session haunts itself"),
                top, juce::Justification::centredRight, true);
}

void ArchaeologistEditor::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (44);                 // title
    r.reduce (12, 4);

    modes_.setBounds (r.removeFromTop (48));
    r.removeFromTop (8);

    transport_.setBounds (r.removeFromBottom (84));
    r.removeFromBottom (8);
    macros_.setBounds (r.removeFromBottom (176));
    r.removeFromBottom (8);

    // central: archive map + detail panel
    detail_.setBounds (r.removeFromRight (256));
    r.removeFromRight (8);
    archiveMap_.setBounds (r);
}
} // namespace arch
