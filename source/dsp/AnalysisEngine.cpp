#include "AnalysisEngine.h"
#include <cmath>
#include <algorithm>

namespace arch
{
void AnalysisEngine::prepare (double sampleRate, int channels, ArtifactStore&)
{
    sampleRate_ = sampleRate;
    channels_   = std::max (1, channels);

    fft_ = std::make_unique<juce::dsp::FFT> (fftOrder);
    window_.resize (fftSize);
    for (int i = 0; i < fftSize; ++i)                       // Hann
        window_[i] = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * i / (fftSize - 1)));

    fftScratch_.assign (static_cast<size_t> (2 * fftSize), 0.0f);
    prevMag_.assign (static_cast<size_t> (fftSize / 2 + 1), 0.0f);
    monoFrame_.assign (static_cast<size_t> (fftSize), 0.0f);
    copyL_.assign (1, 0.0f);
    copyR_.assign (1, 0.0f);
    transientTimes_.reserve (64);
}

void AnalysisEngine::reset (int64_t startAbs)
{
    readAbs_       = startAbs;
    inGesture_     = false;
    silenceFrames_ = 0;
    gestureFrames_ = 0;
    transientsInGesture_ = 0;
    std::fill (prevMag_.begin(), prevMag_.end(), 0.0f);
    fluxAvg_    = 0.0f;
    noiseFloor_ = 1.0e-4f;
    pitchDecimCtr_ = 0;
    rmCount_    = 0;
    rmRms_ = rmCentroid_ = rmFlat_ = 0.0f;
}

AnalysisEngine::FrameFeatures AnalysisEngine::analyseFrame (const float* mono, int n)
{
    FrameFeatures f;

    // time-domain RMS
    double e = 0.0;
    for (int i = 0; i < n; ++i) e += static_cast<double> (mono[i]) * mono[i];
    f.rms = static_cast<float> (std::sqrt (e / n));

    // windowed spectrum
    for (int i = 0; i < fftSize; ++i)
        fftScratch_[static_cast<size_t> (i)] = mono[i] * window_[static_cast<size_t> (i)];
    std::fill (fftScratch_.begin() + fftSize, fftScratch_.end(), 0.0f);
    fft_->performFrequencyOnlyForwardTransform (fftScratch_.data());

    const int bins = fftSize / 2;
    double sumMag = 0.0, sumFMag = 0.0, sumLog = 0.0, flux = 0.0;
    for (int b = 1; b <= bins; ++b)
    {
        const float mag = fftScratch_[static_cast<size_t> (b)];
        sumMag  += mag;
        sumFMag += static_cast<double> (b) * mag;
        sumLog  += std::log (mag + 1.0e-9f);
        const float d = mag - prevMag_[static_cast<size_t> (b)];
        if (d > 0.0f) flux += d;
        prevMag_[static_cast<size_t> (b)] = mag;
    }
    if (sumMag > 1.0e-9)
    {
        f.centroid = static_cast<float> ((sumFMag / sumMag) / bins);          // 0..1
        const double arith = sumMag / bins;
        const double geo   = std::exp (sumLog / bins);
        f.flatness = static_cast<float> (juce::jlimit (0.0, 1.0, geo / (arith + 1.0e-12)));
    }
    f.flux = static_cast<float> (flux);

    // adaptive flux mean & onset flag
    fluxAvg_ = 0.97f * fluxAvg_ + 0.03f * f.flux;
    f.onset  = (f.flux > fluxAvg_ * 1.7f + 1.0e-3f) && (f.rms > noiseFloor_ * 3.0f);

    // Coarse pitch via autocorrelation — the worker's hottest loop (~900 lags x
    // 1024 samples at 96 kHz), so it runs only every `pitchHopDecimation` hops.
    // The math is identical when it runs; the live descriptor holds pitch between
    // updates and the segmenter keeps the best-of across a gesture, so the
    // reduced update rate is musically negligible for ~3x less CPU here.
    if ((pitchDecimCtr_++ % pitchHopDecimation) == 0)
    {
        f.pitchComputed = true;                 // we evaluated pitch this hop
        if (f.rms > noiseFloor_ * 3.0f)
        {
            const int minLag = std::max (2, static_cast<int> (sampleRate_ / 1000.0));
            const int maxLag = std::min (n - 1, static_cast<int> (sampleRate_ / 50.0));
            double r0 = e;
            double bestVal = 0.0; int bestLag = 0;
            for (int lag = minLag; lag <= maxLag; ++lag)
            {
                double acc = 0.0;
                for (int i = 0; i + lag < n; ++i)
                    acc += static_cast<double> (mono[i]) * mono[i + lag];
                if (acc > bestVal) { bestVal = acc; bestLag = lag; }
            }
            if (bestLag > 0 && r0 > 1.0e-9)
            {
                const float conf = static_cast<float> (juce::jlimit (0.0, 1.0, bestVal / r0));
                if (conf > 0.55f)
                {
                    f.pitchHz   = static_cast<float> (sampleRate_ / bestLag);
                    f.pitchConf = conf;
                }
            }
        }
    }
    return f;
}

