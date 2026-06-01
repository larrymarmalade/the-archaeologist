#include "ExcavationEngine.h"
#include "VoiceEngine.h"
#include <cmath>

namespace arch
{
static float pf (std::atomic<float>* a, float def = 0.0f) { return a ? a->load (std::memory_order_relaxed) : def; }

void ExcavationEngine::prepare (double sampleRate, ParamRefs* params)
{
    sampleRate_ = sampleRate;
    p_ = params;
    reset();
}

void ExcavationEngine::reset()
{
    timer_ = 0.0;
    nextInterval_ = 1.0;
    lastPpq_ = 0.0;
    digRequested_.store (false, std::memory_order_release);
    recentHead_ = 0;
    forgetAccum_ = 0.0;
    for (auto& r : recent_) r = {};
    rng_.setSeed (juce::Time::currentTimeMillis());
}

float ExcavationEngine::similarity (const Artifact& a, const AnalysisEngine::InputDescriptor& in) const
{
    float d = std::abs (a.brightness - in.brightness) * 3.0f
            + std::abs (a.flatness   - in.flatness)   * 2.0f;
    if (a.pitchHz > 0.0f && in.pitchHz > 0.0f)
        d += std::abs (std::log2 (a.pitchHz / in.pitchHz));
    return 1.0f - std::tanh (d);
}

bool ExcavationEngine::recentlyPlayed (uint32_t id, double now, double window) const
{
    for (const auto& r : recent_)
        if (r.id == id && now - r.at < window)
            return true;
    return false;
}

float ExcavationEngine::scoreArtifact (const Artifact& a, const Context& ctx, Mode, float recog) const
{
    const int   winIdx  = (int) pf (p_->memoryWindow);
    const double window = memoryWindowSeconds (winIdx);
    const float ageSec  = (float) (ctx.sessionSeconds - a.capturedAtSec);
    const float ageNorm = juce::jlimit (0.0f, 1.0f, ageSec / (float) window);

    const float md = pf (p_->memoryDepth, 0.5f);               // 0 fresh .. 1 old
    const float ageScore = md * ageNorm + (1.0f - md) * (1.0f - ageNorm);

    const float sim      = similarity (a, ctx.input);
    const float simScore = recog * sim + (1.0f - recog) * (1.0f - sim);

    float silenceBoost = 0.0f;
    if (pf (p_->favorSilence) > 0.5f && ctx.input.rms < 0.01f)
    {
        silenceBoost = 0.30f;
        if (a.family == Family::SustainedTone || a.family == Family::ChordCloud)
            silenceBoost += 0.10f;
    }

    // cluster penalty: discourage near-identical-to-recent material
    float clusterPen = 0.0f;
    for (const auto& r : recent_)
    {
        if (r.at < ctx.sessionSeconds - 12.0) continue;
        const bool closeBright = std::abs (a.brightness - r.brightness) < 0.03f;
        const bool closePitch  = (a.pitchHz > 0 && r.pitch > 0)
                               ? std::abs (a.pitchHz - r.pitch) < 3.0f : closeBright;
        if (closeBright && closePitch) clusterPen += 0.25f;
    }

    return 0.25f * ageScore + 0.22f * simScore + 0.18f * a.rarity
         + 0.25f * a.emotionalWeight + silenceBoost - clusterPen;
}

VoiceSpawn ExcavationEngine::buildSpawn (Artifact& a, const Context& ctx, Mode mode,
                                         const ModeProfile& prof, ArtifactStore& store,
                                         float gainScale)
{
    VoiceSpawn s;
    s.art     = &a;
    s.slab[0] = store.slabChannel (a, 0);
    s.slab[1] = store.slabChannel (a, std::min (1, a.channels - 1));

    const int    winIdx  = (int) pf (p_->memoryWindow);
    const double window  = memoryWindowSeconds (winIdx);
    const float  ageNorm = juce::jlimit (0.0f, 1.0f,
                              (float) (ctx.sessionSeconds - a.capturedAtSec) / (float) window);
    const float  beauty   = pf (p_->beauty, 0.6f);
    const float  cont     = pf (p_->continuity, 0.6f);
    auto rf = [&] { return rng_.nextFloat(); };

    // gain
    const float loudN = juce::jlimit (0.0f, 1.0f, (a.loudnessDb + 48.0f) / 48.0f);
    s.gain = juce::jmap (loudN, 0.25f, 0.9f) * gainScale * 0.8f;

    // speed / pitch
    float speed = std::pow (2.0f, (rf() - 0.5f) * 2.0f * prof.varispeedSpread);
    const float roll = rf();
    if (prof.varispeedSpread > 0.0f)
    {
        if (roll < prof.varispeedSpread * 0.5f)        speed *= 0.5f;
        else if (roll > 1.0f - prof.varispeedSpread * 0.5f) speed *= 2.0f;
    }
    s.rate    = juce::jlimit (0.25f, 4.0f, speed);
    s.reverse = rf() < prof.reverseProb;

    // ruin / archival coloration
    const float ruin = juce::jlimit (0.0f, 1.0f,
          pf (p_->ruin) * prof.ruinScale * 0.7f + pf (p_->fidelityDecay) * ageNorm * 0.6f
        + (1.0f - beauty) * 0.3f);
    s.ruin      = ruin;
    s.bandHighHz = juce::jlimit (1500.0f, (float) (sampleRate_ * 0.45),
                                 juce::jmap (ruin, 18000.0f, 2500.0f));
    s.bandLowHz  = juce::jmap (ruin, 25.0f, 160.0f);

    // envelopes — continuity makes long contextual fades, rupture makes stabs
    s.attackSec  = juce::jmap (cont, 0.004f, prof.fadeSeconds);
    s.releaseSec = prof.fadeSeconds * (0.4f + cont);
    if (prof.ruptureBias > 0.0f && rf() < prof.ruptureBias)
        s.attackSec = 0.003f;                          // violent interruption

    // playback morphologies
    const bool sustainFamily = (a.family == Family::SustainedTone || a.family == Family::ChordCloud);
    s.granular      = rf() < prof.granularProb;
    s.microLoop     = ! s.granular && rf() < prof.microLoopProb;
    s.freezeSustain = ! s.granular && sustainFamily
                      && (mode == Mode::Drift || mode == Mode::Fog) && rf() < 0.3f;
    if (s.freezeSustain) s.microLoop = false;
    s.loopLenSec    = juce::jmap (rf(), 0.08f, 0.5f);
    s.loopJitter    = 0.05f + 0.3f * (mode == Mode::Fracture ? 1.0f : prof.emergenceJitter);
    s.grainHz       = juce::jmap (rf(), 16.0f, 55.0f);
    s.grainSpraySec = juce::jmap (rf(), 0.01f, 0.12f);
    s.grainLenSec   = juce::jmap (rf(), 0.06f, 0.18f);
    s.lifetimeSec   = (s.microLoop || s.freezeSustain || s.granular)
                    ? juce::jmap (cont, 1.5f, 6.0f) * (0.7f + rf()) : 0.0f;

    // motion
    s.wowAmt          = juce::jlimit (0.0f, 1.0f, ruin * 0.4f + 0.1f);
    s.wowRateHz       = juce::jmap (rf(), 0.3f, 2.0f);
    s.driftSemis      = prof.pitchDriftAmt * 12.0f * (0.5f + rf());
    s.driftRateHz     = juce::jmap (rf(), 0.02f, 0.12f);
    s.panWanderAmt    = prof.stereoWander * (0.5f + 0.5f * rf());
    s.panWanderRateHz = juce::jmap (rf(), 0.03f, 0.15f);
    s.pan             = (rf() * 2.0f - 1.0f) * prof.stereoWander;

    // silence / pre-echo
    s.silenceProb = juce::jlimit (0.0f, 0.6f,
                       (pf (p_->favorSilence) > 0.5f ? prof.silenceAffinity * 0.5f
                                                     : prof.silenceAffinity * 0.2f)
                       + (1.0f - cont) * 0.2f);
    if ((mode == Mode::Fog || mode == Mode::Seance || mode == Mode::Drift) && rf() < 0.15f)
    {
        s.preEchoSec  = juce::jmap (rf(), 0.05f, 0.25f);
        s.preEchoGain = juce::jmap (rf(), 0.12f, 0.30f);
    }

    s.seed = (uint32_t) rng_.nextInt() | 1u;
    return s;
}

void ExcavationEngine::emit (VoiceSpawn s, ArtifactStore& store, SpawnQueue& queue, const Context& ctx)
{
    (void) store;
    if (s.art == nullptr) return;
    s.art->refCount.fetch_add (1, std::memory_order_acq_rel);   // pin slab until played
    if (! queue.push (s))
    {
        s.art->refCount.fetch_sub (1, std::memory_order_acq_rel);
        return;
    }
    s.art->playCount.fetch_add (1, std::memory_order_relaxed);
    s.art->lastPlayedAtSec.store (ctx.sessionSeconds, std::memory_order_relaxed);

    recent_[(size_t) recentHead_] = { s.art->id, ctx.sessionSeconds,
                                      s.art->brightness, s.art->pitchHz };
    recentHead_ = (recentHead_ + 1) % kRecent;
}

void ExcavationEngine::tick (const Context& ctx, ArtifactStore& store, SpawnQueue& queue)
{
    if (p_ == nullptr) return;

    const Mode        mode = (Mode) juce::jlimit (0, (int) Mode::Count - 1, (int) pf (p_->mode));
    const ModeProfile prof = profileFor (mode);
    const float rate    = pf (p_->excavationRate, 0.35f);
    const float density = pf (p_->density, 0.4f);
    const float recog   = juce::jlimit (0.0f, 1.0f, pf (p_->recognition, 0.4f) + prof.similarityBias);

    // background forgetting: occasionally retire the weakest artifact
    const float forgetting = pf (p_->forgetting, 0.3f);
    forgetAccum_ += forgetting * ctx.dt;
    if (forgetAccum_ > 1.0 && store.aliveCount() > 8)
    {
        forgetAccum_ = 0.0;
        Artifact* weakest = nullptr; float wlo = 1.0e9f;
        store.forEachAlive ([&] (Artifact& a)
        {
            const float w = a.emotionalWeight - 0.2f * a.rarity;
            if (w < wlo && a.refCount.load (std::memory_order_acquire) == 0) { wlo = w; weakest = &a; }
        });
        if (weakest != nullptr)
            store.retire (*weakest);
    }

    const int target = juce::jlimit (1, VoiceEngine::kMaxVoices,
                          (int) std::round ((0.3f + density) * prof.baseDensity * 10.0f));

    timer_ += ctx.dt;
    double base = juce::jmap ((double) (rate * rate), 0.0, 1.0, 16.0, 0.3);
    if (nextInterval_ <= 0.0) nextInterval_ = base;

    // quantisation gate
    bool quantizeGate = true;
    const auto q = (Quantize) (int) pf (p_->quantize);
    const bool synced = pf (p_->syncToHost) > 0.5f && ctx.transportPlaying;
    if (synced && q != Quantize::Off)
    {
        const double unit = (q == Quantize::Phrase) ? 16.0 : (q == Quantize::Bar) ? 4.0 : 1.0;
        quantizeGate = std::floor (ctx.ppqPosition / unit) != std::floor (lastPpq_ / unit);
    }
    lastPpq_ = ctx.ppqPosition;

    // Consume the Dig Now request atomically. If set, this tick always proceeds
    // to a selection attempt (and the request is cleared exactly once).
    const bool dig       = digRequested_.exchange (false, std::memory_order_acq_rel);
    const bool timeReady = timer_ >= nextInterval_;
    const bool budget    = ctx.activeVoices < target;

    if (! (dig || (timeReady && quantizeGate && budget)))
        return;

    // ── select best artifact ────────────────────────────────────────────────
    const double suppress = juce::jmap ((double) rate, 0.0, 1.0, 9.0, 1.2);
    Artifact* best = nullptr; float bestScore = -1.0e9f;
    store.forEachAlive ([&] (Artifact& a)
    {
        if (recentlyPlayed (a.id, ctx.sessionSeconds, suppress)) return;
        float sc = scoreArtifact (a, ctx, mode, recog);
        sc += (rng_.nextFloat() - 0.5f) * prof.emergenceJitter * 0.5f;   // probabilistic
        if (sc > bestScore) { bestScore = sc; best = &a; }
    });

    timer_ = 0.0;
    nextInterval_ = juce::jmax (0.1, base * (1.0 + (rng_.nextDouble() - 0.5) * 2.0 * prof.emergenceJitter));
    // (digRequested_ was already consumed above via exchange.)

    if (best == nullptr) return;

    // ── emit (single / choir / relic stack) ─────────────────────────────────
    const bool choir = pf (p_->choir) > 0.5f || rng_.nextFloat() < prof.choirProb;
    const bool relic = pf (p_->relicStack) > 0.5f || rng_.nextFloat() < prof.relicStackProb;

    emit (buildSpawn (*best, ctx, mode, prof, store, 1.0f), store, queue, ctx);

    if (choir)
    {
        const int copies = 2 + rng_.nextInt (2);
        for (int c = 0; c < copies && ctx.activeVoices + c + 1 < target + 3; ++c)
        {
            VoiceSpawn cp = buildSpawn (*best, ctx, mode, prof, store, 0.55f);
            cp.rate *= std::pow (2.0f, (rng_.nextFloat() - 0.5f) * 0.06f);   // ±~30 cents
            cp.pan   = (rng_.nextFloat() * 2.0f - 1.0f) * 0.8f;
            cp.attackSec *= 1.0f + rng_.nextFloat();
            emit (cp, store, queue, ctx);
        }
    }

    if (relic)
    {
        // layer a few of the OLDEST salient artifacts into a composite ghost
        int placed = 0;
        store.forEachAlive ([&] (Artifact& a)
        {
            if (placed >= 3 || &a == best) return;
            if (a.emotionalWeight < 0.4f) return;
            if (recentlyPlayed (a.id, ctx.sessionSeconds, suppress)) return;
            if (ctx.sessionSeconds - a.capturedAtSec < 8.0) return;        // must be old
            VoiceSpawn r = buildSpawn (a, ctx, mode, prof, store, 0.4f);
            r.ruin       = juce::jlimit (0.0f, 1.0f, r.ruin + 0.25f);
            r.attackSec  = juce::jmax (r.attackSec, prof.fadeSeconds);
            r.releaseSec = prof.fadeSeconds * 1.5f;
            emit (r, store, queue, ctx);
            ++placed;
        });
    }
}
} // namespace arch
