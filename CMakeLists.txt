cmake_minimum_required(VERSION 3.22)
# Must come before project(), which creates these cache entries
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

project(OPTO2 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(OPTO2
    COMPANY_NAME "gueibrielon"
    BUNDLE_ID com.gueibrielon.opto2
    PLUGIN_MANUFACTURER_CODE Guei
    PLUGIN_CODE Opt2
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "OPTO-2"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)

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
