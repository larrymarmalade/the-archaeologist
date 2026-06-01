#include "test_framework.h"
#include "dsp/RingBuffer.h"
#include <vector>

using namespace arch;

void runCircularBufferTests()
{
    RingBuffer ring;
    ring.prepare (2, 16);                 // tiny capacity to force wraparound

    CHECK (ring.capacity() == 16);
    CHECK (ring.channels() == 2);
    CHECK (ring.totalWritten() == 0);

    // write a known ramp across the wrap boundary
    std::vector<float> l (40), r (40);
    for (int i = 0; i < 40; ++i) { l[(size_t) i] = (float) i; r[(size_t) i] = (float) -i; }
    const float* srcs[2] = { l.data(), r.data() };
    ring.write (srcs, 2, 40);
    CHECK (ring.totalWritten() == 40);

    // last 16 samples should be recoverable; older ones eroded
    float bl[16] = {}, br[16] = {};
    float* dst[2] = { bl, br };
    CHECK (ring.copyRange (24, 16, dst, 2));          // [24,40) in window
    for (int i = 0; i < 16; ++i)
    {
        CHECK (bl[i] == (float) (24 + i));
        CHECK (br[i] == (float) -(24 + i));
    }

    // eroded range must be refused
    CHECK (! ring.copyRange (0, 16, dst, 2));
    CHECK (! ring.copyRange (30, 16, dst, 2));        // runs past write head

    // single-sample accessor
    CHECK (ring.at (39, 0) == 39.0f);
    CHECK (ring.at (39, 1) == -39.0f);
    CHECK (ring.at (0, 0) == 0.0f);                   // eroded -> 0

    // mono source into stereo ring duplicates
    RingBuffer mono; mono.prepare (2, 8);
    std::vector<float> m (8); for (int i = 0; i < 8; ++i) m[(size_t) i] = (float) (i + 1);
    const float* msrc[1] = { m.data() };
    mono.write (msrc, 1, 8);
    CHECK (mono.at (5, 0) == mono.at (5, 1));

    // clear resets counters
    ring.clear();
    CHECK (ring.totalWritten() == 0);
    CHECK (! ring.copyRange (24, 16, dst, 2));
}
