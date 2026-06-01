#include "ArtifactStore.h"
#include <algorithm>

namespace arch
{
void ArtifactStore::prepare (int channels, int slabFrames, int maxArtifacts)
{
    channels_     = std::max (1, channels);
    slabFrames_   = std::max (1, slabFrames);
    maxArtifacts_ = std::max (1, maxArtifacts);

    pool_       = std::make_unique<Artifact[]> (static_cast<size_t> (maxArtifacts_));
    slab_.assign (static_cast<size_t> (channels_) * slabFrames_, 0.0f);
    retireFlag_.assign (static_cast<size_t> (maxArtifacts_), 0);

    slabHead_   = 0;
    aliveCount_ = 0;
    nextId_     = 1;
}

void ArtifactStore::reset()
{
    for (int i = 0; i < maxArtifacts_; ++i)
    {
        pool_[i].alive.store (false, std::memory_order_release);
        pool_[i].refCount.store (0, std::memory_order_release);
        pool_[i].length = 0;
        pool_[i].capacity = 0;
        retireFlag_[static_cast<size_t> (i)] = 0;
    }
    std::fill (slab_.begin(), slab_.end(), 0.0f);
    slabHead_   = 0;
    aliveCount_ = 0;
}

bool ArtifactStore::spaceIsFree (int offset, int need) const
{
    const int end = offset + need;
    for (int i = 0; i < maxArtifacts_; ++i)
    {
        const Artifact& a = pool_[i];
        const bool occupied = a.alive.load (std::memory_order_acquire)
                           || a.refCount.load (std::memory_order_acquire) > 0;
        if (! occupied || a.capacity <= 0)
            continue;
        const int aEnd = a.slabOffset + a.capacity;
        if (offset < aEnd && a.slabOffset < end)        // overlap
            return false;
    }
    return true;
}

bool ArtifactStore::freeSpaceFor (int offset, int need)
{
    const int end = offset + need;

    // First pass: if any overlapping region is still pinned by a voice, bail.
    for (int i = 0; i < maxArtifacts_; ++i)
    {
        Artifact& a = pool_[i];
        const bool occupied = a.alive.load (std::memory_order_acquire)
                           || a.refCount.load (std::memory_order_acquire) > 0;
        if (! occupied || a.capacity <= 0)
            continue;
        const int aEnd = a.slabOffset + a.capacity;
        if (offset < aEnd && a.slabOffset < end)
            if (a.refCount.load (std::memory_order_acquire) > 0)
                return false;                            // pinned — retry later
    }

    // Second pass: retire the (unpinned) overlapping artifacts to make room.
    for (int i = 0; i < maxArtifacts_; ++i)
    {
        Artifact& a = pool_[i];
        if (! a.alive.load (std::memory_order_acquire) || a.capacity <= 0)
            continue;
        const int aEnd = a.slabOffset + a.capacity;
        if (offset < aEnd && a.slabOffset < end)
        {
            a.alive.store (false, std::memory_order_release);
            retireFlag_[static_cast<size_t> (i)] = 0;
            --aliveCount_;
        }
    }
    return true;
}

Artifact* ArtifactStore::allocate (int length, int channels)
{
    if (length <= 0 || length > slabFrames_)
        return nullptr;

    int offset = slabHead_;
    if (offset + length > slabFrames_)
        offset = 0;                                      // wrap, discard tail gap

    if (! freeSpaceFor (offset, length))
        return nullptr;

    // Find a free pool slot (retired AND no longer referenced).
    int slot = -1;
    for (int i = 0; i < maxArtifacts_; ++i)
    {
        if (! pool_[i].alive.load (std::memory_order_acquire)
            && pool_[i].refCount.load (std::memory_order_acquire) == 0)
        {
            slot = i;
            break;
        }
    }
    if (slot < 0)
        return nullptr;

    Artifact& a = pool_[slot];
    a.id         = nextId_++;
    a.slabOffset = offset;
    a.length     = length;
    a.capacity   = length;
    a.channels   = std::min (channels, channels_);
    a.playCount.store (0, std::memory_order_relaxed);
    a.lastPlayedAtSec.store (-1.0e9, std::memory_order_relaxed);
    retireFlag_[static_cast<size_t> (slot)] = 0;

    slabHead_ = offset + length;
    return &a;
}

void ArtifactStore::publish (Artifact& a)
{
    a.alive.store (true, std::memory_order_release);
    ++aliveCount_;
}

void ArtifactStore::retire (Artifact& a)
{
    if (! a.alive.load (std::memory_order_acquire))
        return;
    const auto idx = static_cast<size_t> (&a - pool_.get());
    if (idx >= static_cast<size_t> (maxArtifacts_))
        return;
    if (a.refCount.load (std::memory_order_acquire) == 0)
    {
        a.alive.store (false, std::memory_order_release);
        retireFlag_[idx] = 0;
        --aliveCount_;
    }
    else
    {
        retireFlag_[idx] = 1;       // retire when last voice releases
    }
}

void ArtifactStore::clearAll()
{
    for (int i = 0; i < maxArtifacts_; ++i)
        if (pool_[i].alive.load (std::memory_order_acquire))
            retireFlag_[static_cast<size_t> (i)] = 1;
    collect();
}

void ArtifactStore::collect()
{
    for (int i = 0; i < maxArtifacts_; ++i)
    {
        if (retireFlag_[static_cast<size_t> (i)] == 0)
            continue;
        Artifact& a = pool_[i];
        if (a.refCount.load (std::memory_order_acquire) == 0)
        {
            if (a.alive.exchange (false, std::memory_order_acq_rel))
                --aliveCount_;
            retireFlag_[static_cast<size_t> (i)] = 0;
        }
    }
}

void ArtifactStore::forEachAlive (const std::function<void (Artifact&)>& fn)
{
    for (int i = 0; i < maxArtifacts_; ++i)
        if (pool_[i].alive.load (std::memory_order_acquire))
            fn (pool_[i]);
}
} // namespace arch
