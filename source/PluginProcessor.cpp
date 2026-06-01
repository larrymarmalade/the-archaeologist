#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/DenormalGuard.h"
#include "presets/Presets.h"
#include <cmath>

namespace arch
{
juce::AudioProcessor::BusesProperties ArchaeologistProcessor::makeBuses()
{
    return BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true);
}

ArchaeologistProcessor::ArchaeologistProcessor()
    : juce::AudioProcessor (makeBuses()),
      apvts_ (*this, nullptr, "PARAMS", createParameterLayout())
{
    params_.bind (apvts_);
    worker_ = std::make_unique<Worker> (*this);
}

ArchaeologistProcessor::~ArchaeologistProcessor()
{
    if (worker_ != nullptr)
    {
        worker_->signalThreadShouldExit();
        worker_->stopThread (2000);
    }
}

// ── memory sizing ───────────────────────────────────────────────────────────
void ArchaeologistProcessor::allocateMemory (int windowIdx)
{
    const double winSec = memoryWindowSeconds (windowIdx);
    const int capacity  = juce::jmax (1, (int) std::ceil (winSec * sampleRate_));

    ring_.prepare (channels_, capacity);
    store_.prepare (channels_, capacity, 1024);
    analysis_.prepare (sampleRate_, channels_, store_);
    analysis_.reset (0);
    excavation_.prepare (sampleRate_, &params_);
    allocatedWindowIdx_.store (windowIdx, std::memory_order_release);
}

void ArchaeologistProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxBlock_   = samplesPerBlock;
    channels_   = juce::jmax (1, getTotalNumInputChannels());

    // stop the worker while we (re)allocate the shared buffers it reads
    if (worker_->isThreadRunning())
    {
        worker_->signalThreadShouldExit();
        worker_->stopThread (2000);
    }

    const int windowIdx = (int) params_.memoryWindow->load();
    pendingWindowIdx_.store (windowIdx);
    allocateMemory (windowIdx);

    voices_.prepare (sampleRate, samplesPerBlock);
    wet_.setSize (2, samplesPerBlock, false, false, true);

    wetSmooth_.reset (sampleRate, 0.02);
    gainSmooth_.reset (sampleRate, 0.02);
    wetSmooth_.setCurrentAndTargetValue (params_.wetDry->load());
    gainSmooth_.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (params_.outputGain->load()));

    worker_->startThread (juce::Thread::Priority::low);
}

void ArchaeologistProcessor::releaseResources()
{
    voices_.reset();
}

bool ArchaeologistProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    if (in != out && in != juce::AudioChannelSet::disabled())
        return false;
    return true;
}

void ArchaeologistProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    DenormalGuard denormals;

    const int numSamples = buffer.getNumSamples();
    const int numIn  = juce::jmax (1, getTotalNumInputChannels());
    const int numOut = getTotalNumOutputChannels();

    // ── transport ─────────────────────────────────────────────────────────
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm())          transportBpm_.store (*bpm);
            if (auto ppq = pos->getPpqPosition())  transportPpq_.store (*ppq);
            transportPlaying_.store (pos->getIsPlaying());
        }

    // ── parameter edges & toggles ───────────────────────────────────────────
    const bool dig    = params_.digNow->load()       > 0.5f;
    const bool clr    = params_.clearArchive->load()  > 0.5f;
    const bool freeze = params_.freeze->load()        > 0.5f;
    if (dig && ! lastDig_)   excavation_.requestDig();
    if (clr && ! lastClear_) { clearRequested_.store (true); ring_.clear(); voices_.releaseAll(); }
    lastDig_ = dig; lastClear_ = clr; lastFreeze_ = freeze;
    frozen_.store (freeze);

    // ── MIDI: a played note asks the archive to answer ───────────────────────
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            lastMidiNoteHz_.store ((float) m.getMidiNoteInHertz (m.getNoteNumber()));
            excavation_.requestDig();
        }
    }

    // ── capture input into the ring (pre-FX) unless frozen ───────────────────
    if (! freeze && numIn > 0)
    {
        const float* srcs[2];
        for (int ch = 0; ch < juce::jmin (numIn, 2); ++ch)
            srcs[ch] = buffer.getReadPointer (ch);
        ring_.write (srcs, juce::jmin (numIn, 2), numSamples);
    }

    // input metering
    float inSum = 0.0f;
    for (int ch = 0; ch < numIn; ++ch)
    {
        const float* d = buffer.getReadPointer (ch);
        for (int i = 0; i < numSamples; ++i) inSum += d[i] * d[i];
    }
    inputLevel_.store (std::sqrt (inSum / (float) juce::jmax (1, numSamples * numIn)));

    // ── consume excavation orders ────────────────────────────────────────────
    VoiceSpawn order;
    int guard = 0;
    while (spawnQueue_.pop (order) && guard++ < 64)
        voices_.spawn (order);

    // ── render wet ───────────────────────────────────────────────────────────
    wet_.clear();
    {
        const int mode = (int) params_.mode->load();
        const float beauty = params_.beauty->load();
        float smear = (mode == (int) Mode::Fog) ? 0.55f
                    : (mode == (int) Mode::Seance) ? 0.4f
                    : (mode == (int) Mode::Drift) ? 0.28f : 0.12f;
        smear *= juce::jlimit (0.2f, 1.0f, 0.4f + beauty * 0.6f);
        voices_.setSmear (smear);
    }
    voices_.render (wet_.getWritePointer (0), wet_.getWritePointer (1), numSamples);
    activeVoices_.store (voices_.activeVoices());

    // ── wet/dry mix + output gain ────────────────────────────────────────────
    wetSmooth_.setTargetValue (params_.wetDry->load());
    gainSmooth_.setTargetValue (juce::Decibels::decibelsToGain (params_.outputGain->load()));

    float outSum = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float w = wetSmooth_.getNextValue();
        const float g = gainSmooth_.getNextValue();
        for (int ch = 0; ch < numOut; ++ch)
        {
            const int wetCh = juce::jmin (ch, 1);
            float* out = buffer.getWritePointer (ch);
            const float dry = out[i];
            const float wetS = wet_.getReadPointer (wetCh)[i];
            const float y = (dry * (1.0f - w) + wetS * w) * g;
            out[i] = y;
            outSum += y * y;
        }
    }
    outputLevel_.store (std::sqrt (outSum / (float) juce::jmax (1, numSamples * juce::jmax (1, numOut))));

    // NOTE: Memory Window changes are NOT scheduled here. Posting an async
    // message (triggerAsyncUpdate) from the audio callback is not a hard
    // real-time primitive. The background worker polls the parameter and posts
    // the reallocation request instead (see Worker::run()).
}

