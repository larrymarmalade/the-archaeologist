#include "Transformations.h"

namespace arch
{
float Diffuser::AP::process (float x) noexcept
{
    const int n = static_cast<int> (buf.size());
    if (n == 0) return x;
    const float d = buf[static_cast<size_t> (idx)];
    const float y = -g * x + d;
    buf[static_cast<size_t> (idx)] = x + g * y;
    idx = (idx + 1) % n;
    return y;
}

void Diffuser::AP::reset() { std::fill (buf.begin(), buf.end(), 0.0f); idx = 0; }

void Diffuser::prepare (double sampleRate)
{
    // prime-ish delay lengths in samples, scaled to sample rate
    const double base = sampleRate / 1000.0;          // ~1 ms unit
    const float gains[4]  = { 0.62f, 0.58f, 0.54f, 0.5f };
    const double lensL[4] = { 4.77, 7.31, 11.9, 17.3 };
    const double lensR[4] = { 5.43, 8.11, 12.7, 19.1 };
    for (int i = 0; i < 4; ++i)
    {
        apL_[(size_t) i].buf.assign (std::max (1, (int) (lensL[i] * base)), 0.0f);
        apR_[(size_t) i].buf.assign (std::max (1, (int) (lensR[i] * base)), 0.0f);
        apL_[(size_t) i].g = gains[i];
        apR_[(size_t) i].g = gains[i];
    }
}

void Diffuser::reset()
{
    for (auto& a : apL_) a.reset();
    for (auto& a : apR_) a.reset();
}

void Diffuser::process (float& l, float& r) noexcept
{
    if (mix_ <= 0.0001f) return;
    float wl = l, wr = r;
    for (int i = 0; i < 4; ++i) { wl = apL_[(size_t) i].process (wl); wr = apR_[(size_t) i].process (wr); }
    l = l + mix_ * (wl - l);
    r = r + mix_ * (wr - r);
}
} // namespace arch
