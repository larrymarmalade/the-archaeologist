#pragma once
#include <cstdio>
#include <string>

namespace archtest
{
inline int& failures() { static int f = 0; return f; }
inline int& checks()   { static int c = 0; return c; }

inline void check (bool cond, const char* expr, const char* file, int line)
{
    ++checks();
    if (! cond)
    {
        ++failures();
        std::printf ("  FAIL: %s  (%s:%d)\n", expr, file, line);
    }
}
} // namespace archtest

#define CHECK(cond) ::archtest::check ((cond), #cond, __FILE__, __LINE__)

void runCircularBufferTests();
void runArtifactStoreTests();
void runStateTests();
