#pragma once

#include "ArtifactVoice.h"
#include "Transformations.h"
#include <array>

namespace arch
{
/** Audio-thread polyphonic playback bus. Owns a fixed voice pool, consumes
    excavation orders, mixes voices, and applies a global spectral smear. */
class VoiceEngine
{
public:
    static constexpr int kMaxVoices = 16;     // >= 8 required

    void prepare (double sampleRate, int blockSize);
    void reset();

    /** Start a voice for this order, stealing the quietest active voice if the
        pool is full. Audio thread. */
    void spawn (const VoiceSpawn& s);

    /** Render all voices additively into the (pre-cleared) stereo buffers. */
    void render (float* outL, float* outR, int n) noexcept;

    void  setSmear (float amt01) noexcept { diffuser_.setAmount (amt01); }
    int   activeVoices() const noexcept;
    void  releaseAll() noexcept;              // gentle fade of everything

private:
    int findVoiceToUse();

    std::array<ArtifactVoice, kMaxVoices> voices_;
    Diffuser diffuser_;
    double sampleRate_ = 48000.0;
};
} // namespace arch
