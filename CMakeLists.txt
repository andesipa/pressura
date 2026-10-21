cmake_minimum_required(VERSION 3.22)
# Must come before project(), which creates these cache entries
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

project(OPTO2 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

# Copy the plugins into the system plug-in folders after building (off on CI)
option(PRESSURA_COPY_PLUGINS "Copy AU/VST3 to the plug-in folders after building" ON)

if(APPLE)
    set(PRESSURA_FORMATS AU VST3 Standalone)
else()
    set(PRESSURA_FORMATS VST3 Standalone)
endif()

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(OPTO2
    COMPANY_NAME "gueibrielon"
    BUNDLE_ID com.gueibrielon.pressura
    PLUGIN_MANUFACTURER_CODE Guei
    PLUGIN_CODE Opt2
    FORMATS ${PRESSURA_FORMATS}
    PRODUCT_NAME "Pressura"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${PRESSURA_COPY_PLUGINS})

juce_add_binary_data(OPTO2Data SOURCES Resources/UnifrakturMaguntia.ttf)

target_sources(OPTO2 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(OPTO2 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(OPTO2
    PRIVATE
        OPTO2Data
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Renders the editor to a PNG: cmake --build build --target OPTO2Snapshot
juce_add_console_app(OPTO2Snapshot PRODUCT_NAME "OPTO2Snapshot")
target_sources(OPTO2Snapshot PRIVATE
    Tests/Snapshot.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)
target_compile_definitions(OPTO2Snapshot PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Pressura")
target_link_libraries(OPTO2Snapshot
    PRIVATE OPTO2Data juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags)
