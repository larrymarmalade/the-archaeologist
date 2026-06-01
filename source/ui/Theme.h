#pragma once

#include <juce_graphics/juce_graphics.h>
#include "../dsp/Artifact.h"

namespace arch::theme
{
// Museum archive / EMS Synthi / aged tape library palette.
inline const juce::Colour paper      { 0xfff3ece0 }; // warm off-white
inline const juce::Colour paperDeep  { 0xffe7dccb };
inline const juce::Colour ink        { 0xff1c1a17 }; // black ink
inline const juce::Colour inkSoft    { 0xff5b5448 };
inline const juce::Colour brass       { 0xffb08d57 }; // aged brass
inline const juce::Colour brassDim    { 0xff8a7048 };
inline const juce::Colour smoke       { 0xff2b2924 }; // smoked glass panel
inline const juce::Colour smokeLight  { 0xff3a382f };
inline const juce::Colour amber       { 0xffd9a441 };
inline const juce::Colour faintBlue   { 0xff6f8a93 };
inline const juce::Colour faintGreen  { 0xff7f8e6b };
inline const juce::Colour glassEdge   { 0x33ffffff };

inline juce::Colour familyColour (Family f)
{
    switch (f)
    {
        case Family::SustainedTone:  return faintBlue;
        case Family::ChordCloud:     return faintGreen;
        case Family::AttackFragment: return amber;
        case Family::RhythmicCell:   return brass;
        case Family::NoiseScrape:    return juce::Colour (0xff9a6b5a);
        case Family::Silence:        return juce::Colour (0xff6a665d);
        default:                     return inkSoft;
    }
}

inline const char* conditionFor (float ruinOrAge)   // 0 intact .. 1 ruined
{
    if (ruinOrAge < 0.2f) return "intact";
    if (ruinOrAge < 0.45f) return "oxidized";
    if (ruinOrAge < 0.7f) return "fractured";
    return "snowed-in";
}
} // namespace arch::theme
