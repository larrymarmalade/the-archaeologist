# Factory presets

Defined in `source/presets/Presets.cpp` and exposed as host programs. Each is
tuned to demonstrate a distinct behaviour within ~30 seconds of playing.

| # | Name | Mode | Character |
|---|------|------|-----------|
| 1 | **Empty Museum** | Drift | Very sparse, mostly dry, favours silence. The instrument barely speaks — long pauses between fragile, clean surfacings. |
| 2 | **Old Tape Under Snow** | Fog | Heavy fidelity decay + ruin, slow wow/flutter, wide and muffled. Memories arrive wrapped in oxide and frost. |
| 3 | **Broken Children's Piano** | Nursery | Mid ruin, micro-loops, pitch drift. Tonal fragments warped into an uncanny music box. |
| 4 | **Delaware Crossing Fever Dream** | Séance | Dense, layered, choir + relic stack on. Crowded, hallucinatory call-and-response. |
| 5 | **Glassine Loop Rot** | Fracture | Short fragments, high ruin/decay, low continuity. Brittle loops decaying into glassy rot. |
| 6 | **Eno in a Cold Room** | Drift | Sparse, very long fades, high beauty, favours silence. Generative, gentle, non-insistent. |
| 7 | **Aphex Attic Toy** | Nursery | Playful varispeed/reverse, bar-quantised. Eerie wind-up-toy repetition. |
| 8 | **Autechre Fossil Machine** | Fracture | High rate/density, low continuity, rupture-biased, bar-quantised. Angular, machine-like, glitchy. |
| 9 | **ECM Afterimage** | Fog | Restrained, wide stereo, soft attacks, high beauty, lots of air. Cool harmonic clouds. |
| 10 | **Ghost of the Take Before** | Séance | Recognition very high — the most "answering" patch. It tends to reply to your phrases with related material. |

`presets::apply()` resets the transient toggles (favorSilence, choir, relic,
freeze) before applying a preset so recall is reproducible.
