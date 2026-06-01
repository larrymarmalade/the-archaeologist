#pragma once

#include "RingBuffer.h"
#include "ArtifactStore.h"
#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace arch
{
/**
    Background-thread gesture analyser + segmenter.

    Pulls newly captured audio from the RingBuffer in FFT-hop steps, derives
    per-frame features (RMS, spectral centroid, flux, flatness, a coarse pitch),
    and runs a small state machine that opens/closes musically meaningful
    "artifacts". On close it copies the fragment into the ArtifactStore slab,
    aggregates features, classifies a Family, and publishes it.

    Never runs on the audio thread; allocation happens only in prepare().
*/
class AnalysisEngine
{
public:
    void prepare (double sampleRate, int channels, ArtifactStore& store);
    void reset (int64_t startAbs);

    /** Consume available audio from `ring` up to its write head. `maxArtifactSec`
        caps fragment length. Returns number of artifacts committed this call. */
    int process (RingBuffer& ring, ArtifactStore& store, double maxArtifactSec);

    int64_t readPosition() const noexcept { return readAbs_; }

    /** Most recent live-input descriptor (background thread, for selection). */
    struct InputDescriptor { float rms = 0, brightness = 0, flatness = 0,
                             pitchHz = 0, pitchConf = 0; };
    InputDescriptor liveInput() const noexcept { return live_; }

private:
    struct FrameFeatures
    {
        float rms = 0.0f, centroid = 0.0f, flux = 0.0f, flatness = 0.0f;
        float pitchHz = 0.0f, pitchConf = 0.0f;
        bool  onset = false;
        bool  pitchComputed = false;   // false on hops where pitch was decimated out
    };

    FrameFeatures analyseFrame (const float* mono, int n);
    bool commitArtifact (RingBuffer& ring, ArtifactStore& store,
                         int64_t startAbs, int64_t endAbs);
    Family classify (const Artifact& a) const;

    double sampleRate_ = 48000.0;
    int    channels_   = 1;

    static constexpr int fftOrder = 10;            // 1024-pt
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int hop      = 256;
    // The autocorrelation pitch search is the worker's hot loop; run it only
    // every Nth hop (see analyseFrame). Identical math, ~Nx less CPU there.
    static constexpr int pitchHopDecimation = 3;

    std::unique_ptr<juce::dsp::FFT> fft_;
    std::vector<float> window_;
    std::vector<float> fftScratch_;                // 2*fftSize
    std::vector<float> prevMag_;
    std::vector<float> monoFrame_;
    std::vector<float> copyL_, copyR_;             // slab copy scratch

    int64_t readAbs_ = 0;

    // segmenter state
    bool   inGesture_      = false;
    int64_t gestureStart_  = 0;
    int     silenceFrames_ = 0;
    int     gestureFrames_ = 0;
    int     transientsInGesture_ = 0;
    int64_t lastTransientAbs_    = 0;
    std::vector<int64_t> transientTimes_;

    // running aggregates within a gesture
    double sumRms_ = 0, sumCentroid_ = 0, sumFlat_ = 0, sumFlux_ = 0;
    double peakRms_ = 0;
    float  bestPitch_ = 0.0f, bestPitchConf_ = 0.0f;

    // adaptive thresholds
    float  fluxAvg_     = 0.0f;
    float  noiseFloor_  = 1.0e-4f;
    unsigned pitchDecimCtr_ = 0;        // hop counter for pitch decimation (wraps safely)

    // rarity: running mean feature vector
    float  rmRms_ = 0, rmCentroid_ = 0, rmFlat_ = 0;
    int    rmCount_ = 0;

    InputDescriptor live_ {};
};
} // namespace arch
