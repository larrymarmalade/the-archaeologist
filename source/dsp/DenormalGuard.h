#pragma once

#include <juce_audio_basics/juce_audio_basics.h>   // juce::ScopedNoDenormals

namespace arch
{
/** RAII wrapper that enables flush-to-zero / denormals-are-zero for the
    lifetime of the scope. Construct once at the top of processBlock. */
struct DenormalGuard
{
    DenormalGuard() noexcept = default;
    juce::ScopedNoDenormals scoped;
};

/** Tiny DC-ish offset to break denormal feedback loops in recursive filters. */
inline float antiDenormal (float x) noexcept
{
    constexpr float tiny = 1.0e-20f;
    x += tiny;
    x -= tiny;
    return x;
}
} // namespace arch
