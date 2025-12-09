cmake_minimum_required(VERSION 3.15)

project(HtmlToVstPlugin VERSION 1.0.0)

# JUCE setup
add_subdirectory(JUCE)

# =====================================================================
# EMBEDDED HTML UI (Option B)
# =====================================================================

juce_add_binary_data(HtmlToVst_UI
    SOURCES
        Assets/UI/ampex_ui.html
)

# =====================================================================
# PLUGIN DEFINITION
# =====================================================================

juce_add_plugin(HtmlToVstPlugin
    COMPANY_NAME "ANALOGEXACT"
    PLUGIN_MANUFACTURER_CODE ANEX
    PLUGIN_CODE HTVP
    FORMATS VST3
    PRODUCT_NAME "HTML to VST Plugin"
    VST3_CATEGORIES "Fx"
)

# =====================================================================
# SOURCE FILES
# =====================================================================

target_sources(HtmlToVstPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/GeneratedParams.h
)

# =====================================================================
# LINK LIBRARIES
# =====================================================================

target_link_libraries(HtmlToVstPlugin PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_dsp
    juce::juce_gui_extra
    HtmlToVst_UI
)

# =====================================================================
# NEEDED DEFINITIONS
# =====================================================================

target_compile_definitions(HtmlToVstPlugin PRIVATE
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)
