# Testing checklist

## Automated (JUCE-free) — run now

```bash
cmake -B build && cmake --build build --target arch_tests && ./build/arch_tests
```

Covered (`tests/`), 89 checks, all passing:

* **CircularBufferTests** — wraparound correctness, erosion refusal, single-sample
  reads, mono→stereo duplication, O(1) clear.
* **ArtifactStoreTests** — allocate/publish, non-overlapping slab layout,
  circular reuse/forgetting, **pinned regions are never recycled while a voice
  references them**, deferred retire + collect bookkeeping, clearAll.
* **SelectionTests** — LockFreeQueue FIFO + full-behaviour + a 200k-item SPSC
  stress with no lost/duplicated items; TripleBuffer monotonic publish ordering.

## pluginval (VST3, strictness level 8) — PASSING

Validated against JUCE 8.0.13 build, arm64, macOS:

```
/Applications/pluginval.app/Contents/MacOS/pluginval \
    --strictness-level 8 --validate \
    "~/Library/Audio/Plug-Ins/VST3/The Archaeologist.vst3"
```

All categories pass across 5 different random seeds: open cold/warm, plugin
info, 10 programs, editor, editor-while-processing, audio + **non-releasing**
audio processing at 44.1/48/96 kHz × block 64–1024, plugin state, **plugin
state restoration**, automation, editor automation, automatable parameters,
**parameter thread safety**, **background thread state**, bus layouts
(mono/stereo + enable/disable/restore), and fuzz parameters.

Note: an earlier run failed *Plugin state restoration* for discrete (bool/choice)
parameters because `AudioParameterBool/Choice::getValue()` is unsnapped and APVTS
skips re-pushing an unchanged snapped value on restore. Fixed in
`setStateInformation()` by snapping every parameter to its legal value after
`replaceState()`.

## Manual / DAW checklist (requires a plugin host)

### Build & load
- [ ] Configures and builds Release on macOS (arm64 + x86_64).
- [ ] Builds Debug with JUCE leak detector enabled, no leaks on close.
- [x] `pluginval --strictness-level 8` passes (VST3, 5 seeds).
- [ ] Loads in: Reaper, Live, Logic (AU).
- [ ] No crash on load / unload / reload, repeated open/close of the editor.

### Audio correctness
- [ ] No clicks on voice start/stop, loop boundaries, or voice stealing.
- [ ] No denormal CPU spikes after input goes silent (leave running 5 min).
- [ ] Wet/dry and output gain behave; full dry == bit-identical passthrough.
- [ ] Test at 44.1 / 48 / 88.2 / 96 kHz.
- [ ] Test buffer sizes 32 / 64 / 128 / 512.
- [ ] Mono-in/mono-out and stereo-in/stereo-out both work.

### Behaviour
- [ ] Each of the 5 modes feels musically distinct.
- [ ] Dig Now reliably surfaces one memory.
- [ ] Freeze stops capture but playback continues; archive preserved.
- [ ] Clear Archive empties the map and silences future selection (current
      voices fade out cleanly).
- [ ] Changing Memory Window reallocates without crash or audio thread stall.
- [ ] Anti-repetition: the same fragment does not loop back-to-back.

### State & automation
- [ ] Save/restore session state round-trips all parameters.
- [ ] Every parameter automates from the host.
- [ ] Preset menu recalls all 10 presets; each produces musical results in 30 s.

### Sources to test with
- [ ] Clean electric guitar, ambient synth pad, piano, drum loop, spoken voice,
      MPE/expressive MIDI.

### Long-running
- [ ] 1-hour session: stable artifact count, no memory growth, no slab corruption.

## Profiling hooks
Build Release; the audio thread does no allocation/locking, so a sampling
profiler (Instruments → Time Profiler / Allocations on the audio thread) should
show zero malloc in `processBlock`. The background worker is the place to watch
for CPU; the autocorrelation pitch detector is its hottest loop.
