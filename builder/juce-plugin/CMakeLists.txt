cmake_minimum_required(VERSION 3.15)

project(HtmlToVstPlugin VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Expect JUCE to be cloned into JUCE/ inside this folder by the workflow
add_subdirectory(JUCE)

juce_add_plugin(HtmlToVstPlugin
    COMPANY_NAME "HTMLToVST"
    BUNDLE_ID    "com.htmltovst.plugin"
    IS_SYNTH     FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE

    PLUGIN_MANUFACTURER_CODE HTVS
    PLUGIN_CODE H2VT

    FORMATS VST3
    PRODUCT_NAME "HTML to VST Plugin"
)

target_sources(HtmlToVstPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(HtmlToVstPlugin
    PRIVATE
        JUCE_WEB_BROWSER=1           # enables WebBrowserComponent
        JUCE_USE_CURL=0
)

target_link_libraries(HtmlToVstPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra
        juce::juce_gui_basics
        juce::juce_audio_basics
        juce::juce_core
)
