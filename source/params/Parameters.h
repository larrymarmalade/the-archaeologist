#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace arch
{
namespace pid
{
    // Macro / behaviour
    inline constexpr const char* excavationRate = "excavationRate";
    inline constexpr const char* memoryDepth    = "memoryDepth";
    inline constexpr const char* artifactLength = "artifactLength";
    inline constexpr const char* recognition    = "recognition";   // similarity bias
    inline constexpr const char* forgetting     = "forgetting";
    inline constexpr const char* fidelityDecay  = "fidelityDecay";
    inline constexpr const char* ruin           = "ruin";
    inline constexpr const char* continuity     = "continuity";    // continuity<->rupture
    inline constexpr const char* beauty         = "beauty";        // beauty<->damage
    inline constexpr const char* density        = "density";

    // Output
    inline constexpr const char* wetDry     = "wetDry";
    inline constexpr const char* outputGain  = "outputGain";

    // Choices
    inline constexpr const char* mode          = "mode";
    inline constexpr const char* memoryWindow  = "memoryWindow";
    inline constexpr const char* quantize      = "quantize";

    // Toggles
    inline constexpr const char* freeze       = "freezeArchive";
    inline constexpr const char* favorSilence = "favorSilence";
    inline constexpr const char* syncToHost   = "syncToHost";
    inline constexpr const char* choir        = "choirMode";
    inline constexpr const char* relicStack   = "relicStack";

    // Momentary triggers (rising-edge detected)
    inline constexpr const char* digNow      = "digNow";
    inline constexpr const char* clearArchive = "clearArchive";
} // namespace pid

enum class MemoryWindow { S30 = 0, M2, M5, M15, Count };
inline double memoryWindowSeconds (int idx)
{
    switch (idx) { case 0: return 30.0; case 1: return 120.0;
                   case 2: return 300.0; default: return 900.0; }
}

enum class Quantize { Off = 0, Loose, Bar, Phrase, Count };

/** Cached atomic pointers for lock-free reads on audio/background threads. */
struct ParamRefs
{
    std::atomic<float>* excavationRate = nullptr;
    std::atomic<float>* memoryDepth    = nullptr;
    std::atomic<float>* artifactLength = nullptr;
    std::atomic<float>* recognition    = nullptr;
    std::atomic<float>* forgetting     = nullptr;
    std::atomic<float>* fidelityDecay  = nullptr;
    std::atomic<float>* ruin           = nullptr;
    std::atomic<float>* continuity     = nullptr;
    std::atomic<float>* beauty         = nullptr;
    std::atomic<float>* density        = nullptr;
    std::atomic<float>* wetDry         = nullptr;
    std::atomic<float>* outputGain     = nullptr;
    std::atomic<float>* mode           = nullptr;
    std::atomic<float>* memoryWindow   = nullptr;
    std::atomic<float>* quantize       = nullptr;
    std::atomic<float>* freeze         = nullptr;
    std::atomic<float>* favorSilence   = nullptr;
    std::atomic<float>* syncToHost     = nullptr;
    std::atomic<float>* choir          = nullptr;
    std::atomic<float>* relicStack     = nullptr;
    std::atomic<float>* digNow         = nullptr;
    std::atomic<float>* clearArchive   = nullptr;

    void bind (juce::AudioProcessorValueTreeState& s);
};

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
} // namespace arch
