# Architecture

## Threading model (the most important thing)

Three roles, no locks on the audio path, no allocation on the audio path.

```
                 ┌──────────────────────────────────────────────┐
   input audio → │ AUDIO THREAD (processBlock)                   │
                 │  • write input → RingBuffer (unless frozen)   │
                 │  • publish transport (bpm/ppq/playing)        │
                 │  • detect Dig/Clear/Freeze param edges        │
                 │  • drain SpawnQueue → VoiceEngine.spawn()     │
                 │  • VoiceEngine.render() + global smear        │
                 │  • wet/dry mix + output gain + metering       │
                 └───────────────┬───────────────▲──────────────┘
            RingBuffer (SPSC)     │               │ SpawnQueue (SPSC, lock-free)
            written→ acquire      ▼               │
                 ┌──────────────────────────────────────────────┐
                 │ BACKGROUND WORKER THREAD (~80 Hz)             │
                 │  • AnalysisEngine: segment → classify         │
                 │    → copy fragment into ArtifactStore slab    │
                 │  • ExcavationEngine: schedule + score + emit  │
                 │  • store.collect() (reclaim retired slots)    │
                 │  • build Snapshot → TripleBuffer.publish()    │
                 └───────────────────────────────┬──────────────┘
                                  Snapshot         │ TripleBuffer (SPSC, lock-free)
                                                   ▼
                 ┌──────────────────────────────────────────────┐
                 │ UI THREAD (editor timer, 30 Hz)               │
                 │  • TripleBuffer.read() → ArchiveMap + Detail  │
                 └──────────────────────────────────────────────┘
```

* **RingBuffer** — single writer (audio), single reader (background). The reader
  only ever touches samples strictly older than the write head, so they never
  address the same cell. `totalWritten` is the monotonic 64-bit clock; reads of
  eroded ranges are refused.
* **ArtifactStore** — owned/mutated structurally by the background thread only.
  The audio thread reads published artifacts + slab samples and mutates only
  per-artifact atomics (`refCount`, `playCount`, `lastPlayedAtSec`). A slab
  region is **never recycled while `refCount > 0`**, so a playing voice can
  never read reclaimed memory.
* **SpawnQueue** / **TripleBuffer** — lock-free SPSC primitives
  (`source/state/`). The triple buffer uses a dirty-bit index swap so the reader
  always observes a complete, monotonic sequence of published snapshots.
* **Memory reallocation** (changing the Memory Window) happens on the message
  thread inside `handleAsyncUpdate()`, which suspends the audio thread *and*
  stops the worker before touching the shared buffers.

## The pipeline

### 1. Input Capture (`RingBuffer`)
Stereo (mono gracefully duplicated). Window sizes 30 s / 2 min / 5 min / 15 min,
allocated up front. Captures pre-FX. Freeze stops the write head, preserving the
archive (and is O(1) — `clear()` resets counters without zeroing memory, so it
is audio-thread safe).

### 2. Gesture Analysis (`AnalysisEngine`)
Runs on the background thread in FFT hops (1024-pt, hop 256). Per frame it
derives RMS, spectral centroid (brightness), spectral flux (onset), spectral
flatness (noisiness), and a coarse autocorrelation pitch + confidence. A small
state machine opens a gesture on an onset above the adaptive noise floor and
closes it on sustained silence or a max-length cap (driven by **Artifact
Length**). On close it copies the fragment into the slab and computes aggregate
features, a **family** classification, a **rarity** score (distance from a
running feature mean) and an **emotional weight** (rarity + clarity + loudness +
a family prior).

### 3. Excavation (`ExcavationEngine`)
The brain. Each background tick it decides whether a memory should surface based
on **Excavation Rate** (interval), **Density** + mode (concurrency budget), host
transport + **Quantize Emergence**, and **Dig Now**. When it fires it scores
every live, non-suppressed artifact:

```
score =  0.25·ageScore        (Memory Depth: fresh↔old)
       + 0.22·similarityScore  (Recognition: contrast↔call-and-response)
       + 0.18·rarity
       + 0.25·emotionalWeight
       + silenceBoost          (Favor Silence, when input is quiet)
       − clusterPenalty        (near-identical-to-recent suppression)
       + jitter                (mode emergenceJitter → probabilistic)
```

Anti-repetition: a rolling list of recently played ids is suppressed for a
rate-dependent window, and clustering penalises material that is too close to
something just played. **Forgetting** proactively retires the weakest artifacts
over time; the slab also forgets oldest-first as it wraps.

### 4. Transformation (`Transformations`, `ArtifactVoice`)
Per voice: varispeed (Hermite interpolation), reverse, micro-loop with unstable
boundaries, freeze-like sustain (tiny crossfaded loop), granular cloud
(overlapping Hann grains with spray), bit-depth + sample-rate **aging**,
band-limited archival coloration (one-pole HP/LP per channel), wow/flutter and
slow pitch drift (LFOs on playback rate), probabilistic silence gating, stereo
wander, and ghost **pre-echo** (reading ahead in the fragment). A shared
allpass **Diffuser** provides global spectral smear. Cosine attack/release
envelopes keep everything click-free.

### 5. Playback / Voices (`VoiceEngine`)
Fixed pool of 16 voices (≥ 8 required). Voice stealing only reuses a near-silent
voice; otherwise the order is dropped (a memory simply doesn't surface) so there
are no audible cuts. **Choir** mode emits detuned, panned copies; **Relic
Stack** layers a few of the oldest salient artifacts into a composite ghost.

### 6. Behaviour Modes (`BehaviorModes`)
Drift, Fracture, Nursery, Fog, Séance. Each is a `ModeProfile` of biases
(density, jitter, rupture, similarity, fades, drift, reverse/granular/loop
probabilities, stereo width, ruin scaling, silence affinity, choir/relic
probability, varispeed spread) that shape selection and transformation without a
separate code path.

## Real-time safety rules honoured
No heap allocation, file I/O, locks, logging, or UI calls on the audio thread.
The slab copy (potentially large `memcpy`) happens on the background thread, not
in `processBlock`. Parameters are read through cached `std::atomic<float>*` and
smoothed. `ScopedNoDenormals` wraps the block.

## Source layout
```
source/
  PluginProcessor.*      orchestration, threads, state, presets, metering
  PluginEditor.*         window, 30 Hz snapshot pump
  params/                APVTS layout, cached refs, behaviour-mode profiles
  state/                 LockFreeQueue, TripleBuffer + Snapshot
  dsp/                   RingBuffer, Artifact, ArtifactStore, AnalysisEngine,
                         ExcavationEngine, VoiceEngine, ArtifactVoice,
                         Transformations, Interpolation, DenormalGuard
  presets/               factory presets
  ui/                    LookAndFeel, ArchiveMap, ModeSelector, MacroPanel,
                         DetailPanel, TransportBar, Theme
tests/                   JUCE-free unit tests (ring buffer, store, queues)
```
