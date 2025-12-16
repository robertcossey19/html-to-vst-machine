cmake_minimum_required(VERSION 3.15)

project(HtmlToVstPlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE lives inside builder/juce-plugin/JUCE in your repo
add_subdirectory(JUCE)

# ---- UI FILE WE EMBED INTO BinaryData ----
set(UI_HTML "${CMAKE_CURRENT_LIST_DIR}/Assets/UI/ampex_ui.html")

if (NOT EXISTS "${UI_HTML}")
    message(FATAL_ERROR
        "UI load failed at build-time: missing\n"
        "  ${UI_HTML}\n\n"
        "Fix: add your UI file at builder/juce-plugin/Assets/UI/ampex_ui.html"
    )
endif()

juce_add_plugin(HtmlToVstPlugin
    COMPANY_NAME "AnalogExact"
    PRODUCT_NAME "HTML to VST Plugin"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Lbir
    FORMATS VST3
)

# ✅ THIS IS WHAT YOUR BUILD IS MISSING RIGHT NOW:
# It generates HtmlToVstPlugin_artefacts/JuceLibraryCode/JuceHeader.h
juce_generate_juce_header(HtmlToVstPlugin)

# BinaryData target that embeds the UI HTML
juce_add_binary_data(HtmlUIData SOURCES "${UI_HTML}")

target_sources(HtmlToVstPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_link_libraries(HtmlToVstPlugin
    PRIVATE
        HtmlUIData
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(HtmlToVstPlugin
    PRIVATE
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
