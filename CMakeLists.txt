cmake_minimum_required(VERSION 3.22)

project(ArpSequencer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Fetch JUCE via FetchContent (no local clone required) ───────────────────
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.9
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ─── Plugin target ───────────────────────────────────────────────────────────
juce_add_plugin(ArpSequencer
    VERSION                     "1.0.0"
    COMPANY_NAME                "MannyZeneke"
    COMPANY_WEBSITE             "https://github.com/Mxnny88/mannyzeneke"
    PLUGIN_MANUFACTURER_CODE    MnZk
    PLUGIN_CODE                 ArSq
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "ArpSequencer"
    BUNDLE_ID                   "com.mannyzeneke.arpsequencer"
    PLUGIN_NAME                 "ArpSequencer"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             "Instrument" "Fx" "MIDI"
)

# ─── Source files ────────────────────────────────────────────────────────────
target_sources(ArpSequencer
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/ArpeggiatorEngine.cpp
        Source/StepSequencer.cpp
        Source/PresetManager.cpp
)

# ─── Compile definitions ─────────────────────────────────────────────────────
target_compile_definitions(ArpSequencer
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
)

# ─── Link libraries ──────────────────────────────────────────────────────────
target_link_libraries(ArpSequencer
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_gui_extra
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ─── Generate JuceHeader.h (required for #include <JuceHeader.h>) ────────────
juce_generate_juce_header(ArpSequencer)

# ─── Include directories ─────────────────────────────────────────────────────
target_include_directories(ArpSequencer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# ─── Unit tests (optional standalone executable) ─────────────────────────────
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()

    juce_add_console_app(ArpSequencerTests
        PRODUCT_NAME "ArpSequencerTests"
    )
    target_sources(ArpSequencerTests
        PRIVATE
            Source/Tests/TestMain.cpp
            Source/ArpeggiatorEngine.cpp
            Source/StepSequencer.cpp
    )
    target_compile_definitions(ArpSequencerTests
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_DISPLAY_SPLASH_SCREEN=0
            JUCE_REPORT_APP_USAGE=0
            JUCE_STRICT_REFCOUNTEDPOINTER=1
        PRIVATE
            JUCE_UNIT_TESTS=1
    )
    target_link_libraries(ArpSequencerTests
        PRIVATE
            juce::juce_audio_basics
            juce::juce_audio_processors
            juce::juce_core
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )
    juce_generate_juce_header(ArpSequencerTests)

    target_include_directories(ArpSequencerTests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )
    add_test(NAME ArpSequencerUnitTests COMMAND ArpSequencerTests)
endif()
