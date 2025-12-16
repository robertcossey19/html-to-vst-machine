cmake_minimum_required(VERSION 3.15)
project(HtmlToVstPlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

# ===============================
# UI → BinaryData (FIXED)
# ===============================
set(UI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Assets/UI)

if(NOT EXISTS "${UI_DIR}/ampex_ui.html")
    message(FATAL_ERROR "UI load failed at build-time: missing ${UI_DIR}/ampex_ui.html")
endif()

juce_add_binary_data(HtmlUIData
    SOURCES
        ${UI_DIR}/ampex_ui.html
)

# ===============================
# Plugin
# ===============================
juce_add_plugin(HtmlToVstPlugin
    COMPANY_NAME "AnalogExact"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Lbir
    FORMATS VST3
    PRODUCT_NAME "HTML to VST Plugin"
)

target_sources(HtmlToVstPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_link_libraries(HtmlToVstPlugin PRIVATE
    juce::juce_audio_utils
    HtmlUIData
)

juce_generate_juce_header(HtmlToVstPlugin)
