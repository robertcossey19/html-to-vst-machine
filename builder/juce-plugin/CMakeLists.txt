cmake_minimum_required(VERSION 3.15)

project(HtmlToVstPlugin VERSION 1.0.0)

add_subdirectory(JUCE)

# ========================
#  HTML UI binary data
# ========================
juce_add_binary_data(HtmlUIData
    SOURCES
        Assets/UI/ampex_ui.html
)

# ========================
#  Plugin target
# ========================
juce_add_plugin(HtmlToVstPlugin
    COMPANY_NAME "AnalogExact"
    BUNDLE_ID    "com.analogexact.htmltovst"

    IS_SYNTH              FALSE
    NEEDS_MIDI_INPUT      FALSE
    NEEDS_MIDI_OUTPUT     FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    # *** IMPORTANT: VST3 ONLY ***
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

# =========================================================
#  FIX: VST2/VST3 parameter automation guard in JUCE
# =========================================================
# JUCE’s juce_audio_plugin_client_VST3.cpp has a safety check:
#   if (JucePlugin_Build_VST && JucePlugin_Build_VST3 && !JUCE_FORCE_USE_LEGACY_PARAM_IDS)
#       #error ...
#
# We ONLY care about VST3 here, and we want the check disabled.
# This define tells JUCE “it’s fine, use the legacy param IDs”.
#
target_compile_definitions(HtmlToVstPlugin
    PRIVATE
        JUCE_FORCE_USE_LEGACY_PARAM_IDS=1

        # Be explicit for safety – VST2 OFF, VST3 ON
        JucePlugin_Build_VST=0
        JucePlugin_Build_VST3=1
)
