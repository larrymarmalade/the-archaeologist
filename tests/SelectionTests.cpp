#include "test_framework.h"
#include "state/LockFreeQueue.h"
#include "state/Snapshot.h"
#include <thread>

using namespace arch;

// These cover the lock-free hand-off primitives the selection/playback path
// relies on. (The scoring logic itself depends on JUCE and is exercised in the
// plugin build / DAW testing checklist.)
void runStateTests()
{
    // ── LockFreeQueue: FIFO, full behaviour ─────────────────────────────────
    LockFreeQueue<int, 8> q;
    int out = 0;
    CHECK (! q.pop (out));                     // empty
    for (int i = 0; i < 7; ++i) CHECK (q.push (i));   // capacity-1 usable slots
    CHECK (! q.push (99));                      // full -> rejected, never blocks
    for (int i = 0; i < 7; ++i) { CHECK (q.pop (out)); CHECK (out == i); }
    CHECK (! q.pop (out));

    // ── SPSC stress: no lost/duplicated items ───────────────────────────────
    LockFreeQueue<int, 1024> sq;
    constexpr int N = 200000;
    std::thread producer ([&]
    {
        int i = 0;
        while (i < N) if (sq.push (i)) ++i;
    });
    long long sum = 0; int got = 0, v = 0;
    while (got < N) if (sq.pop (v)) { sum += v; ++got; }
    producer.join();
    const long long expected = (long long) (N - 1) * N / 2;
    CHECK (sum == expected);

    // ── TripleBuffer: reader always sees a complete published value ──────────
    TripleBuffer<int> tb;
    tb.writeRef() = 0; tb.publish();
    std::atomic<bool> stop { false };
    std::thread writer ([&]
    {
        for (int i = 1; i <= 100000 && ! stop.load(); ++i) { tb.writeRef() = i; tb.publish(); }
    });
    int last = -1; bool monotonicOk = true;
    for (int i = 0; i < 100000; ++i)
    {
        const int r = tb.read();
        if (r < last) monotonicOk = false;     // published values only increase
        last = r;
    }
    stop = true; writer.join();
    CHECK (monotonicOk);
}