// ── background worker ─────────────────────────────────────────────────────────
void ArchaeologistProcessor::Worker::run()
{
    lastTimeMs = juce::Time::getMillisecondCounterHiRes();
    while (! threadShouldExit())
    {
        wait (12);
        if (threadShouldExit()) break;

        auto& o = owner;

        if (o.clearRequested_.exchange (false))
        {
            o.store_.clearAll();
            o.analysis_.reset (o.ring_.totalWritten());
        }

        const bool frozen = o.frozen_.load();
        const float artLenN = o.params_.artifactLength->load();
        const double maxArtSec = juce::jmap (artLenN, 0.25f, 8.0f);

        if (! frozen)
            o.analysis_.process (o.ring_, o.store_, maxArtSec);
        o.store_.collect();

        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        const double dt = juce::jlimit (0.0, 0.5, (nowMs - lastTimeMs) * 0.001);
        lastTimeMs = nowMs;

        ExcavationEngine::Context ctx;
        ctx.sessionSeconds   = (double) o.ring_.totalWritten() / o.sampleRate_;
        ctx.dt               = dt;
        ctx.activeVoices     = o.activeVoices_.load();
        ctx.bpm              = o.transportBpm_.load();
        ctx.ppqPosition      = o.transportPpq_.load();
        ctx.transportPlaying = o.transportPlaying_.load();
        ctx.frozen           = frozen;
        ctx.input            = o.analysis_.liveInput();
        o.excavation_.tick (ctx, o.store_, o.spawnQueue_);

        o.buildSnapshot();

        // Memory Window change detection lives on this (background) thread, not
        // the audio thread. triggerAsyncUpdate() is safe to call from here (it
        // is not the audio callback); handleAsyncUpdate() does the reallocation
        // on the message thread. Coalesced, so repeated calls are harmless.
        const int wantWin = (int) o.params_.memoryWindow->load();
        if (wantWin != o.allocatedWindowIdx_.load())
        {
            o.pendingWindowIdx_.store (wantWin);
            o.triggerAsyncUpdate();
        }
    }
}

