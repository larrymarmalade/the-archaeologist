#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../state/Snapshot.h"
#include <unordered_map>

namespace arch
{
/** The central, slowly-evolving field of captured artifacts. Glyph position
    encodes age (x) and brightness (y); size encodes loudness; colour encodes
    family; an excavated glyph glows and drifts upward like a surfacing memory.
    Beautiful at idle, legible in motion, decoupled from the audio thread. */
class ArchiveMapComponent : public juce::Component
{
public:
    ArchiveMapComponent();
    void paint (juce::Graphics&) override;

    /** Called by the editor's timer with the latest published snapshot. */
    void update (const Snapshot& snap, float dt);

private:
    struct Glyph
    {
        float x = 0.5f, y = 0.5f, tx = 0.5f, ty = 0.5f;
        float glow = 0.0f, size = 0.5f, seen = 0.0f;
        Family family = Family::Unstable;
        bool present = false;
    };

    std::unordered_map<uint32_t, Glyph> glyphs_;
    float clock_ = 0.0f;
    int   aliveTotal_ = 0, activeVoices_ = 0;
    bool  frozen_ = false;
    Mode  mode_ = Mode::Drift;
    float archiveFill_ = 0.0f;
};
} // namespace arch
