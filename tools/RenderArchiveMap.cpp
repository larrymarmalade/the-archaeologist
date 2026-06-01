// Offline renderer for a README hero image of the real ArchiveMapComponent.
// Renders the actual UI paint code (no mockup, no screen capture) populated with
// a representative snapshot, to a PNG. Reproducible:
//   cmake -B build -DARCH_BUILD_TOOLS=ON [...]
//   cmake --build build --target arch_render_map
//   ./build/.../arch_render_map docs/archive-map.png
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/ArchiveMapComponent.h"
#include "state/Snapshot.h"
#include <cmath>
#include <cstdio>

using namespace arch;

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI guiInit;   // fonts / graphics subsystem

    const int W = 1000, H = 520;
    const juce::String outArg = argc > 1 ? juce::String (argv[1]) : juce::String ("archive-map.png");

    // A representative archive: varied families, ages, brightness, a few surfacing.
    Snapshot snap;
    juce::Random rng (20260601);
    static const Family fams[] = { Family::SustainedTone, Family::ChordCloud,
                                   Family::AttackFragment, Family::RhythmicCell,
                                   Family::NoiseScrape, Family::Silence };
    const int n = 46;
    for (int i = 0; i < n; ++i)
    {
        ArtifactView v;
        v.id              = (uint32_t) (i + 1);
        v.family          = fams[i % 6];
        v.ageSec          = rng.nextFloat() * 55.0f;
        v.brightness      = juce::jlimit (0.0f, 0.24f, rng.nextFloat() * 0.24f);
        v.loudnessDb      = -4.0f - rng.nextFloat() * 42.0f;
        v.rarity          = rng.nextFloat();
        v.emotionalWeight = rng.nextFloat();
        const bool dig    = (i % 11 == 0);
        v.excavating      = dig;
        v.excitation      = dig ? 1.0f : std::pow (rng.nextFloat(), 2.0f) * 0.5f;
        snap.views[(size_t) i] = v;
    }
    snap.viewCount     = n;
    snap.aliveTotal    = n;
    snap.activeVoices  = 4;
    snap.mode          = Mode::Seance;
    snap.sessionSeconds = 75.0;

    ArchiveMapComponent map;
    map.setBounds (0, 0, W, H);
    for (int f = 0; f < 40; ++f)        // settle glyph positions + appearance
        map.update (snap, 0.1f);

    const auto img = map.createComponentSnapshot (map.getLocalBounds(), true, 2.0f);  // @2x

    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile (outArg);
    out.deleteFile();
    if (auto fos = std::unique_ptr<juce::FileOutputStream> (out.createOutputStream()))
    {
        juce::PNGImageFormat png;
        if (png.writeImageToStream (img, *fos))
        {
            fos->flush();
            std::printf ("wrote %s (%dx%d @2x)\n", out.getFullPathName().toRawUTF8(),
                         W, H);
            return 0;
        }
    }
    std::printf ("FAILED to write %s\n", out.getFullPathName().toRawUTF8());
    return 1;
}
