cmake_minimum_required(VERSION 3.22)
project(PluginBrowser VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(FORMATS VST3 AU Standalone)
else()
    set(FORMATS VST3 Standalone)
endif()

if(DEFINED ENV{CI})
    set(COPY_AFTER FALSE)
else()
    set(COPY_AFTER TRUE)
endif()

juce_add_plugin(PluginBrowser
    COMPANY_NAME "MiEstudio"
    PLUGIN_MANUFACTURER_CODE Mest
    PLUGIN_CODE Pbrw
    FORMATS ${FORMATS}
    PRODUCT_NAME "Plugin Browser"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    VST3_CATEGORIES Fx Tools
    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER})

target_sources(PluginBrowser PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(PluginBrowser PUBLIC
    JUCE_PLUGINHOST_VST3=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(APPLE)
    target_compile_definitions(PluginBrowser PUBLIC JUCE_PLUGINHOST_AU=1)
endif()

target_link_libraries(PluginBrowser
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
