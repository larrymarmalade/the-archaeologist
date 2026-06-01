#include "VoiceEngine.h"

namespace arch
{
void VoiceEngine::prepare (double sampleRate, int blockSize)
{
    sampleRate_ = sampleRate;
    for (auto& v : voices_) v.prepare (sampleRate, blockSize);
    diffuser_.prepare (sampleRate);
    diffuser_.reset();
}

void VoiceEngine::reset()
{
    for (auto& v : voices_) v.reset();
    diffuser_.reset();
}

int VoiceEngine::findVoiceToUse()
{
    // 1) a free voice
    for (int i = 0; i < kMaxVoices; ++i)
        if (! voices_[(size_t) i].isActive())
            return i;

    // 2) steal only a near-silent voice to stay click-free; otherwise drop the
    //    order so density is respected and no audible cut occurs.
    int best = -1; float bestScore = 1.0e9f;
    for (int i = 0; i < kMaxVoices; ++i)
    {
        const float lvl = voices_[(size_t) i].levelEstimate();
        if (lvl < bestScore) { bestScore = lvl; best = i; }
    }
    if (best >= 0 && bestScore < 0.04f) return best;   // quiet enough to reuse cleanly
    return -1;
}

void VoiceEngine::spawn (const VoiceSpawn& s)
{
    if (s.art == nullptr) return;
    const int idx = findVoiceToUse();
    if (idx < 0)
    {
        // no voice available — refund the refcount the engine took for this order
        s.art->refCount.fetch_sub (1, std::memory_order_acq_rel);
        return;
    }
    voices_[(size_t) idx].start (s);
}

void VoiceEngine::render (float* outL, float* outR, int n) noexcept
{
    for (auto& v : voices_)
        if (v.isActive())
            v.render (outL, outR, n);

    for (int i = 0; i < n; ++i)
        diffuser_.process (outL[i], outR[i]);
}

int VoiceEngine::activeVoices() const noexcept
{
    int c = 0;
    for (const auto& v : voices_) if (v.isActive()) ++c;
    return c;
}

void VoiceEngine::releaseAll() noexcept
{
    for (auto& v : voices_) if (v.isActive()) v.release();
}
} // namespace arch
