cmake_minimum_required(VERSION 3.15)

project(HtmlToVstPlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# JUCE is vendored here
add_subdirectory(JUCE)

# ---------------------------
# HTML UI BinaryData (FIXED PATH)
# ---------------------------
# CMakeLists.txt lives in:  builder/juce-plugin
# Your UI lives in:         ui/
# So the correct relative path is ../../ui/index.html
set(UI_INDEX_HTML "${CMAKE_CURRENT_LIST_DIR}/../../ui/index.html")

if (NOT EXISTS "${UI_INDEX_HTML}")
    message(FATAL_ERROR "UI file not found: ${UI_INDEX_HTML} (BinaryData generation would crash)")
endif()

juce_add_binary_data(HtmlUIData
    SOURCES
        "${UI_INDEX_HTML}"
)

# ---------------------------
# Plugin target
# ---------------------------
juce_add_plugin(HtmlToVstPlugin
    COMPANY_NAME "AnalogExact"
    BUNDLE_ID "com.analogexact.htmltovst"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Lbir
    PRODUCT_NAME "HTML to VST Plugin"
    FORMATS VST3
)

# Make sure JuceHeader.h is generated
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

# ---------------------------
# IMPORTANT compile definitions
# ---------------------------
# Keep stable automation IDs and prevent JUCE VST3 wrapper from acting like VST2 replacement.
target_compile_definitions(HtmlToVstPlugin
    PRIVATE
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_FORCE_USE_LEGACY_PARAM_IDS=1
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# ---------------------------
# Harden include paths (JuceHeader.h)
# ---------------------------
target_include_directories(HtmlToVstPlugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_BINARY_DIR}/HtmlToVstPlugin_artefacts/JuceLibraryCode
)
