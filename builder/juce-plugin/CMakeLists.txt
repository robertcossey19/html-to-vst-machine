cmake_minimum_required(VERSION 3.15)

project(HtmlToVstPlugin VERSION 1.0.0)

add_subdirectory(JUCE)

# Embed the HTML UI
juce_add_binary_data(HtmlUIData
    SOURCES
        Assets/UI/ampex_ui.html
)

juce_add_plugin(HtmlToVstPlugin
    COMPANY_NAME "AnalogExact"
    BUNDLE_ID    "com.analogexact.htmltovst"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS VST3
    PRODUCT_NAME "HTML to VST Plugin"
)

target_sources(HtmlToVstPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_link_libraries(HtmlToVstPlugin PRIVATE
    HtmlUIData
    juce::juce_dsp
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_basics
    juce::juce_gui_basics
    juce::juce_gui_extra
)
