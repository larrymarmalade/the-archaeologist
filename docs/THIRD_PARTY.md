# Third-party code & algorithm notes

## JUCE
* **JUCE 8** (https://github.com/juce-framework/JUCE). By default CMake fetches
  it via FetchContent pinned to tag `8.0.13`; pass `-DARCH_USE_LOCAL_JUCE=ON
  -DJUCE_SOURCE_DIR=...` to build against a local checkout instead. Builds were
  compiled and validated against JUCE **8.0.13** (the pinned tag).
* License: JUCE is dual-licensed (GPLv3 / commercial). This project as written
  is GPL-compatible; **if you ship a closed-source binary you need a JUCE
  commercial licence.** The build does **not** define `JUCE_DISPLAY_SPLASH_SCREEN`
  — the only JUCE config definitions in `CMakeLists.txt` are `JUCE_WEB_BROWSER=0`,
  `JUCE_USE_CURL=0`, and `JUCE_VST3_CAN_REPLACE_VST2=0`, none of which touch the
  splash/licence terms. (JUCE 8.0.13 no longer renders a splash screen at all.)
* Used: `juce_audio_processors` (plugin wrapper, APVTS, parameters),
  `juce_dsp` (`juce::dsp::FFT`), `juce_gui_basics`/`juce_graphics` (UI),
  `juce::ScopedNoDenormals`, `juce::SmoothedValue`, `juce::Thread`,
  `juce::Random`.

## Algorithms — original implementations, externally inspired
No third-party source was copied. The following ideas were studied as
*references* (per the brief) and re-implemented from scratch:

* **Onset / segmentation** — spectral-flux onset detection with an adaptive mean
  threshold, plus RMS gating and silence-close, inspired conceptually by
  **aubio**'s onset/feature approach. No aubio code is used.
* **Spectral features** — centroid (brightness) and spectral flatness
  (noisiness) computed directly from the JUCE FFT magnitude spectrum.
* **Pitch** — plain time-domain autocorrelation with a confidence ratio (not
  aubio's YIN; see limitations).
* **Long-buffer transformation philosophy** (freeze / smear / time manipulation
  of long fragments) was informed by looking at **PaulXStretch**'s problem space,
  but the implementations here (slab arena, allpass diffuser smear, grain cloud)
  are independent and intentionally lightweight.
* **Plugin/GUI structure** follows the standard **JUCE** plugin examples and
  `AudioProcessorValueTreeState` patterns. **iPlug2** and **DPF** were consulted
  only as comparative references for lightweight architecture; no code derived.

## DSP building blocks (textbook, implemented here)
* 4-point 3rd-order **Hermite interpolation** for varispeed.
* One-pole low/high-pass filters for archival band-limiting.
* Sample-and-hold + quantisation **bit/rate aging**.
* Schroeder **allpass** chain for the global diffusion/smear.
* Hann-windowed **grain** scheduler for granular cloud expansion.
* Lock-free **SPSC ring queue** and **triple buffer** (dirty-bit index swap).

If you add any GPL/MIT/other-licensed source later, record it here with its
licence and attribution before shipping.
