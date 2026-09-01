cmake_minimum_required(VERSION 3.20)
project(Simple106 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Universal Binary support for macOS (Apple Silicon + Intel)
if (APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)
endif()

# 1. Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
)
FetchContent_MakeAvailable(JUCE)

# Determine Plugin Formats per OS
set(PLUGIN_FORMATS VST3 Standalone)
if (APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()

# 2. Define Plugin Target
juce_add_plugin(Simple106
    COMPANY_NAME "Fadedlimes"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE Fade
    PLUGIN_CODE S106
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Simple106"
)

# 3. Source Files
target_sources(Simple106 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/UI/SilverLookAndFeel.h
    Source/UI/VintageButtonKeyboard.h
    Source/DSP/PolyBLEPOscillator.h
    Source/DSP/JunoFilter.h
    Source/DSP/ADSREnvelope.h
    Source/DSP/LFO.h
    Source/DSP/StereoChorus.h
    Source/DSP/StereoDelay.h
    Source/DSP/StereoReverb.h
    Source/DSP/SequencerEngine.h
    Source/DSP/SynthVoice.h
    Source/DSP/VoiceManager.h
)

# 4. Include Folders
target_include_directories(Simple106 PRIVATE Source Source/DSP Source/UI)

# 5. Compiler Flags (Safe check across GCC, Clang, and MSVC)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Simple106 PRIVATE -Wno-calloc-transposed-args -Wno-error)
endif()

# Public Definitions (Inherited by VST3, AU, and Standalone targets)
target_compile_definitions(Simple106 PUBLIC
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# 6. Link JUCE Modules
target_link_libraries(Simple106 PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
)
