cmake_minimum_required(VERSION 3.15)

project(HtmlToVstPlugin VERSION 1.0.0)

# ------------------------------------------------------------------
# JUCE configuration – turn OFF AAX for this build
# ------------------------------------------------------------------
# These cache vars are picked up by the JUCE CMake scripts
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_PIP_EXAMPLES OFF CACHE BOOL "" FORCE)

# Most important line: tells JUCE *not* to build the AAX client code
set(JUCE_BUILD_AAX OFF CACHE BOOL "" FORCE)
set(JUCE_ENABLE_AAX OFF CACHE BOOL "" FORCE)

add_subdirectory(JUCE)

# ------------------------------------------------------------------
# Embed your HTML UI as BinaryData
# ------------------------------------------------------------------
juce_add_binary_data(HtmlUIData
    SOURCES
        Assets/UI/ampex_ui.html
)

# ------------------------------------------------------------------
# VST3 plugin definition (no AAX, no AU, etc.)
# ------------------------------------------------------------------
juce_add_plugin(HtmlToVstPlugin
    COMPANY_NAME               "AnalogExact"
    BUNDLE_ID                  "com.analogexact.htmltovst"
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD    TRUE

    # Only build VST3 here – AAX is *not* requested
    FORMATS                    VST3

    PRODUCT_NAME               "HTML to VST Plugin"
)

target_sources(HtmlToVstPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_link_libraries(HtmlToVstPlugin PRIVATE
    HtmlUIData
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
)
