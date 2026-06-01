#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../params/BehaviorModes.h"

namespace arch
{
/** Five large mode buttons, each with a slightly different visual personality. */
class ModeSelector : public juce::Component
{
public:
    explicit ModeSelector (juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh();    // sync from parameter

private:
    juce::AudioProcessorValueTreeState& apvts_;
    juce::RangedAudioParameter* modeParam_ = nullptr;
    std::array<juce::TextButton, (size_t) Mode::Count> buttons_;
};
} // namespace arch
