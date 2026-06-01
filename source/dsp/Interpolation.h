#pragma once

#include <cmath>

namespace arch
{
/** 4-point, 3rd-order Hermite interpolation (Catmull-Rom flavour).

    High quality for varispeed / fractional read positions. xm1..x2 are the
    four consecutive samples surrounding the fractional position `t` in [0,1)
    that sits between x0 and x1. */
inline float hermite (float xm1, float x0, float x1, float x2, float t) noexcept
{
    const float c0 = x0;
    const float c1 = 0.5f * (x1 - xm1);
    const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
    return ((c3 * t + c2) * t + c1) * t + c0;
}

/** Linear interpolation — used for control-rate smoothing and cheap reads. */
inline float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }

/** Equal-power pan gains for a position in [-1, 1]. */
inline void equalPowerPan (float pan, float& lGain, float& rGain) noexcept
{
    // Map [-1,1] -> [0, pi/2]
    const float p = (pan * 0.5f + 0.5f) * 1.5707963268f;
    lGain = std::cos (p);
    rGain = std::sin (p);
}
} // namespace arch
