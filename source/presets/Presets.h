#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace arch::presets
{
int          count();
const char*  name (int index);
void         apply (int index, juce::AudioProcessorValueTreeState& apvts);
} // namespace arch::presets
