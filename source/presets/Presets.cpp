#include "Presets.h"
#include "../params/Parameters.h"
#include <vector>

namespace arch::presets
{
struct PV { const char* id; float value; };   // value is the PLAIN value
struct Preset { const char* name; std::vector<PV> values; };

// Macro order convenience. Modes: 0 Drift,1 Fracture,2 Nursery,3 Fog,4 Seance.
// Windows: 0 30s,1 2m,2 5m,3 15m.  Quantize: 0 off,1 loose,2 bar,3 phrase.
static const std::vector<Preset>& table()
{
    static const std::vector<Preset> t = {
        { "Empty Museum", {
            { pid::mode, 0 }, { pid::excavationRate, 0.12f }, { pid::memoryDepth, 0.55f },
            { pid::artifactLength, 0.4f }, { pid::recognition, 0.3f }, { pid::forgetting, 0.2f },
            { pid::fidelityDecay, 0.25f }, { pid::ruin, 0.12f }, { pid::continuity, 0.85f },
            { pid::beauty, 0.7f }, { pid::density, 0.15f }, { pid::wetDry, 0.35f },
            { pid::quantize, 0 }, { pid::favorSilence, 1 } } },

        { "Old Tape Under Snow", {
            { pid::mode, 3 }, { pid::excavationRate, 0.3f }, { pid::memoryDepth, 0.7f },
            { pid::artifactLength, 0.6f }, { pid::recognition, 0.35f }, { pid::forgetting, 0.3f },
            { pid::fidelityDecay, 0.8f }, { pid::ruin, 0.7f }, { pid::continuity, 0.8f },
            { pid::beauty, 0.45f }, { pid::density, 0.35f }, { pid::wetDry, 0.6f },
            { pid::quantize, 0 } } },

        { "Broken Children's Piano", {
            { pid::mode, 2 }, { pid::excavationRate, 0.45f }, { pid::memoryDepth, 0.4f },
            { pid::artifactLength, 0.35f }, { pid::recognition, 0.5f }, { pid::forgetting, 0.4f },
            { pid::fidelityDecay, 0.5f }, { pid::ruin, 0.4f }, { pid::continuity, 0.5f },
            { pid::beauty, 0.55f }, { pid::density, 0.45f }, { pid::wetDry, 0.55f },
            { pid::quantize, 1 } } },

        { "Delaware Crossing Fever Dream", {
            { pid::mode, 4 }, { pid::excavationRate, 0.6f }, { pid::memoryDepth, 0.6f },
            { pid::artifactLength, 0.5f }, { pid::recognition, 0.7f }, { pid::forgetting, 0.35f },
            { pid::fidelityDecay, 0.55f }, { pid::ruin, 0.45f }, { pid::continuity, 0.55f },
            { pid::beauty, 0.5f }, { pid::density, 0.7f }, { pid::wetDry, 0.65f },
            { pid::quantize, 1 }, { pid::relicStack, 1 }, { pid::choir, 1 } } },

        { "Glassine Loop Rot", {
            { pid::mode, 1 }, { pid::excavationRate, 0.5f }, { pid::memoryDepth, 0.5f },
            { pid::artifactLength, 0.3f }, { pid::recognition, 0.25f }, { pid::forgetting, 0.5f },
            { pid::fidelityDecay, 0.7f }, { pid::ruin, 0.75f }, { pid::continuity, 0.35f },
            { pid::beauty, 0.35f }, { pid::density, 0.55f }, { pid::wetDry, 0.7f },
            { pid::quantize, 0 } } },

        { "Eno in a Cold Room", {
            { pid::mode, 0 }, { pid::excavationRate, 0.2f }, { pid::memoryDepth, 0.65f },
            { pid::artifactLength, 0.6f }, { pid::recognition, 0.3f }, { pid::forgetting, 0.2f },
            { pid::fidelityDecay, 0.3f }, { pid::ruin, 0.2f }, { pid::continuity, 0.92f },
            { pid::beauty, 0.85f }, { pid::density, 0.25f }, { pid::wetDry, 0.5f },
            { pid::quantize, 0 }, { pid::favorSilence, 1 } } },

        { "Aphex Attic Toy", {
            { pid::mode, 2 }, { pid::excavationRate, 0.55f }, { pid::memoryDepth, 0.35f },
            { pid::artifactLength, 0.3f }, { pid::recognition, 0.55f }, { pid::forgetting, 0.45f },
            { pid::fidelityDecay, 0.45f }, { pid::ruin, 0.5f }, { pid::continuity, 0.4f },
            { pid::beauty, 0.5f }, { pid::density, 0.55f }, { pid::wetDry, 0.6f },
            { pid::quantize, 2 } } },

        { "Autechre Fossil Machine", {
            { pid::mode, 1 }, { pid::excavationRate, 0.7f }, { pid::memoryDepth, 0.45f },
            { pid::artifactLength, 0.25f }, { pid::recognition, 0.2f }, { pid::forgetting, 0.5f },
            { pid::fidelityDecay, 0.6f }, { pid::ruin, 0.6f }, { pid::continuity, 0.2f },
            { pid::beauty, 0.35f }, { pid::density, 0.75f }, { pid::wetDry, 0.7f },
            { pid::quantize, 2 } } },

        { "ECM Afterimage", {
            { pid::mode, 3 }, { pid::excavationRate, 0.28f }, { pid::memoryDepth, 0.6f },
            { pid::artifactLength, 0.55f }, { pid::recognition, 0.4f }, { pid::forgetting, 0.25f },
            { pid::fidelityDecay, 0.35f }, { pid::ruin, 0.25f }, { pid::continuity, 0.88f },
            { pid::beauty, 0.8f }, { pid::density, 0.3f }, { pid::wetDry, 0.5f },
            { pid::quantize, 0 } } },

        { "Ghost of the Take Before", {
            { pid::mode, 4 }, { pid::excavationRate, 0.4f }, { pid::memoryDepth, 0.5f },
            { pid::artifactLength, 0.45f }, { pid::recognition, 0.85f }, { pid::forgetting, 0.3f },
            { pid::fidelityDecay, 0.45f }, { pid::ruin, 0.35f }, { pid::continuity, 0.7f },
            { pid::beauty, 0.65f }, { pid::density, 0.45f }, { pid::wetDry, 0.55f },
            { pid::quantize, 1 } } },
    };
    return t;
}

int count() { return (int) table().size(); }

const char* name (int index)
{
    const auto& t = table();
    if (index < 0 || index >= (int) t.size()) return "";
    return t[(size_t) index].name;
}

void apply (int index, juce::AudioProcessorValueTreeState& apvts)
{
    const auto& t = table();
    if (index < 0 || index >= (int) t.size()) return;

    // Start from a clean default-ish baseline so presets are reproducible.
    static const char* resettable[] = {
        pid::favorSilence, pid::choir, pid::relicStack, pid::freeze
    };
    for (auto* id : resettable)
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (0.0f);

    for (const auto& pv : t[(size_t) index].values)
        if (auto* p = apvts.getParameter (pv.id))
            p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (pv.value));
}
} // namespace arch::presets
