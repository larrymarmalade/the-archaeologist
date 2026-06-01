#include "test_framework.h"
#include "dsp/ArtifactStore.h"

using namespace arch;

void runArtifactStoreTests()
{
    ArtifactStore store;
    store.prepare (2, 1000, 8);            // 1000-frame slab, 8 slots

    CHECK (store.aliveCount() == 0);

    // allocate + publish a few artifacts
    auto* a = store.allocate (200, 2);
    CHECK (a != nullptr);
    // fill slab so we can verify contiguity
    store.slabChannel (*a, 0)[0] = 1.0f;
    store.slabChannel (*a, 1)[0] = 2.0f;
    store.publish (*a);
    CHECK (store.aliveCount() == 1);
    CHECK (store.slabChannel (*a, 0)[0] == 1.0f);
    CHECK (store.slabChannel (*a, 1)[0] == 2.0f);

    auto* b = store.allocate (300, 2);
    CHECK (b != nullptr);
    CHECK (b->slabOffset >= a->slabOffset + a->length);   // no overlap
    store.publish (*b);
    CHECK (store.aliveCount() == 2);

    // circular reuse: fill remaining space, forcing wrap + forgetting of oldest
    int created = 2;
    for (int i = 0; i < 20; ++i)
    {
        auto* c = store.allocate (250, 2);
        if (c) { store.publish (*c); ++created; }
    }
    CHECK (created > 2);
    CHECK (store.aliveCount() <= store.maxArtifacts());

    // pinned region must not be recycled
    ArtifactStore s2; s2.prepare (1, 500, 4);
    auto* p = s2.allocate (400, 1);
    CHECK (p != nullptr);
    s2.publish (*p);
    p->refCount.fetch_add (1);                            // simulate a playing voice
    auto* q = s2.allocate (400, 1);                       // needs to overwrite p -> blocked
    CHECK (q == nullptr);
    p->refCount.fetch_sub (1);                            // voice releases
    auto* q2 = s2.allocate (400, 1);
    CHECK (q2 != nullptr);                                // now space can be reclaimed

    // retire + collect bookkeeping
    ArtifactStore s3; s3.prepare (1, 200, 4);
    auto* x = s3.allocate (50, 1); s3.publish (*x);
    CHECK (s3.aliveCount() == 1);
    x->refCount.fetch_add (1);
    s3.retire (*x);                                       // deferred (pinned)
    CHECK (s3.aliveCount() == 1);
    x->refCount.fetch_sub (1);
    s3.collect();
    CHECK (s3.aliveCount() == 0);

    // clearAll
    ArtifactStore s4; s4.prepare (1, 200, 4);
    auto* y = s4.allocate (40, 1); s4.publish (*y);
    auto* z = s4.allocate (40, 1); s4.publish (*z);
    CHECK (s4.aliveCount() == 2);
    s4.clearAll();
    CHECK (s4.aliveCount() == 0);
}
