#pragma once

#include "Artifact.h"
#include "Transformations.h"

namespace arch
{
/** Fully-specified excavation order produced by the ExcavationEngine and
    handed to the audio thread via a lock-free queue. Trivially copyable. */
struct VoiceSpawn
{
    Artifact* art           = nullptr;
    const float* slab[2]    = { nullptr, nullptr };  // per-channel base into the store slab
    float  rate             = 1.0f;     // playback speed (pitch+time)
    bool   reverse          = false;
    float  gain             = 0.7f;     // linear
    float  pan              = 0.0f;     // -1..1
    float  attackSec        = 0.5f;
    float  releaseSec       = 1.5f;
    float  lifetimeSec      = 0.0f;     // 0 == play once to end
    bool   microLoop        = false;
    float  loopLenSec       = 0.4f;
    float  loopJitter       = 0.1f;
    bool   freezeSustain    = false;
    bool   granular         = false;
    float  grainHz          = 30.0f;
    float  grainSpraySec    = 0.05f;
    float  grainLenSec      = 0.12f;
    float  ruin             = 0.0f;     // 0..1 aging
    float  bandLowHz        = 20.0f;    // archival HP
    float  bandHighHz       = 18000.0f; // archival LP
    float  wowAmt           = 0.0f;     // 0..1
    float  wowRateHz        = 0.7f;
    float  driftSemis       = 0.0f;     // slow pitch drift range
    float  driftRateHz      = 0.05f;
    float  panWanderAmt     = 0.0f;
    float  panWanderRateHz  = 0.08f;
    float  smear            = 0.0f;     // 0..1 diffusion
    float  silenceProb      = 0.0f;     // probabilistic gating
    float  preEchoSec       = 0.0f;     // 0 == no pre-echo
    float  preEchoGain      = 0.0f;
    uint32_t seed           = 1;
};

/** A single polyphonic playback voice with built-in transformations. */
class ArtifactVoice
{
public:
    void prepare (double sampleRate, int blockSize);
    void reset();

    bool isActive() const noexcept { return active_; }
    uint32_t artifactId() const noexcept { return art_ ? art_->id : 0; }
    float levelEstimate() const noexcept { return active_ ? (float) env_ * s_.gain : 0.0f; }
    bool isReleasing() const noexcept { return stage_ == Stage::Release; }

    void start (const VoiceSpawn& s);
    void release();                              // begin fade-out
    void steal();                                // fast fade for voice stealing

    /** Render additively into stereo out. */
    void render (float* outL, float* outR, int n) noexcept;

private:
    float readSlab (double posFrames, int ch) const noexcept;
    float envTick() noexcept;
    /** Release the artifact ref (exactly once) and put the voice fully idle.
        Idempotent: safe to call when no artifact is held. RT-safe / noexcept. */
    void finish() noexcept;

    double sampleRate_ = 48000.0;

    Artifact* art_ = nullptr;
    const float* slabBase_[2] = { nullptr, nullptr };
    VoiceSpawn s_;
    bool active_ = false;

    // playback position (in artifact-local frames)
    double pos_      = 0.0;
    double dir_      = 1.0;          // +1 / -1 for reverse
    double loopStart_ = 0.0, loopEnd_ = 0.0;

    // envelope
    enum class Stage { Attack, Sustain, Release, Done };
    Stage stage_ = Stage::Done;
    double env_  = 0.0;
    double attackInc_ = 0.0, releaseInc_ = 0.0;
    double ageFrames_ = 0.0, lifetimeFrames_ = 0.0;

    // modulators / processors
    LFO   wow_, drift_, panLfo_, gate_;
    std::array<OnePole, 2> hp_, lp_;     // per-channel band-limiting
    Ager  ager_;

    // granular grains
    struct Grain { double pos = 0.0; double rate = 1.0; int age = 0; int len = 0; bool on = false; };
    static constexpr int kGrains = 12;
    std::array<Grain, kGrains> grains_ {};
    double grainPeriod_ = 0.0, grainPhase_ = 0.0;

    juce::Random rng_;
};
} // namespace arch
