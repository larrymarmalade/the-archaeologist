#pragma once

#include "Artifact.h"
#include <vector>
#include <memory>
#include <functional>

namespace arch
{
/**
    Owns the fixed artifact pool and the contiguous sample "slab".

    Ownership / threading:
      - All structural mutation (allocate / publish / retire / recycle) happens
        on the single BACKGROUND thread.
      - The AUDIO thread only reads published artifacts and the slab samples,
        and mutates the per-artifact atomic play bookkeeping (refCount etc).
      - The slab is a circular arena: oldest artifacts are forgotten first.
        A region is never recycled while refCount > 0, so a playing voice can
        never read reclaimed memory.

    No allocation occurs after prepare(): the pool and slab are pre-sized.
*/
class ArtifactStore
{
public:
    void prepare (int channels, int slabFrames, int maxArtifacts);
    void reset();                                   // background thread

    // ── background-thread API ──────────────────────────────────────────────
    /** Reserve a contiguous slab region + a free pool slot.
        Returns nullptr if no slot is free or the space needed is still pinned
        by a playing voice (caller should retry next cycle). */
    Artifact* allocate (int length, int channels);

    /** Writable slab pointer for filling samples between allocate() and publish(). */
    float* slabChannel (Artifact& a, int ch) noexcept
    {
        return slab_.data() + static_cast<size_t> (ch) * slabFrames_ + a.slabOffset;
    }
    const float* slabChannel (const Artifact& a, int ch) const noexcept
    {
        return slab_.data() + static_cast<size_t> (ch) * slabFrames_ + a.slabOffset;
    }

    /** Make an allocated artifact visible to selection and the UI. */
    void publish (Artifact& a);

    /** Retire a single artifact (background thread). Deferred if still pinned. */
    void retire (Artifact& a);

    /** Mark every artifact for retirement (Clear Archive). Pinned ones are
        retired later when their last voice releases. */
    void clearAll();

    int   aliveCount() const noexcept { return aliveCount_; }
    int   maxArtifacts() const noexcept { return maxArtifacts_; }
    int   slabFrames()  const noexcept { return slabFrames_; }
    int   channels()    const noexcept { return channels_; }

    /** Iterate currently-alive artifacts (background thread). */
    void forEachAlive (const std::function<void (Artifact&)>& fn);

    /** Reclaim pool slots whose audio is no longer pinned and were flagged
        for retirement. Call once per background cycle. */
    void collect();

    Artifact* pool() noexcept { return pool_.get(); }

private:
    bool spaceIsFree (int offset, int need) const;     // no live/pinned overlap
    bool freeSpaceFor (int offset, int need);          // retire overlapping; false if pinned

    std::unique_ptr<Artifact[]> pool_;
    std::vector<float> slab_;
    std::vector<uint8_t> retireFlag_;                  // per-slot: pending retire
    int channels_     = 1;
    int slabFrames_   = 0;
    int maxArtifacts_ = 0;
    int slabHead_     = 0;                              // circular arena cursor
    int aliveCount_   = 0;
    uint32_t nextId_  = 1;
};
} // namespace arch
