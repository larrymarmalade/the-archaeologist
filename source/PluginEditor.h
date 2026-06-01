#pragma once

#include "PluginProcessor.h"
#include "ui/ArchaeologistLookAndFeel.h"
#include "ui/ArchiveMapComponent.h"
#include "ui/ModeSelector.h"
#include "ui/MacroPanel.h"
#include "ui/DetailPanel.h"
#include "ui/TransportBar.h"

namespace arch
{
class ArchaeologistEditor : public juce::AudioProcessorEditor,
                            private juce::Timer
{
public:
    explicit ArchaeologistEditor (ArchaeologistProcessor&);
    ~ArchaeologistEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ArchaeologistProcessor& proc_;
    ArchaeologistLookAndFeel lnf_;
    juce::TooltipWindow tooltips_ { this, 600 };

    ArchiveMapComponent archiveMap_;
    ModeSelector        modes_;
    MacroPanel          macros_;
    DetailPanel         detail_;
    TransportBar        transport_;

    double lastTickMs_ = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArchaeologistEditor)
};
} // namespace arch
