#include "ArtifactVoice.h"
#include "Interpolation.h"
#include <cmath>

namespace arch
{
void ArtifactVoice::prepare (double sampleRate, int)
{
    sampleRate_ = sampleRate;
    reset();
}

void ArtifactVoice::finish() noexcept
{
    if (art_ != nullptr)
    {
        art_->refCount.fetch_sub (1, std::memory_order_acq_rel);   // release exactly once
        art_ = nullptr;
    }
    slabBase_[0] = nullptr;
    slabBase_[1] = nullptr;
    active_ = false;
    stage_  = Stage::Done;
    env_    = 0.0;
    for (auto& g : grains_) g.on = false;
}

void ArtifactVoice::reset()
{
    finish();
    for (auto& f : hp_) f.reset();
    for (auto& f : lp_) f.reset();
    ager_.reset();
}

void ArtifactVoice::start (const VoiceSpawn& s)
{
    // assumes caller already incremented s.art->refCount.
    // finish() releases any artifact this slot still holds (stolen/idle voice).
    finish();

    s_   = s;
    art_ = s.art;
    slabBase_[0] = s.slab[0];
    slabBase_[1] = s.slab[1] != nullptr ? s.slab[1] : s.slab[0];
    if (art_ == nullptr) { active_ = false; return; }

    const double len = art_->length;
    dir_  = s_.reverse ? -1.0 : 1.0;
    pos_  = s_.reverse ? len - 1.0 : 0.0;

    rng_.setSeed ((juce::int64) s_.seed | 1);

    // loop region (micro-loop / freeze sustain)
    if (s_.microLoop || s_.freezeSustain)
    {
        const double loopLen = juce::jlimit (8.0, len,
                                  (s_.freezeSustain ? 0.08 : s_.loopLenSec) * sampleRate_);
        loopStart_ = juce::jlimit (0.0, len - loopLen, rng_.nextDouble() * (len - loopLen));
        loopEnd_   = loopStart_ + loopLen;
        pos_       = s_.reverse ? loopEnd_ - 1.0 : loopStart_;
    }
    else { loopStart_ = 0.0; loopEnd_ = len; }

    stage_      = Stage::Attack;
    env_        = 0.0;
    attackInc_  = 1.0 / std::max (1.0, s_.attackSec  * sampleRate_);
    releaseInc_ = 1.0 / std::max (1.0, s_.releaseSec * sampleRate_);
    ageFrames_  = 0.0;
    lifetimeFrames_ = s_.lifetimeSec > 0.0f ? s_.lifetimeSec * sampleRate_ : 0.0;

    wow_.setRate   (s_.wowRateHz,        sampleRate_);
    drift_.setRate (s_.driftRateHz,      sampleRate_);
    panLfo_.setRate(s_.panWanderRateHz,  sampleRate_);
    gate_.setRate  (0.35,                sampleRate_);
    wow_.setPhase (rng_.nextDouble());
    drift_.setPhase (rng_.nextDouble());
    panLfo_.setPhase (rng_.nextDouble());

    ager_.set (s_.ruin);
    for (auto& f : hp_) { f.reset(); f.setCutoff (s_.bandLowHz  / (float) sampleRate_); }
    for (auto& f : lp_) { f.reset(); f.setCutoff (s_.bandHighHz / (float) sampleRate_); }

    if (s_.granular)
    {
        grainPeriod_ = std::max (1.0, sampleRate_ / std::max (1.0f, s_.grainHz));
        grainPhase_  = 0.0;
        for (auto& g : grains_) g.on = false;
    }
    active_ = true;
}

void ArtifactVoice::release()
{
    if (stage_ != Stage::Done && stage_ != Stage::Release)
        stage_ = Stage::Release;
}

void ArtifactVoice::steal()
{
    releaseInc_ = 1.0 / std::max (1.0, 0.01 * sampleRate_);   // 10 ms fade
    stage_ = Stage::Release;
}

float ArtifactVoice::readSlab (double posFrames, int ch) const noexcept
{
    const int len = art_->length;
    const int c   = (ch < art_->channels) ? ch : 0;
    const float* slab = slabBase_[static_cast<size_t> (c)];
    if (slab == nullptr || len <= 0) return 0.0f;

    const int i1 = juce::jlimit (0, len - 1, (int) std::floor (posFrames));
    const float t = (float) (posFrames - std::floor (posFrames));
    const int i0 = juce::jlimit (0, len - 1, i1 - 1);
    const int i2 = juce::jlimit (0, len - 1, i1 + 1);
    const int i3 = juce::jlimit (0, len - 1, i1 + 2);
    return hermite (slab[i0], slab[i1], slab[i2], slab[i3], t);
}

float ArtifactVoice::envTick() noexcept
{
    switch (stage_)
    {
        case Stage::Attack:
            env_ += attackInc_;
            if (env_ >= 1.0) { env_ = 1.0; stage_ = Stage::Sustain; }
            break;
        case Stage::Sustain:                    break;
        case Stage::Release:
            env_ -= releaseInc_;
            if (env_ <= 0.0) { env_ = 0.0; stage_ = Stage::Done; }
            break;
        case Stage::Done:    env_ = 0.0;        break;
    }
    // cosine-shaped for click-free fades
    return 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * (float) env_);
}

