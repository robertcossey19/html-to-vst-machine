cmake_minimum_required(VERSION 3.15)
project(HtmlToVstPlugin VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# JUCE is vendored in this repo under builder/juce-plugin/JUCE
add_subdirectory(JUCE)

# ============================================================
# UI ASSETS -> JUCE BinaryData
# ============================================================
set(HTML_UI_DIR "" CACHE PATH "Optional path to built UI assets to embed (e.g. .../dist or .../build)")

set(_ui_roots "")
if (HTML_UI_DIR AND EXISTS "${HTML_UI_DIR}" AND IS_DIRECTORY "${HTML_UI_DIR}")
  list(APPEND _ui_roots "${HTML_UI_DIR}")
endif()

# Common locations (keep broad but safe)
list(APPEND _ui_roots
  "${CMAKE_CURRENT_SOURCE_DIR}/ui"
  "${CMAKE_CURRENT_SOURCE_DIR}/ui/dist"
  "${CMAKE_CURRENT_SOURCE_DIR}/ui/build"
  "${CMAKE_CURRENT_SOURCE_DIR}/html-ui"
  "${CMAKE_CURRENT_SOURCE_DIR}/html-ui/dist"
  "${CMAKE_CURRENT_SOURCE_DIR}/html-ui/build"
  "${CMAKE_CURRENT_SOURCE_DIR}/web-ui"
  "${CMAKE_CURRENT_SOURCE_DIR}/web-ui/dist"
  "${CMAKE_CURRENT_SOURCE_DIR}/web-ui/build"
  "${CMAKE_CURRENT_SOURCE_DIR}/resources/ui"
  "${CMAKE_CURRENT_SOURCE_DIR}/Resources/ui"
  "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui"
  "${CMAKE_CURRENT_SOURCE_DIR}/Assets/ui"
)

list(REMOVE_DUPLICATES _ui_roots)

set(UI_FILES "")
foreach(dir IN LISTS _ui_roots)
  if (EXISTS "${dir}" AND IS_DIRECTORY "${dir}")
    file(GLOB_RECURSE _found CONFIGURE_DEPENDS
      "${dir}/*.html" "${dir}/*.htm"
      "${dir}/*.css"  "${dir}/*.js" "${dir}/*.mjs" "${dir}/*.cjs"
      "${dir}/*.json" "${dir}/*.map"
      "${dir}/*.svg"  "${dir}/*.png" "${dir}/*.jpg" "${dir}/*.jpeg" "${dir}/*.gif" "${dir}/*.webp" "${dir}/*.ico"
      "${dir}/*.wasm" "${dir}/*.txt"
      "${dir}/*.woff" "${dir}/*.woff2" "${dir}/*.ttf" "${dir}/*.otf" "${dir}/*.eot"
    )
    list(APPEND UI_FILES ${_found})
  endif()
endforeach()

list(REMOVE_DUPLICATES UI_FILES)

# Filter out "death" folders
if (UI_FILES)
  list(FILTER UI_FILES EXCLUDE REGEX "/node_modules/")
  list(FILTER UI_FILES EXCLUDE REGEX "/\\.git/")
  list(FILTER UI_FILES EXCLUDE REGEX "/CMakeFiles/")
  list(FILTER UI_FILES EXCLUDE REGEX "/cmake%-build/")
  list(FILTER UI_FILES EXCLUDE REGEX "/JUCE/")
endif()

# Fallback HTML so BinaryData is never empty
if (NOT UI_FILES)
  message(WARNING "No UI assets found. Creating fallback HTML so build can continue.")
  set(_fallback_dir "${CMAKE_CURRENT_BINARY_DIR}/ui_fallback")
  file(MAKE_DIRECTORY "${_fallback_dir}")
  file(WRITE "${_fallback_dir}/index.html" "<!doctype html><html><head><meta charset='utf-8'><title>HTML to VST Plugin</title></head><body style='font-family:system-ui;background:#111;color:#eee'><h2>HTML to VST Plugin</h2><p>Fallback UI: no embedded web assets found.</p><p>Configure CMake with: <code>-DHTML_UI_DIR=/absolute/path/to/dist</code></p></body></html>")
  list(APPEND UI_FILES "${_fallback_dir}/index.html")
endif()

juce_add_binary_data(HtmlUIData SOURCES ${UI_FILES})

# ============================================================
# PLUGIN
# ============================================================
juce_add_plugin(HtmlToVstPlugin
  COMPANY_NAME "AnalogExact"
  COMPANY_WEBSITE ""
  COMPANY_EMAIL ""
  PLUGIN_MANUFACTURER_CODE Manu
  PLUGIN_CODE Lbir
  FORMATS VST3
  PRODUCT_NAME "HTML to VST Plugin"
  BUNDLE_ID "com.analogexact.htmltovst"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  VST3_CATEGORIES "Fx"
)

juce_generate_juce_header(HtmlToVstPlugin)

# IMPORTANT: Only compile from Source/ (NOT source/, src/, etc.)
file(GLOB_RECURSE PLUGIN_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.mm"
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.hpp"
)
target_sources(HtmlToVstPlugin PRIVATE ${PLUGIN_SOURCES})

target_compile_definitions(HtmlToVstPlugin PUBLIC
  JUCE_WEB_BROWSER=1
  JUCE_USE_CURL=0
  JUCE_FORCE_USE_LEGACY_PARAM_IDS=1
)

target_link_libraries(HtmlToVstPlugin PRIVATE
  HtmlUIData
  juce::juce_audio_processors
  juce::juce_audio_utils
  juce::juce_gui_extra
  PUBLIC
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags
  juce::juce_recommended_lto_flags
)
