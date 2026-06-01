#pragma once

#include "../dsp/Artifact.h"
#include "../params/BehaviorModes.h"
#include <atomic>
#include <array>
#include <cstdint>

namespace arch
{
inline constexpr int kMaxViews = 256;   // max glyphs handed to the UI

/** Immutable-per-frame picture of the instrument's state for the editor. */
struct Snapshot
{
    std::array<ArtifactView, kMaxViews> views {};
    int      viewCount      = 0;
    int      aliveTotal     = 0;
    int      activeVoices   = 0;
    float    inputLevel     = 0.0f;   // 0..1 (smoothed)
    float    outputLevel    = 0.0f;
    float    archiveFill    = 0.0f;   // 0..1 slab occupancy
    Mode     mode           = Mode::Drift;
    bool     frozen         = false;
    double   sessionSeconds = 0.0;
    uint32_t lastExcavatedId = 0;
    ArtifactView focus      {};       // the most recently excavated artifact
    bool     hasFocus       = false;
};

/**
    Lock-free triple buffer for one-way SPSC hand-off of a POD-ish snapshot.
    The producer (background thread) fills writeRef() then publish()es; the
    consumer (UI thread) calls read() to obtain the most recent published copy.
    Writer and reader provably never touch the same slot. No locks, no alloc.
*/
template <typename T>
class TripleBuffer
{
public:
    T& writeRef() noexcept { return buf_[(size_t) writeIdx_]; }

    void publish() noexcept
    {
        // hand our freshly-written buffer to the shared slot, flagged dirty,
        // and take whatever was there to write into next.
        const int handed = writeIdx_ | kDirty;
        writeIdx_ = shared_.exchange (handed, std::memory_order_acq_rel) & kIndex;
    }

    /** Returns the most recently published value. If nothing new has been
        published since the last read, returns the same value again (so the
        sequence of returned values is always the published order). */
    const T& read() noexcept
    {
        if (shared_.load (std::memory_order_acquire) & kDirty)
            readIdx_ = shared_.exchange (readIdx_, std::memory_order_acq_rel) & kIndex;
        return buf_[(size_t) readIdx_];
    }

    bool hasNew() const noexcept { return (shared_.load (std::memory_order_acquire) & kDirty) != 0; }

private:
    static constexpr int kIndex = 0x3;   // low 2 bits hold a buffer index 0..2
    static constexpr int kDirty = 0x4;   // bit 2 == "fresh data waiting"

    T buf_[3] {};
    int writeIdx_ = 0;
    int readIdx_  = 1;
    std::atomic<int> shared_ { 2 };       // {0,1,2} always a permutation of slots
};
} // namespace arch
