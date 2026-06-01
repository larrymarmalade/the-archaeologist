#include "test_framework.h"
#include <cstdio>

int main()
{
    std::printf ("The Archaeologist — unit tests\n");
    std::printf ("running circular buffer tests...\n");  runCircularBufferTests();
    std::printf ("running artifact store tests...\n");    runArtifactStoreTests();
    std::printf ("running state/queue tests...\n");       runStateTests();

    std::printf ("\n%d checks, %d failures\n", archtest::checks(), archtest::failures());
    return archtest::failures() == 0 ? 0 : 1;
}