void ArtifactVoice::render (float* outL, float* outR, int n) noexcept
{
    if (! active_ || art_ == nullptr) return;
    const double len = art_->length;

    for (int i = 0; i < n; ++i)
    {
        if (stage_ == Stage::Done) { finish(); break; }   // release ref on completion

        const float e = envTick();

        // lifetime / end-of-fragment triggers release
        ageFrames_ += 1.0;
        if (lifetimeFrames_ > 0.0 && ageFrames_ >= lifetimeFrames_) release();

        // modulated playback rate
        double mod = 1.0;
        if (s_.wowAmt   > 0.0f) mod *= 1.0 + (double) s_.wowAmt * 0.03 * wow_.sine();
        if (s_.driftSemis > 0.0f)
            mod *= std::pow (2.0, ((double) s_.driftSemis / 12.0) * drift_.sine());
        const double rate = (double) s_.rate * mod;

        float l = 0.0f, r = 0.0f;

        if (s_.granular)
        {
            // advance / spawn grains around a slowly moving centre (pos_)
            grainPhase_ += 1.0;
            if (grainPhase_ >= grainPeriod_)
            {
                grainPhase_ -= grainPeriod_;
                for (auto& g : grains_)
                    if (! g.on)
                    {
                        const double spray = (rng_.nextDouble() * 2.0 - 1.0)
                                           * s_.grainSpraySec * sampleRate_;
                        g.pos  = juce::jlimit (0.0, len - 1.0, pos_ + spray);
                        g.rate = rate * std::pow (2.0, (rng_.nextDouble() - 0.5) * 0.08);
                        g.len  = std::max (16, (int) (s_.grainLenSec * sampleRate_));
                        g.age  = 0;
                        g.on   = true;
                        break;
                    }
            }
            for (auto& g : grains_)
            {
                if (! g.on) continue;
                const float gw = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi
                                       * (float) g.age / (float) g.len);   // Hann
                l += readSlab (g.pos, 0) * gw;
                r += readSlab (g.pos, 1) * gw;
                g.pos += g.rate * dir_;
                if (++g.age >= g.len || g.pos < 0.0 || g.pos >= len) g.on = false;
            }
            l *= 0.5f; r *= 0.5f;
            pos_ += dir_ * rate * 0.25;                 // centre drifts slowly
            if (pos_ >= len || pos_ < 0.0) release();
        }
        else
        {
            l = readSlab (pos_, 0);
            r = readSlab (pos_, 1);

            if (s_.preEchoGain > 0.0f)                  // ghost pre-echo: read ahead
            {
                const double lead = pos_ + (double) s_.preEchoSec * sampleRate_ * dir_;
                l += readSlab (lead, 0) * s_.preEchoGain;
                r += readSlab (lead, 1) * s_.preEchoGain;
            }

            pos_ += dir_ * rate;

            if (s_.microLoop || s_.freezeSustain)
            {
                if (pos_ >= loopEnd_)   pos_ = loopStart_ + (pos_ - loopEnd_);
                if (pos_ <  loopStart_) pos_ = loopEnd_  - (loopStart_ - pos_);
                if (s_.loopJitter > 0.0f && rng_.nextFloat() < 0.001f)  // unstable boundary
                {
                    const double jit = (rng_.nextDouble() * 2.0 - 1.0)
                                     * s_.loopJitter * (loopEnd_ - loopStart_);
                    loopEnd_ = juce::jlimit (loopStart_ + 8.0, len, loopEnd_ + jit);
                }
            }
            else if (pos_ >= len || pos_ < 0.0)
            {
                pos_ = juce::jlimit (0.0, len - 1.0, pos_);
                release();
            }
        }

        // archival aging + band-limiting (per channel state)
        ager_.process (l, r);
        l = lp_[0].lp (hp_[0].hp (l));
        r = lp_[1].lp (hp_[1].hp (r));

        // probabilistic silence gating
        float gateGain = 1.0f;
        if (s_.silenceProb > 0.0f)
            gateGain = (gate_.sine() < (s_.silenceProb * 2.0f - 1.0f)) ? 0.0f : 1.0f;

        // pan (+ wander)
        float pan = s_.pan;
        if (s_.panWanderAmt > 0.0f) pan += s_.panWanderAmt * panLfo_.sine();
        float gl, gr; equalPowerPan (juce::jlimit (-1.0f, 1.0f, pan), gl, gr);

        const float amp = e * s_.gain * gateGain;
        outL[i] += l * gl * amp;
        outR[i] += r * gr * amp;
    }

    if (stage_ == Stage::Done) finish();   // completed on the last sample of the block
}
} // namespace arch
