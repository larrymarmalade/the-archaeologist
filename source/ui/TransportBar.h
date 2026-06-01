#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace arch
{
class ArchaeologistProcessor;

/** Performance buttons + global controls + preset menu. */
class TransportBar : public juce::Component
{
public:
    TransportBar (ArchaeologistProcessor& proc, juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
    void paint (juce::Graphics&) override;
    void refreshPreset();

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void pulse (const char* paramId);

    ArchaeologistProcessor& proc_;
    juce::AudioProcessorValueTreeState& apvts_;

    juce::TextButton   digBtn_ { "Dig Now" }, clearBtn_ { "Clear Archive" };
    juce::ToggleButton freezeBtn_, silenceBtn_, syncBtn_, choirBtn_, relicBtn_;
    juce::ComboBox     windowBox_, quantizeBox_, presetBox_;
    juce::Slider       wetSlider_, gainSlider_;
    juce::Label        wetLabel_, gainLabel_;

    std::unique_ptr<BA> freezeAtt_, silenceAtt_, syncAtt_, choirAtt_, relicAtt_;
    std::unique_ptr<CA> windowAtt_, quantizeAtt_;
    std::unique_ptr<SA> wetAtt_, gainAtt_;
};
} // namespace arch
