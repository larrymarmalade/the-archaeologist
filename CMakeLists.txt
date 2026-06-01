cmake_minimum_required(VERSION 3.22)

# ─────────────────────────────────────────────────────────────────────────────
#  The Archaeologist — a generative resampler / memory instrument (VST3)
#
#  Build:
#    cmake -B build -DCMAKE_BUILD_TYPE=Release
#    cmake --build build --config Release -j
#
#  JUCE is pulled automatically via FetchContent (pinned tag below) unless you
#  point JUCE_SOURCE_DIR at a local checkout to avoid the download.
# ─────────────────────────────────────────────────────────────────────────────

# macOS: build a universal (fat) binary by default. These MUST be set BEFORE
# project() — otherwise project() creates the (empty) CMAKE_OSX_ARCHITECTURES
# cache entry first and a later set(... CACHE) without FORCE is a silent no-op
# (which produced arm64-only builds). A user override such as
# -DCMAKE_OSX_ARCHITECTURES=arm64 still wins (no FORCE). Ignored off Apple.
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS build architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment target")

project(TheArchaeologist VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ── JUCE ─────────────────────────────────────────────────────────────────────
option(ARCH_USE_LOCAL_JUCE "Use a local JUCE checkout via JUCE_SOURCE_DIR" OFF)

if(ARCH_USE_LOCAL_JUCE)
    if(NOT DEFINED JUCE_SOURCE_DIR)
        message(FATAL_ERROR "ARCH_USE_LOCAL_JUCE=ON requires -DJUCE_SOURCE_DIR=/path/to/JUCE")
    endif()
    add_subdirectory(${JUCE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4          # pinned for reproducibility
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(juce)
endif()

# ── Plugin target ────────────────────────────────────────────────────────────
juce_add_plugin(TheArchaeologist
    COMPANY_NAME            "DeepTime Instruments"
    BUNDLE_ID               com.deeptime.thearchaeologist
    PLUGIN_MANUFACTURER_CODE Dpti
    PLUGIN_CODE             Arch
    FORMATS                 VST3 AU Standalone
    PRODUCT_NAME            "The Archaeologist"
    IS_SYNTH                FALSE          # it processes input audio
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES         "Fx" "Delay"
    AU_MAIN_TYPE            kAudioUnitType_Effect)

juce_generate_juce_header(TheArchaeologist)

target_sources(TheArchaeologist PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp

    source/params/Parameters.cpp

    source/dsp/AnalysisEngine.cpp
    source/dsp/ArtifactStore.cpp
    source/dsp/ExcavationEngine.cpp
    source/dsp/VoiceEngine.cpp
    source/dsp/ArtifactVoice.cpp
    source/dsp/Transformations.cpp

    source/presets/Presets.cpp

    source/ui/ArchaeologistLookAndFeel.cpp
    source/ui/ArchiveMapComponent.cpp
    source/ui/ModeSelector.cpp
    source/ui/MacroPanel.cpp
    source/ui/DetailPanel.cpp
    source/ui/TransportBar.cpp)

target_compile_definitions(TheArchaeologist PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(TheArchaeologist PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

target_include_directories(TheArchaeologist PRIVATE source)

# We rely on IEEE semantics (denormals handled explicitly via ScopedNoDenormals),
# so make sure fast-math is never silently enabled.
if(NOT MSVC)
    target_compile_options(TheArchaeologist PRIVATE -fno-fast-math)
endif()

# ── Tests (host-side, no JUCE plugin host needed) ────────────────────────────
option(ARCH_BUILD_TESTS "Build unit tests" ON)
if(ARCH_BUILD_TESTS)
    enable_testing()
    add_executable(arch_tests
        tests/test_main.cpp
        tests/CircularBufferTests.cpp
        tests/ArtifactStoreTests.cpp
        tests/SelectionTests.cpp
        source/dsp/ArtifactStore.cpp)
    target_include_directories(arch_tests PRIVATE source tests)
    target_compile_features(arch_tests PRIVATE cxx_std_20)
    add_test(NAME arch_tests COMMAND arch_tests)
endif()