void ArchaeologistProcessor::buildSnapshot()
{
    Snapshot& s = snapshot_.writeRef();
    const double now = (double) ring_.totalWritten() / sampleRate_;

    s.aliveTotal   = store_.aliveCount();
    s.activeVoices = activeVoices_.load();
    s.inputLevel   = inputLevel_.load();
    s.outputLevel  = outputLevel_.load();
    s.mode         = (Mode) juce::jlimit (0, (int) Mode::Count - 1, (int) params_.mode->load());
    s.frozen       = frozen_.load();
    s.sessionSeconds = now;
    s.archiveFill  = juce::jlimit (0.0f, 1.0f,
                       (float) store_.aliveCount() / (float) juce::jmax (1, store_.maxArtifacts() / 4));

    int n = 0;
    bool haveFocus = false;
    ArtifactView focus;
    float bestFocusT = -1.0e9f;

    store_.forEachAlive ([&] (Artifact& a)
    {
        if (n >= kMaxViews) return;
        ArtifactView v;
        v.id          = a.id;
        v.ageSec      = (float) (now - a.capturedAtSec);
        v.family      = a.family;
        v.loudnessDb  = a.loudnessDb;
        v.brightness  = a.brightness;
        v.pitchHz     = a.pitchHz;
        v.rarity      = a.rarity;
        v.emotionalWeight = a.emotionalWeight;
        v.stability   = a.pitchConfidence;
        v.condition   = juce::jlimit (0.0f, 1.0f,
                          0.5f * a.flatness + 0.5f * juce::jlimit (0.0f, 1.0f, v.ageSec / 120.0f));
        const bool playing = a.refCount.load (std::memory_order_acquire) > 0;
        v.excavating  = playing;
        const double lp = a.lastPlayedAtSec.load (std::memory_order_relaxed);
        v.excitation  = juce::jlimit (0.0f, 1.0f, playing ? 1.0f
                          : (float) std::exp (-(now - lp) / 2.5));
        s.views[(size_t) n++] = v;

        if (lp > bestFocusT) { bestFocusT = (float) lp; focus = v; haveFocus = lp > -1.0e8; }
    });
    s.viewCount = n;
    s.focus = focus;
    s.hasFocus = haveFocus;

    snapshot_.publish();
}

void ArchaeologistProcessor::handleAsyncUpdate()
{
    const int idx = pendingWindowIdx_.load();
    if (idx == allocatedWindowIdx_.load()) return;

    // Pause BOTH the audio thread and the background worker before touching the
    // shared ring/store/analysis buffers, then bring everything back up.
    //
    // suspendProcessing(true) is a real barrier, not just a flag: JUCE's
    // AudioProcessor::suspendProcessing acquires `callbackLock`, and every
    // plugin wrapper (VST3/AU/Standalone) holds that same getCallbackLock()
    // around the isSuspended() check + processBlock() call. So once
    // suspendProcessing(true) returns here, no processBlock is in flight and
    // none can start until we suspendProcessing(false). (Verified against JUCE
    // 8.0.13: juce_AudioProcessor.cpp suspendProcessing; the VST3 wrapper's
    // ScopedLock(getCallbackLock()) guarding isSuspended()/processBlock.)
    //
    // suspendProcessing does NOT stop the worker, so we explicitly stop it
    // before reallocating; nothing reallocates until stopThread() has returned.
    suspendProcessing (true);
    const bool wasRunning = worker_->isThreadRunning();
    if (wasRunning) { worker_->signalThreadShouldExit(); worker_->stopThread (2000); }

    voices_.reset();
    allocateMemory (idx);

    if (wasRunning) worker_->startThread (juce::Thread::Priority::low);
    suspendProcessing (false);
}

// ── programs / presets ────────────────────────────────────────────────────────
int ArchaeologistProcessor::getNumPrograms() { return presets::count(); }

void ArchaeologistProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= presets::count()) return;
    currentProgram_ = index;
    presets::apply (index, apvts_);
}

const juce::String ArchaeologistProcessor::getProgramName (int index)
{
    return presets::name (index);
}

// ── state ──────────────────────────────────────────────────────────────────────
void ArchaeologistProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto state = apvts_.copyState(); state.isValid())
    {
        juce::MemoryOutputStream mos (dest, false);
        state.writeToStream (mos);
    }
}

void ArchaeologistProcessor::setStateInformation (const void* data, int size)
{
    auto tree = juce::ValueTree::readFromData (data, (size_t) size);
    if (tree.isValid())
        apvts_.replaceState (tree);

    // Reconcile discrete (bool/choice) parameters after restore.
    // AudioParameterBool/Choice keep an *unsnapped* getValue(), and APVTS skips
    // re-pushing a value whose snapped form is unchanged — which can leave
    // getValue() holding a stale fractional value (e.g. 0.26 for a bool). We
    // snap every parameter to its nearest legal value so the host-visible state
    // exactly matches what was persisted. No-op for continuous parameters.
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            const float snapped = rp->convertTo0to1 (rp->convertFrom0to1 (rp->getValue()));
            if (std::abs (snapped - rp->getValue()) > 1.0e-6f)
                rp->setValueNotifyingHost (snapped);
        }
}

juce::AudioProcessorEditor* ArchaeologistProcessor::createEditor()
{
    return new ArchaeologistEditor (*this);
}
} // namespace arch

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new arch::ArchaeologistProcessor();
}
