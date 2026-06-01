# Parameter reference

All parameters are exposed through `AudioProcessorValueTreeState` and are fully
host-automatable. IDs are defined in `source/params/Parameters.h`.

## Macros (0–1 unless noted)

| ID | Name | Default | Effect |
|----|------|---------|--------|
| `excavationRate` | Excavation Rate | 0.35 | How often memories surface (interval, squared mapping). |
| `memoryDepth` | Memory Depth | 0.50 | 0 prefers fresh fragments, 1 prefers old ones. |
| `artifactLength` | Artifact Length | 0.45 | Caps fragment length, 0.25 s … 8 s. |
| `recognition` | Recognition | 0.40 | Similarity-to-current-input bias: high → call-and-response, low → contrast-seeking. |
| `forgetting` | Forgetting | 0.30 | How aggressively weak/old artifacts are retired. |
| `fidelityDecay` | Fidelity Decay | 0.40 | Adds degradation with artifact age. |
| `ruin` | Ruin | 0.25 | Spectral/granular/tape damage amount. |
| `continuity` | Continuity | 0.60 | 1 = smooth contextual fades, 0 = violent ruptures (short stabs). |
| `beauty` | Beauty | 0.60 | 1 = clean/airy, 0 = damaged; scales smear & ruin. |
| `density` | Density | 0.40 | Concurrent-voice budget (with mode). |

## Output

| ID | Name | Default | Range |
|----|------|---------|-------|
| `wetDry` | Wet/Dry | 0.50 | 0–1 |
| `outputGain` | Output Gain | 0 dB | −36 … +12 dB |

## Choices

| ID | Name | Options | Default |
|----|------|---------|---------|
| `mode` | Mode | Drift, Fracture, Nursery, Fog, Seance | Drift |
| `memoryWindow` | Memory Window | 30 s, 2 min, 5 min, 15 min | 2 min |
| `quantize` | Quantize Emergence | Off, Loose, Bar, Phrase | Loose |

## Toggles

| ID | Name | Default | Notes |
|----|------|---------|-------|
| `freezeArchive` | Freeze Archive | off | Stops capture; archive preserved; playback continues. |
| `favorSilence` | Favor Silence | off | Surfaces memories preferentially into quiet input. |
| `syncToHost` | Sync to Host | on | Enables tempo-aware emergence quantisation. |
| `choirMode` | Choir Mode | off | One artifact → several detuned/offset copies. |
| `relicStack` | Relic Stack | off | Layers 2–4 old artifacts into a composite ghost. |

## Momentary (rising-edge) triggers

| ID | Name | Notes |
|----|------|-------|
| `digNow` | Dig Now | Forces one excavation. UI pulses it; automatable. |
| `clearArchive` | Clear Archive | Clears the ring + retires all artifacts. |

## UI macro mapping

The six large knobs map to: **Depth** → `memoryDepth`, **Rate** →
`excavationRate`, **Ruin** → `ruin`, **Beauty** → `beauty`, **Density** →
`density`, **Memory** → `forgetting`. The secondary row exposes Recognition,
Length, Fidelity, Continuity. Window/Quantize/toggles/Wet/Out live in the
transport bar.

## MIDI

Note-ons request an excavation (the archive "answers" a played note) and update
a last-note reference for future pitch-aware selection. A fuller MPE/CC capture
layer is noted as future work in `KNOWN_LIMITATIONS.md`.
