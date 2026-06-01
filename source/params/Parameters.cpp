#include "Parameters.h"
#include "BehaviorModes.h"

namespace arch
{
using APVTS = juce::AudioProcessorValueTreeState;
using Range = juce::NormalisableRange<float>;

static auto pct() { return Range (0.0f, 1.0f, 0.0001f); }

void ParamRefs::bind (APVTS& s)
{
    excavationRate = s.getRawParameterValue (pid::excavationRate);
    memoryDepth    = s.getRawParameterValue (pid::memoryDepth);
    artifactLength = s.getRawParameterValue (pid::artifactLength);
    recognition    = s.getRawParameterValue (pid::recognition);
    forgetting     = s.getRawParameterValue (pid::forgetting);
    fidelityDecay  = s.getRawParameterValue (pid::fidelityDecay);
    ruin           = s.getRawParameterValue (pid::ruin);
    continuity     = s.getRawParameterValue (pid::continuity);
    beauty         = s.getRawParameterValue (pid::beauty);
    density        = s.getRawParameterValue (pid::density);
    wetDry         = s.getRawParameterValue (pid::wetDry);
    outputGain     = s.getRawParameterValue (pid::outputGain);
    mode           = s.getRawParameterValue (pid::mode);
    memoryWindow   = s.getRawParameterValue (pid::memoryWindow);
    quantize       = s.getRawParameterValue (pid::quantize);
    freeze         = s.getRawParameterValue (pid::freeze);
    favorSilence   = s.getRawParameterValue (pid::favorSilence);
    syncToHost     = s.getRawParameterValue (pid::syncToHost);
    choir          = s.getRawParameterValue (pid::choir);
    relicStack     = s.getRawParameterValue (pid::relicStack);
    digNow         = s.getRawParameterValue (pid::digNow);
    clearArchive   = s.getRawParameterValue (pid::clearArchive);
}

APVTS::ParameterLayout createParameterLayout()
{
    using namespace juce;
    APVTS::ParameterLayout layout;

    auto addFloat = [&] (const char* id, const String& name, Range r, float def,
                         const String& suffix = {})
    {
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, r, def,
            AudioParameterFloatAttributes().withLabel (suffix)));
    };
    auto addBool = [&] (const char* id, const String& name, bool def)
    {
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };

    addFloat (pid::excavationRate, "Excavation Rate", pct(), 0.35f);
    addFloat (pid::memoryDepth,    "Memory Depth",    pct(), 0.5f);
    addFloat (pid::artifactLength, "Artifact Length", pct(), 0.45f);
    addFloat (pid::recognition,    "Recognition",     pct(), 0.4f);
    addFloat (pid::forgetting,     "Forgetting",      pct(), 0.3f);
    addFloat (pid::fidelityDecay,  "Fidelity Decay",  pct(), 0.4f);
    addFloat (pid::ruin,           "Ruin",            pct(), 0.25f);
    addFloat (pid::continuity,     "Continuity",      pct(), 0.6f);  // 1=continuous,0=rupture
    addFloat (pid::beauty,         "Beauty",          pct(), 0.6f);  // 1=beauty,0=damage
    addFloat (pid::density,        "Density",         pct(), 0.4f);

    addFloat (pid::wetDry,     "Wet/Dry",     pct(), 0.5f);
    addFloat (pid::outputGain, "Output Gain", Range (-36.0f, 12.0f, 0.01f), 0.0f, "dB");

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { pid::mode, 1 }, "Mode",
        StringArray { "Drift", "Fracture", "Nursery", "Fog", "Seance" }, 0));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { pid::memoryWindow, 1 }, "Memory Window",
        StringArray { "30 s", "2 min", "5 min", "15 min" }, 1));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { pid::quantize, 1 }, "Quantize Emergence",
        StringArray { "Off", "Loose", "Bar", "Phrase" }, 1));

    addBool (pid::freeze,       "Freeze Archive",  false);
    addBool (pid::favorSilence, "Favor Silence",   false);
    addBool (pid::syncToHost,   "Sync to Host",    true);
    addBool (pid::choir,        "Choir Mode",      false);
    addBool (pid::relicStack,   "Relic Stack",     false);
    addBool (pid::digNow,       "Dig Now",         false);
    addBool (pid::clearArchive, "Clear Archive",   false);

    return layout;
}
} // namespace arch
