#pragma once

#include "ArtifactStore.h"
#include "ArtifactVoice.h"
#include "AnalysisEngine.h"
#include "../params/Parameters.h"
#include "../params/BehaviorModes.h"
#include "../state/LockFreeQueue.h"
#include <juce_core/juce_core.h>
#include <atomic>

namespace arch
{
using SpawnQueue = LockFreeQueue<VoiceSpawn, 256>;

/**
    The selection + scheduling brain. Background thread. Decides WHEN a memory
    surfaces, WHICH artifact, and HOW it is transformed — balancing age,
    similarity/contrast, rarity, salience, silence and anti-repetition, shaped
    by the current behaviour mode and macro parameters.
*/
class ExcavationEngine
{
public:
    struct Context
    {
        double sessionSeconds = 0.0;
        double dt             = 0.0;     // seconds since last tick
        int    activeVoices   = 0;
        double bpm            = 120.0;
        double ppqPosition    = 0.0;
        bool   transportPlaying = false;
        bool   frozen         = false;
        AnalysisEngine::InputDescriptor input;
    };

    void prepare (double sampleRate, ParamRefs* params);
    void reset();

    /** Run one scheduling step; may push 0..N orders into `queue`. */
    void tick (const Context& ctx, ArtifactStore& store, SpawnQueue& queue);

    /** Force one excavation on the next tick (Dig Now). Called from the audio
        and message threads; consumed on the worker thread. */
    void requestDig() noexcept { digRequested_.store (true, std::memory_order_release); }

private:
    float scoreArtifact (const Artifact& a, const Context& ctx, Mode mode, float recog) const;
    float similarity (const Artifact& a, const AnalysisEngine::InputDescriptor& in) const;
    bool  recentlyPlayed (uint32_t id, double now, double window) const;
    VoiceSpawn buildSpawn (Artifact& a, const Context& ctx, Mode mode,
                           const ModeProfile& prof, ArtifactStore& store, float gainScale);
    void  emit (VoiceSpawn s, ArtifactStore& store, SpawnQueue& queue, const Context& ctx);

    double sampleRate_ = 48000.0;
    ParamRefs* p_ = nullptr;
    juce::Random rng_;

    double timer_       = 0.0;
    double nextInterval_= 1.0;
    double lastPpq_     = 0.0;
    std::atomic<bool> digRequested_ { false };   // cross-thread Dig Now request

    // anti-repetition
    static constexpr int kRecent = 32;
    struct Played { uint32_t id = 0; double at = -1.0e9; float brightness = 0, pitch = 0; };
    std::array<Played, kRecent> recent_ {};
    int recentHead_ = 0;

    double forgetAccum_ = 0.0;
};
} // namespace arch
