cmake_minimum_required(VERSION 3.15)

project(HtmlToVstPlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# JUCE lives in builder/juce-plugin/JUCE
add_subdirectory(JUCE)

# Bundle your HTML UI into JUCE BinaryData target
# (path is relative to builder/juce-plugin)
juce_add_binary_data(HtmlUIData
    SOURCES
        ../ui/index.html
)

# Main plugin target
juce_add_plugin(HtmlToVstPlugin
    COMPANY_NAME "AnalogExact"
    BUNDLE_ID "com.analogexact.htmltovst"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Lbir
    PRODUCT_NAME "HTML to VST Plugin"
    FORMATS VST3
)

# Generate JuceHeader.h (CI can otherwise fail finding it)
juce_generate_juce_header(HtmlToVstPlugin)

target_sources(HtmlToVstPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_link_libraries(HtmlToVstPlugin
    PRIVATE
        HtmlUIData
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
)

# ---- CRITICAL FIXES ----
# 1) Prevent JUCE from trying to build VST2 replacement/wrapper bits inside VST3
#    (avoids JUCE compile-time #error about VST2/VST3 automation conflicts)
# 2) Keep stable/legacy param IDs for automation consistency
target_compile_definitions(HtmlToVstPlugin
    PRIVATE
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_FORCE_USE_LEGACY_PARAM_IDS=1
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# ---- HARDEN "JuceHeader.h not found" ----
# Explicitly add the generated JuceHeader.h location to include paths.
# This makes the build robust even if an IDE/CI compile step happens before
# the include dirs are fully inferred.
target_include_directories(HtmlToVstPlugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_BINARY_DIR}/HtmlToVstPlugin_artefacts/JuceLibraryCode
)
