#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "params/Parameters.h"
#include "dsp/RingBuffer.h"
#include "dsp/ArtifactStore.h"
#include "dsp/AnalysisEngine.h"
#include "dsp/ExcavationEngine.h"
#include "dsp/VoiceEngine.h"
#include "state/Snapshot.h"

namespace arch
{
class ArchaeologistProcessor : public juce::AudioProcessor,
                               private juce::AsyncUpdater
{
public:
    ArchaeologistProcessor();
    ~ArchaeologistProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "The Archaeologist"; }
    bool acceptsMidi()  const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram_; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ── accessors for the editor ────────────────────────────────────────────
    juce::AudioProcessorValueTreeState& apvts() { return apvts_; }
    TripleBuffer<Snapshot>& snapshot() { return snapshot_; }
    void triggerDigNow() { excavation_.requestDig(); }
    void requestClear()  { clearRequested_.store (true, std::memory_order_release); }

private:
    // background analysis + excavation worker
    class Worker : public juce::Thread
    {
    public:
        explicit Worker (ArchaeologistProcessor& o) : juce::Thread ("ArchWorker"), owner (o) {}
        void run() override;
        ArchaeologistProcessor& owner;
        double lastTimeMs = 0.0;
    };

    void handleAsyncUpdate() override;               // reallocate on window change
    void allocateMemory (int windowIdx);
    void buildSnapshot();
    static BusesProperties makeBuses();              // BusesProperties is protected

    juce::AudioProcessorValueTreeState apvts_;
    ParamRefs params_;

    RingBuffer       ring_;
    ArtifactStore    store_;
    AnalysisEngine   analysis_;
    ExcavationEngine excavation_;
    VoiceEngine      voices_;
    SpawnQueue       spawnQueue_;
    TripleBuffer<Snapshot> snapshot_;

    std::unique_ptr<Worker> worker_;

    juce::AudioBuffer<float> wet_;
    juce::SmoothedValue<float> wetSmooth_, gainSmooth_;

    double sampleRate_ = 48000.0;
    int    maxBlock_   = 512;
    int    channels_   = 2;
    std::atomic<int> allocatedWindowIdx_ { -1 };  // written on message thread, read on worker
    std::atomic<int> pendingWindowIdx_ { 1 };

    // edge detection (audio thread)
    bool lastDig_ = false, lastClear_ = false, lastFreeze_ = false;

    // cross-thread runtime state
    std::atomic<double> transportBpm_ { 120.0 };
    std::atomic<double> transportPpq_ { 0.0 };
    std::atomic<bool>   transportPlaying_ { false };
    std::atomic<int>    activeVoices_ { 0 };
    std::atomic<float>  inputLevel_ { 0.0f };
    std::atomic<float>  outputLevel_ { 0.0f };
    std::atomic<bool>   clearRequested_ { false };
    std::atomic<bool>   frozen_ { false };
    std::atomic<float>  lastMidiNoteHz_ { 0.0f };

    int currentProgram_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArchaeologistProcessor)
};
} // namespace arch