int AnalysisEngine::process (RingBuffer& ring, ArtifactStore& store, double maxArtifactSec)
{
    const int64_t written = ring.totalWritten();
    const int     cap     = ring.capacity();

    // If we fell behind the erosion horizon, skip forward and drop the gesture.
    if (readAbs_ < written - cap + fftSize)
    {
        readAbs_   = written - cap + fftSize;
        inGesture_ = false;
    }

    const int    maxArt   = juce::jlimit (1, store.slabFrames(),
                                          static_cast<int> (maxArtifactSec * sampleRate_));
    const int    minGest  = static_cast<int> (0.05 * sampleRate_);
    const int    silClose = std::max (1, static_cast<int> ((0.18 * sampleRate_) / hop));
    const int    minTransGap = static_cast<int> (0.04 * sampleRate_);

    int committed = 0;

    float* dst[1] = { monoFrame_.data() };
    while (readAbs_ + fftSize <= written)
    {
        // build a mono frame [readAbs_, readAbs_+fftSize)
        if (ring.channels() == 1)
        {
            if (! ring.copyRange (readAbs_, fftSize, dst, 1)) break;
        }
        else
        {
            for (int i = 0; i < fftSize; ++i)
                monoFrame_[static_cast<size_t> (i)] =
                    0.5f * (ring.at (readAbs_ + i, 0) + ring.at (readAbs_ + i, 1));
        }

        const FrameFeatures f = analyseFrame (monoFrame_.data(), fftSize);

        // smooth a live-input descriptor for the selection engine
        live_.rms        = 0.85f * live_.rms        + 0.15f * f.rms;
        live_.brightness = 0.85f * live_.brightness + 0.15f * f.centroid;
        live_.flatness   = 0.85f * live_.flatness   + 0.15f * f.flatness;
        if (f.pitchComputed)     // only update/decay on hops where pitch was measured
        {
            if (f.pitchConf > 0.55f) { live_.pitchHz = f.pitchHz; live_.pitchConf = f.pitchConf; }
            else                     { live_.pitchConf *= 0.9f; }
        }

        const int64_t frameAbs = readAbs_;
        const float silenceThresh = juce::jmax (3.0f * noiseFloor_, 2.0e-4f);
        const bool   loud = f.rms > silenceThresh;

        if (! inGesture_)
        {
            noiseFloor_ = 0.995f * noiseFloor_ + 0.005f * f.rms;
            if (loud && (f.onset || f.rms > silenceThresh * 2.0f))
            {
                inGesture_      = true;
                gestureStart_   = frameAbs;
                silenceFrames_  = 0;
                gestureFrames_  = 0;
                transientsInGesture_ = 1;
                lastTransientAbs_    = frameAbs;
                transientTimes_.clear();
                transientTimes_.push_back (frameAbs);
                sumRms_ = sumCentroid_ = sumFlat_ = sumFlux_ = 0.0;
                peakRms_ = 0.0;
                bestPitch_ = f.pitchHz; bestPitchConf_ = f.pitchConf;
            }
        }
        else
        {
            ++gestureFrames_;
            sumRms_      += f.rms;
            sumCentroid_ += f.centroid;
            sumFlat_     += f.flatness;
            sumFlux_     += f.flux;
            peakRms_      = std::max (peakRms_, (double) f.rms);
            if (f.pitchConf > bestPitchConf_) { bestPitchConf_ = f.pitchConf; bestPitch_ = f.pitchHz; }

            if (f.onset && frameAbs - lastTransientAbs_ > minTransGap)
            {
                ++transientsInGesture_;
                lastTransientAbs_ = frameAbs;
                if (transientTimes_.size() < 64) transientTimes_.push_back (frameAbs);
            }

            silenceFrames_ = loud ? 0 : silenceFrames_ + 1;

            const int64_t curLen = frameAbs - gestureStart_;
            const bool tooLong = curLen >= maxArt;
            const bool silDone = silenceFrames_ >= silClose;
            if (tooLong || silDone)
            {
                const int64_t endAbs = silDone ? frameAbs - (int64_t) silenceFrames_ * hop
                                               : frameAbs;
                if (endAbs - gestureStart_ >= minGest)
                    if (commitArtifact (ring, store, gestureStart_, endAbs))
                        ++committed;
                inGesture_ = false;
            }
        }

        readAbs_ += hop;
    }
    return committed;
}

