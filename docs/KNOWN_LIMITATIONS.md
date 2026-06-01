# Known limitations & future work

## Verification status

Verified:
* **Full Release build** against local JUCE 8.0.13 (macOS, arm64) — compiles and
  links cleanly (0 errors).
* **JUCE-free core unit tests**: 89/89 checks pass via `ctest` (ring buffer,
  artifact store/slab arena, lock-free queue, triple buffer).
* **VST3 passes `pluginval --strictness-level 8`** across multiple random seeds,
  including audio + non-releasing audio processing at 44.1/48/96 kHz × block
  64–1024, automation, parameter thread safety, background-thread state, bus
  layouts, fuzz, and plugin state save/restore. (A real state-restoration bug on
  discrete bool/choice parameters was found by pluginval and fixed in
  `setStateInformation()`.)
* **GUI confirmed correct in a DAW** (user-verified), after fixing UTF-8 text
  rendering (`Séance`, `·`, `—`) via `String::fromUTF8`.
* **AU passes `auval`** — a full `cmake --build` compiles the `.component` and
  (via `COPY_PLUGIN_AFTER_BUILD`) installs it to
  `~/Library/Audio/Plug-Ins/Components/`; `auval -v aufx Arch Dpti` reports
  "AU VALIDATION SUCCEEDED".
* **Debug build** (leak detector + assertions) builds cleanly, unit tests pass,
  and the Debug VST3 passes `pluginval --strictness-level 8` (no assertions or
  leaks tripped).

Not yet verified — gates before calling it shippable (see `TESTING.md`):
* **Universal binary** (arm64 + x86_64). Current builds are **arm64-only**; the
  CMake default targets both but has only been exercised at arm64 here.
* **Multi-host matrix** (Reaper / Live / Logic) and a **long-session (~1 hour)
  soak** for memory/archive stability.

## Design limitations
* **Memory footprint at large windows.** The ring *and* the artifact slab are
  each sized to the window. At 15 min / 96 kHz / stereo that is ≈ 1.3 GB total.
  Default window is 2 min. Consider capping the slab independently of the ring
  if low-memory operation matters.
* **Offline (faster-than-realtime) bounces.** Excavation scheduling uses
  wall-clock `dt` so memories keep emerging during Freeze. During an offline
  render the audio thread outruns the wall clock, so emergence under-triggers.
  Real-time use is the target; a sample-clock scheduling mode is future work.
* **Quantize Emergence is loose.** The background worker runs at ~80 Hz, so
  bar/phrase alignment is approximate (within ~12 ms), not sample-accurate.
* **Voice stealing drops rather than cuts.** When all 16 voices are busy and
  none is near-silent, a new excavation order is dropped to stay click-free.
  Under extreme density some intended memories simply won't surface.
* **MIDI capture is minimal.** Note-ons trigger excavation and set a pitch
  reference, but a full MIDI/MPE artifact layer (recording CC/pitch-bend/MPE
  dimensions as playable gesture objects) is not implemented yet.
* **Analysis is mono-summed** for segmentation/feature extraction; stereo field
  is preserved in playback but not analysed per-channel.
* **Pitch detection** is a single-frame autocorrelation (no YIN/median tracking),
  so confidence on polyphonic or noisy material is conservative by design.

## Reasonable next steps
1. Universal (arm64 + x86_64) build; install + `auval` the AU; run the
   multi-host matrix and the long-session soak from `TESTING.md`.
2. Real spectral smear/blur via an FFT phase-randomising freeze (PaulXStretch-style)
   instead of the current allpass approximation.
3. Per-channel stereo analysis and width-aware artifact placement.
4. Full MPE capture → expressive artifact playback.
5. Sample-clock excavation scheduling option for deterministic offline renders.
6. User-savable presets and an artifact "pin/protect from forgetting" gesture in
   the Archive Map.
