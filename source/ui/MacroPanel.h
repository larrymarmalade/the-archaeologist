#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <memory>

namespace arch
{
/** The six large performance knobs plus a secondary row of finer controls.
    Every visible control is bound to an automatable parameter. */
class MacroPanel : public juce::Component
{
public:
    explicit MacroPanel (juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
        bool big = true;
    };
    void addKnob (juce::AudioProcessorValueTreeState&, const char* paramId,
                  const juce::String& name, bool big);

    std::vector<std::unique_ptr<Knob>> knobs_;
    int bigCount_ = 0;
};
} // namespace arch
