#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../state/Snapshot.h"

namespace arch
{
/** Poetic-but-useful readout of the most recently excavated artifact. */
class DetailPanel : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void update (const ArtifactView& v, bool hasFocus);

private:
    ArtifactView v_ {};
    bool has_ = false;
};
} // namespace arch