bool AnalysisEngine::commitArtifact (RingBuffer& ring, ArtifactStore& store,
                                     int64_t startAbs, int64_t endAbs)
{
    int length = static_cast<int> (endAbs - startAbs);
    length = std::min (length, store.slabFrames());
    if (length <= 0) return false;

    const int ch = std::min (channels_, store.channels());
    Artifact* ap = store.allocate (length, ch);
    if (ap == nullptr) return false;
    Artifact& a = *ap;

    float* dst[2] = { store.slabChannel (a, 0), store.slabChannel (a, std::min (1, ch - 1)) };
    if (! ring.copyRange (startAbs, length, dst, ch))
        return false;                                       // eroded mid-commit

    const int frames = std::max (1, gestureFrames_);
    a.capturedAtAbs   = startAbs;
    a.capturedAtSec   = static_cast<double> (startAbs) / sampleRate_;
    a.durationSec     = static_cast<double> (length) / sampleRate_;
    a.loudnessRms     = static_cast<float> (sumRms_ / frames);
    a.loudnessDb      = juce::Decibels::gainToDecibels (juce::jmax (1.0e-6f, a.loudnessRms));
    a.brightness      = juce::jlimit (0.0f, 1.0f, static_cast<float> (sumCentroid_ / frames));
    a.flatness        = juce::jlimit (0.0f, 1.0f, static_cast<float> (sumFlat_ / frames));
    a.harmonicity     = 1.0f - a.flatness;
    a.pitchHz         = bestPitchConf_ > 0.55f ? bestPitch_ : 0.0f;
    a.pitchConfidence = bestPitchConf_;
    a.transientCount  = transientsInGesture_;
    a.transientDensity= juce::jlimit (0.0f, 1.0f,
                            (float) transientsInGesture_ / (float) (a.durationSec * 10.0 + 1.0));
    a.family          = classify (a);

    // rarity: normalised distance from running feature mean
    float rarity = 0.5f;
    if (rmCount_ > 4)
    {
        const float d = std::abs (a.loudnessRms - rmRms_) * 8.0f
                      + std::abs (a.brightness  - rmCentroid_) * 4.0f
                      + std::abs (a.flatness    - rmFlat_) * 4.0f;
        rarity = std::tanh (d);
    }
    a.rarity = rarity;
    const float invN = 1.0f / (rmCount_ + 1);
    rmRms_      += (a.loudnessRms - rmRms_) * invN;
    rmCentroid_ += (a.brightness  - rmCentroid_) * invN;
    rmFlat_     += (a.flatness    - rmFlat_) * invN;
    ++rmCount_;

    // family "beauty" prior
    auto beautyOf = [] (Family fam) -> float
    {
        switch (fam) {
            case Family::SustainedTone: return 0.9f;
            case Family::ChordCloud:    return 0.85f;
            case Family::RhythmicCell:  return 0.6f;
            case Family::AttackFragment:return 0.55f;
            case Family::Silence:       return 0.4f;
            case Family::NoiseScrape:   return 0.35f;
            default:                    return 0.45f;
        }
    };
    const float loudN = juce::jlimit (0.0f, 1.0f, (a.loudnessDb + 48.0f) / 48.0f);
    a.emotionalWeight = juce::jlimit (0.0f, 1.0f,
          0.32f * a.rarity + 0.24f * a.pitchConfidence
        + 0.18f * loudN    + 0.26f * beautyOf (a.family));

    store.publish (a);
    return true;
}

Family AnalysisEngine::classify (const Artifact& a) const
{
    const double dur = a.durationSec;
    if (a.loudnessDb < -45.0f)                                   return Family::Silence;
    if (a.flatness > 0.55f && a.pitchConfidence < 0.40f)         return Family::NoiseScrape;
    if (a.transientCount >= 3)                                   return Family::RhythmicCell;
    if (dur < 0.35 && a.transientCount >= 1 && a.transientDensity > 0.45f)
                                                                 return Family::AttackFragment;
    if (dur >= 0.8 && a.pitchConfidence > 0.55f && a.flatness < 0.40f)
                                                                 return Family::SustainedTone;
    if (a.harmonicity > 0.5f && a.pitchConfidence < 0.55f && a.loudnessDb > -35.0f)
                                                                 return Family::ChordCloud;
    return Family::Unstable;
}
} // namespace arch
