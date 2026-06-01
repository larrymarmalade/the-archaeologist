# The Archaeologist

*A generative resampler / memory instrument. The session haunts itself.*

The Archaeologist continuously listens to your playing, stores recent gestures
in a rolling private archive, analyses them, and later **excavates** fragments
as degraded, transformed, musically sensitive memory objects. It is not a
looper, a granular delay, a tape delay, or a random sampler. It is closer to a
small machine that *remembers* what you did and answers back — Eno tape systems
meeting Autechre machine logic meeting an Aphex Twin uncanny memory box.

Built with **JUCE 8 + CMake**, **C++20**, hard real-time-safe audio path,
**VST3 / AU / Standalone** targets.

---

## Build

Requirements: CMake ≥ 3.22, a C++20 compiler (Xcode/clang on macOS, MSVC 2022 on
Windows). JUCE is fetched automatically (pinned tag `8.0.13`).

```bash
# from the repository root
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j

# run the (JUCE-free) core unit tests
ctest --test-dir build --output-on-failure
# or directly:
./build/arch_tests
```

The built plugin is copied to your user plugin folders automatically
(`COPY_PLUGIN_AFTER_BUILD`). On macOS:

* VST3 → `~/Library/Audio/Plug-Ins/VST3/The Archaeologist.vst3`
* AU   → `~/Library/Audio/Plug-Ins/Components/The Archaeologist.component`
* Standalone app under `build/`.

### Using a local JUCE checkout (offline / faster)

```bash
cmake -B build -DARCH_USE_LOCAL_JUCE=ON -DJUCE_SOURCE_DIR=/path/to/JUCE
```

### Build types

* `Debug` — asserts on, JUCE leak detector active, no LTO.
* `Release` — optimised, LTO via `juce_recommended_lto_flags`, denormals
  flushed at runtime. This is the profiling target.

---

## What it does, in one paragraph

A stereo **circular capture buffer** records your input pre-FX. A **background
analysis thread** segments that audio into *artifacts* — musically meaningful
fragments — classifies each into a family (attack, sustained tone, chord/cloud,
noise, silence, rhythmic cell, unstable), and copies it into a fixed sample
*slab*. An **excavation engine** periodically decides *when* a memory should
surface and *which* one, balancing age, similarity/contrast to what you're
playing now, rarity, salience, silence and anti-repetition, all shaped by the
current **behaviour mode**. Selected artifacts are handed to a polyphonic
**voice engine** that plays them back through per-voice transformations
(varispeed, reverse, micro-loop, freeze, granular cloud, tape aging,
band-limiting, wow/flutter, pitch drift, stereo wander, ghost pre-echo) and a
shared spectral smear. A lock-free snapshot drives a living **Archive Map** UI.

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for the full design,
[`docs/PARAMETERS.md`](docs/PARAMETERS.md) for the parameter reference,
[`docs/PRESETS.md`](docs/PRESETS.md) for the factory presets,
[`docs/TESTING.md`](docs/TESTING.md) for the test checklist,
[`docs/KNOWN_LIMITATIONS.md`](docs/KNOWN_LIMITATIONS.md), and
[`docs/THIRD_PARTY.md`](docs/THIRD_PARTY.md).

## Quick start (musical)

1. Insert on a track and play for ~20–30 seconds. Watch glyphs accumulate in the
   Archive Map.
2. Pick a mode (start with **Drift** or **Séance**).
3. Raise **Rate** and **Density** to invite more memories; raise **Ruin** and
   **Fidelity** to age them.
4. Hit **Dig Now** to force a memory to surface immediately.
5. **Freeze Archive** to stop capturing and let the instrument play only with
   what it already remembers.
6. Try the **Ghost of the Take Before** preset with **Recognition** high — it
   tends to answer phrases with related material.

## License

The Archaeologist is released under the **GNU General Public License v3.0** (see
[`LICENSE`](LICENSE)), to match JUCE's open-source licence. You're free to use,
study, modify, and share it; derivative works must also be GPLv3. JUCE itself is
dual-licensed (GPLv3 / commercial) — see [`docs/THIRD_PARTY.md`](docs/THIRD_PARTY.md).

## Contributing

A non-commercial labour of love — tinkering, forks, and pull requests are
welcome. Good places to dig in are in
[`docs/KNOWN_LIMITATIONS.md`](docs/KNOWN_LIMITATIONS.md) (real FFT spectral
smear, MPE capture, per-channel stereo analysis, sample-clock scheduling). Build
and test instructions are above; please keep the audio thread allocation/lock
free and run `ctest` + `pluginval` before submitting.
