cmake_minimum_required(VERSION 3.22)
project(OrcaPlugin VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(OrcaPlugin
    COMPANY_NAME "Orca"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE Orca
    PLUGIN_CODE Orka
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Orca"
    BUNDLE_ID "com.orca.plugin"
    AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
)

target_sources(OrcaPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/engine/Types.h
        Source/engine/Grid.h
        Source/engine/Grid.cpp
        Source/engine/Operator.h
        Source/engine/Operator.cpp
        Source/engine/Transpose.h
        Source/engine/MidiStack.h
        Source/engine/CcStack.h
        Source/engine/MonoStack.h
        Source/engine/EngineIO.h
        Source/engine/Engine.h
)

target_include_directories(OrcaPlugin
    PRIVATE
        Source
)

target_compile_definitions(OrcaPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

find_library(COREMIDI_FRAMEWORK CoreMIDI)
find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)

target_link_libraries(OrcaPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_basics
        ${COREMIDI_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
