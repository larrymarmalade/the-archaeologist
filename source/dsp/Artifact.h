#pragma once

#include <atomic>
#include <cstdint>
#include <array>

namespace arch
{
/** Coarse musical taxonomy assigned by the analysis engine. */
enum class Family : uint8_t
{
    AttackFragment = 0, // short, percussive, high transient density
    SustainedTone,      // long, stable pitch, low flux
    ChordCloud,         // polyphonic / dense harmonic content
    NoiseScrape,        // inharmonic, high flatness, noisy
    Silence,            // near-silent region
    RhythmicCell,       // multiple regularly spaced transients
    Unstable,           // could not be classified confidently
    Count
};

inline const char* familyName (Family f) noexcept
{
    switch (f)
    {
        case Family::AttackFragment: return "attack";
        case Family::SustainedTone:  return "sustained tone";
        case Family::ChordCloud:     return "chord/cloud";
        case Family::NoiseScrape:    return "noise/scrape";
        case Family::Silence:        return "silence";
        case Family::RhythmicCell:   return "rhythmic cell";
        case Family::Unstable:       return "unstable";
        default:                     return "unknown";
    }
}

/**
    An immutable record of a captured musical gesture.

    Once committed by the analysis thread, every field except `playState`,
    `lastPlayedAtSec`, `playCount` and `refCount` is read-only. The audio thread
    reads metadata and the slab samples; it only mutates the atomic play
    bookkeeping. `refCount` guards the slab region: storage is not recycled
    while any voice still references it.
*/
struct Artifact
{
    // ── identity & storage ────────────────────────────────────────────────
    uint32_t id            = 0;        // monotonically increasing unique id
    int      slabOffset    = 0;        // sample offset into the shared slab (per-channel)
    int      length        = 0;        // length in frames
    int      channels      = 1;
    int      capacity      = 0;        // reserved frames in slab (>= length)

    // ── time ──────────────────────────────────────────────────────────────
    int64_t  capturedAtAbs = 0;        // absolute capture sample position
    double   capturedAtSec  = 0.0;     // wall-ish session seconds at capture
    double   durationSec    = 0.0;

    // ── analysis features (normalised 0..1 unless noted) ───────────────────
    Family   family         = Family::Unstable;
    float    loudnessRms     = 0.0f;   // linear RMS
    float    loudnessDb      = -100.0f;
    float    brightness      = 0.0f;   // normalised spectral centroid
    float    pitchHz         = 0.0f;   // 0 == no confident pitch
    float    pitchConfidence = 0.0f;
    float    flatness        = 0.0f;   // spectral flatness (noisiness)
    float    harmonicity     = 0.0f;   // 1 - flatness-ish
    int      transientCount  = 0;
    float    transientDensity= 0.0f;   // transients per second, normalised
    float    rarity          = 0.5f;   // how unlike its neighbours (filled later)
    float    emotionalWeight = 0.5f;   // salience score used by selection

    // ── mutable play bookkeeping (audio thread) ────────────────────────────
    std::atomic<int>     refCount       { 0 };   // active voices reading the slab
    std::atomic<double>  lastPlayedAtSec{ -1.0e9 };
    std::atomic<int>     playCount      { 0 };
    std::atomic<bool>    alive          { false };// false == free slot

    Artifact() = default;

    // Atomics make this non-copyable; the pool holds them in place.
    Artifact (const Artifact&) = delete;
    Artifact& operator= (const Artifact&) = delete;
};

/** Plain-old snapshot of an artifact for lock-free hand-off to the UI. */
struct ArtifactView
{
    uint32_t id          = 0;
    float    ageSec      = 0.0f;
    Family   family      = Family::Unstable;
    float    loudnessDb  = -100.0f;
    float    brightness  = 0.0f;
    float    pitchHz      = 0.0f;
    float    rarity       = 0.5f;
    float    emotionalWeight = 0.5f;
    float    stability    = 0.0f;    // pitch confidence proxy
    float    condition    = 0.0f;    // 0 intact .. 1 ruined (age + noisiness)
    bool     excavating   = false;   // currently being played by a voice
    float    excitation   = 0.0f;    // 0..1 glow used by the archive map
};
} // namespace arch
