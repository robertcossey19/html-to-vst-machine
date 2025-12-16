cmake_minimum_required(VERSION 3.22)

project(HtmlToVstPlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

# -----------------------------
# UI BinaryData target (HtmlUIData)
# -----------------------------
set(UI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ui")

if (NOT EXISTS "${UI_DIR}/index.html")
    message(FATAL_ERROR
        "UI load failed at build-time: missing ${UI_DIR}/index.html\n"
        "Fix: create builder/juce-plugin/ui/index.html (embedded into BinaryData).")
endif()

file(GLOB_RECURSE UI_FILES CONFIGURE_DEPENDS
    "${UI_DIR}/*"
)

# Filter out directories just in case
set(UI_FILE_LIST "")
foreach(f ${UI_FILES})
    if (NOT IS_DIRECTORY "${f}")
        list(APPEND UI_FILE_LIST "${f}")
    endif()
endforeach()

juce_add_binary_data(HtmlUIData SOURCES ${UI_FILE_LIST})

# -----------------------------
# Plugin target
# -----------------------------
juce_add_plugin(HtmlToVstPlugin
    COMPANY_NAME "AnalogExact"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Lbir
    FORMATS VST3
    PRODUCT_NAME "HTML to VST Plugin"
)

target_sources(HtmlToVstPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_link_libraries(HtmlToVstPlugin PRIVATE
    HtmlUIData
    juce::juce_audio_utils
    juce::juce_gui_extra
)

target_compile_definitions(HtmlToVstPlugin PRIVATE
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
)
