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

# IMPORTANT:
# This generates JuceHeader.h in the JUCE CMake workflow.
# Without this, CI can fail with: fatal error: 'JuceHeader.h' file not found
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

target_compile_definitions(HtmlToVstPlugin
    PRIVATE
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0

        # Keep stable/legacy param IDs (helps automation stability)
        JUCE_FORCE_USE_LEGACY_PARAM_IDS=1

        # CRITICAL FIX:
        # Prevent JUCE from compiling VST2 wrapper code into the VST3 target.
        # This avoids the JUCE compile-time #error about VST2/VST3 automation conflicts.
        JUCE_VST3_CAN_REPLACE_VST2=0
)
