#pragma once

#include <cmath>
#include <array>
#include <vector>
#include <juce_core/juce_core.h>

namespace arch
{
/** One-pole low/high-pass — cheap archival band-limiting. */
struct OnePole
{
    float a = 0.0f, zLP = 0.0f, zHP = 0.0f;
    void setCutoff (float normFc) noexcept           // normFc in (0,0.5]
    {
        const float fc = juce::jlimit (0.0005f, 0.49f, normFc);
        a = std::exp (-juce::MathConstants<float>::twoPi * fc);
    }
    float lp (float x) noexcept { zLP = (1.0f - a) * x + a * zLP; return zLP; }
    float hp (float x) noexcept { zHP = (1.0f - a) * x + a * zHP; return x - zHP; }
    void reset() noexcept { zLP = zHP = 0.0f; }
};

/** Sample-rate reduction + bit-depth quantisation ("tape/digital aging"). */
struct Ager
{
    float  bits     = 16.0f;     // effective bit depth
    int    holdLen  = 1;         // sample & hold length (samplerate reduction)
    int    holdCtr  = 0;
    float  heldL = 0.0f, heldR = 0.0f;

    void set (float ruin01) noexcept
    {
        bits    = juce::jmap (juce::jlimit (0.0f, 1.0f, ruin01), 16.0f, 5.0f);
        holdLen = 1 + static_cast<int> (ruin01 * ruin01 * 18.0f);
    }
    static float crush (float x, float bits) noexcept
    {
        const float steps = std::pow (2.0f, bits);
        return std::round (x * steps) / steps;
    }
    void process (float& l, float& r) noexcept
    {
        if (holdCtr <= 0) { heldL = l; heldR = r; holdCtr = holdLen; }
        --holdCtr;
        l = crush (heldL, bits);
        r = crush (heldR, bits);
    }
    void reset() noexcept { holdCtr = 0; heldL = heldR = 0.0f; }
};

/** Bipolar sine/triangle LFO for wow/flutter, drift, pan-wander, gating. */
struct LFO
{
    double phase = 0.0, inc = 0.0;
    void setRate (double hz, double sr) noexcept { inc = hz / sr; }
    void setPhase (double p) noexcept { phase = p; }
    float sine() noexcept
    {
        const float v = (float) std::sin (phase * juce::MathConstants<double>::twoPi);
        advance();
        return v;
    }
    float tri() noexcept
    {
        const float v = 4.0f * std::abs (static_cast<float> (phase) - 0.5f) - 1.0f;
        advance();
        return v;
    }
    void advance() noexcept { phase += inc; if (phase >= 1.0) phase -= 1.0; }
};

/** Short allpass diffusion chain — approximates a gentle spectral smear/blur. */
class Diffuser
{
public:
    void prepare (double sampleRate);
    void reset();
    void setAmount (float amt01) noexcept { mix_ = juce::jlimit (0.0f, 1.0f, amt01); }
    void process (float& l, float& r) noexcept;

private:
    struct AP { std::vector<float> buf; int idx = 0; float g = 0.6f;
                float process (float x) noexcept; void reset(); };
    std::array<AP, 4> apL_, apR_;
    float mix_ = 0.0f;
};
} // namespace arch
