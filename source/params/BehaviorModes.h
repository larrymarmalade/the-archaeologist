#pragma once

#include <cstdint>

namespace arch
{
enum class Mode : uint8_t
{
    Drift = 0,   // Eno: sparse, gentle, long fades, low interruption
    Fracture,    // Autechre: angular, glitchy, rhythmically unstable
    Nursery,     // Aphex: tonal, uncanny repetition, warped music-box
    Fog,         // ECM: restrained, wide stereo, soft attacks, air
    Seance,      // most autonomous: "answers" the performer
    Count
};

inline const char* modeName (Mode m) noexcept
{
    switch (m)
    {
        case Mode::Drift:    return "Drift";
        case Mode::Fracture: return "Fracture";
        case Mode::Nursery:  return "Nursery";
        case Mode::Fog:      return "Fog";
        case Mode::Seance:   return "Séance";
        default:             return "?";
    }
}

/**
    Personality of a behavior mode. These bias the excavation scorer and the
    transformation choices so each mode feels musically distinct without
    needing a separate code path. Values are gentle multipliers / targets in
    a roughly 0..1 range unless noted.
*/
struct ModeProfile
{
    float baseDensity       = 0.5f;  // scales how many voices co-exist
    float emergenceJitter   = 0.5f;  // randomness in timing (0 = metronomic)
    float ruptureBias       = 0.0f;  // prefer violent interruption over fades
    float similarityBias    = 0.0f;  // +call-and-response, -contrast seeking
    float ageBias           = 0.0f;  // + prefer old, - prefer fresh
    float fadeSeconds       = 2.0f;  // default attack/release of voices
    float pitchDriftAmt     = 0.05f; // semitone drift range
    float reverseProb       = 0.1f;
    float granularProb      = 0.1f;
    float microLoopProb     = 0.1f;
    float stereoWander       = 0.3f;
    float ruinScale          = 1.0f; // multiplies the Ruin macro
    float silenceAffinity    = 0.3f; // tendency to answer into silence
    float choirProb          = 0.0f;
    float relicStackProb     = 0.0f;
    float varispeedSpread    = 0.0f; // +/- speed randomisation
};

inline ModeProfile profileFor (Mode m) noexcept
{
    ModeProfile p;
    switch (m)
    {
        case Mode::Drift:
            p = { 0.30f, 0.7f, -0.4f, -0.1f, 0.3f, 6.0f, 0.04f,
                  0.05f, 0.05f, 0.05f, 0.45f, 0.6f, 0.55f, 0.25f, 0.0f, 0.02f };
            break;
        case Mode::Fracture:
            p = { 0.7f, 0.9f, 0.8f, -0.3f, -0.2f, 0.25f, 0.12f,
                  0.4f, 0.35f, 0.55f, 0.5f, 1.4f, 0.1f, 0.0f, 0.2f, 0.18f };
            break;
        case Mode::Nursery:
            p = { 0.5f, 0.4f, 0.1f, 0.5f, 0.0f, 1.2f, 0.18f,
                  0.2f, 0.15f, 0.45f, 0.25f, 1.1f, 0.2f, 0.35f, 0.1f, 0.08f };
            break;
        case Mode::Fog:
            p = { 0.35f, 0.6f, -0.5f, 0.0f, 0.2f, 5.0f, 0.03f,
                  0.1f, 0.25f, 0.1f, 0.8f, 0.7f, 0.5f, 0.4f, 0.25f, 0.03f };
            break;
        case Mode::Seance:
            p = { 0.5f, 0.55f, 0.0f, 0.7f, 0.1f, 2.5f, 0.07f,
                  0.15f, 0.2f, 0.25f, 0.5f, 0.9f, 0.7f, 0.2f, 0.3f, 0.05f };
            break;
        default: break;
    }
    return p;
}
} // namespace arch
