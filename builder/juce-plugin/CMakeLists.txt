cmake_minimum_required(VERSION 3.15)

project(HtmlToVstPlugin VERSION 1.0.0 LANGUAGES C CXX)

# Keep things predictable across CI/local
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# JUCE is vendored in this repo under builder/juce-plugin/JUCE
add_subdirectory(JUCE)

# ============================================================
# UI ASSETS -> JUCE BinaryData
#
# The macOS CI failure you hit ("Unhandled exception") occurs in the
# HtmlUIData BinaryData generation script phase. In practice this happens
# when the input file list is empty, or when it explodes (e.g. includes
# node_modules).
#
# This section:
#   - searches common UI build output dirs (dist/build/public)
#   - filters node_modules/.git/etc
#   - if nothing found, creates fallback HTML at configure time
# ============================================================

set(HTML_UI_DIR "" CACHE PATH "Optional path to built UI assets to embed (e.g. .../dist or .../build)")

set(_ui_roots "")

# If caller provides a UI dir, prefer it
if (HTML_UI_DIR AND EXISTS "${HTML_UI_DIR}" AND IS_DIRECTORY "${HTML_UI_DIR}")
  list(APPEND _ui_roots "${HTML_UI_DIR}")
endif()

# Common repo/template locations (add more without breaking existing setups)
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
  "${CMAKE_CURRENT_SOURCE_DIR}/html"
  "${CMAKE_CURRENT_SOURCE_DIR}/html/dist"
  "${CMAKE_CURRENT_SOURCE_DIR}/html/build"
  "${CMAKE_CURRENT_SOURCE_DIR}/web"
  "${CMAKE_CURRENT_SOURCE_DIR}/web/dist"
  "${CMAKE_CURRENT_SOURCE_DIR}/web/build"
  "${CMAKE_CURRENT_SOURCE_DIR}/resources/ui"
  "${CMAKE_CURRENT_SOURCE_DIR}/Resources/ui"
  "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui"
  "${CMAKE_CURRENT_SOURCE_DIR}/Assets/ui"
)

# Auto-detect top-level folders containing "ui" in their name (safe, shallow)
file(GLOB _auto_ui_dirs LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/*ui*")
foreach(d IN LISTS _auto_ui_dirs)
  if (EXISTS "${d}" AND IS_DIRECTORY "${d}")
    list(APPEND _ui_roots "${d}")
    if (EXISTS "${d}/dist" AND IS_DIRECTORY "${d}/dist")
      list(APPEND _ui_roots "${d}/dist")
    endif()
    if (EXISTS "${d}/build" AND IS_DIRECTORY "${d}/build")
      list(APPEND _ui_roots "${d}/build")
    endif()
    if (EXISTS "${d}/public" AND IS_DIRECTORY "${d}/public")
      list(APPEND _ui_roots "${d}/public")
    endif()
  endif()
endforeach()

list(REMOVE_DUPLICATES _ui_roots)

set(UI_FILES "")

foreach(dir IN LISTS _ui_roots)
  if (EXISTS "${dir}" AND IS_DIRECTORY "${dir}")
    # Only embed typical web build artifacts (avoid vacuuming dev folders)
    file(GLOB_RECURSE _found CONFIGURE_DEPENDS
      "${dir}/*.html"
      "${dir}/*.htm"
      "${dir}/*.css"
      "${dir}/*.js"
      "${dir}/*.mjs"
      "${dir}/*.cjs"
      "${dir}/*.json"
      "${dir}/*.map"
      "${dir}/*.svg"
      "${dir}/*.png"
      "${dir}/*.jpg"
      "${dir}/*.jpeg"
      "${dir}/*.gif"
      "${dir}/*.webp"
      "${dir}/*.ico"
      "${dir}/*.wasm"
      "${dir}/*.txt"
      "${dir}/*.woff"
      "${dir}/*.woff2"
      "${dir}/*.ttf"
      "${dir}/*.otf"
      "${dir}/*.eot"
    )
    list(APPEND UI_FILES ${_found})
  endif()
endforeach()

list(REMOVE_DUPLICATES UI_FILES)

# Strip the usual "this will blow up BinaryData" suspects
if (UI_FILES)
  list(FILTER UI_FILES EXCLUDE REGEX "/node_modules/")
  list(FILTER UI_FILES EXCLUDE REGEX "/\\.git/")
  list(FILTER UI_FILES EXCLUDE REGEX "/CMakeFiles/")
  list(FILTER UI_FILES EXCLUDE REGEX "/cmake%-build/")
  list(FILTER UI_FILES EXCLUDE REGEX "/build/")      # generic build dirs
  list(FILTER UI_FILES EXCLUDE REGEX "/JUCE/")       # never embed JUCE itself
endif()

# Absolute last resort: create fallback UI so CI never passes an empty list
if (NOT UI_FILES)
  message(WARNING "No UI assets found for HtmlUIData. Creating fallback HTML so build can continue.")
  set(_fallback_dir "${CMAKE_CURRENT_BINARY_DIR}/ui_fallback")
  file(MAKE_DIRECTORY "${_fallback_dir}")

  file(WRITE "${_fallback_dir}/index.html"
"<!doctype html>
<html lang='en'>
<head>
  <meta charset='utf-8'/>
  <meta name='viewport' content='width=device-width, initial-scale=1'/>
  <title>HTML to VST Plugin</title>
</head>
<body style='font-family:-apple-system,system-ui,Segoe UI,Roboto,Helvetica,Arial,sans-serif;padding:16px;'>
  <h2>HTML to VST Plugin</h2>
  <p><b>Fallback UI</b>: no embedded web assets were found at configure time.</p>
  <p>Fix by adding a built UI folder (e.g. <code>ui/dist</code>) or configuring CMake with:</p>
  <pre>-DHTML_UI_DIR=/absolute/path/to/dist</pre>
</body>
</html>
")

  file(WRITE "${_fallback_dir}/main.html"
"<!doctype html><meta charset='utf-8'><title>HTML to VST Plugin</title><p>Fallback UI</p>"
)

  list(APPEND UI_FILES
    "${_fallback_dir}/index.html"
    "${_fallback_dir}/main.html"
  )
endif()

# This is the target that was failing in your log
juce_add_binary_data(HtmlUIData SOURCES ${UI_FILES})

# ============================================================
# PLUGIN
# ============================================================

juce_add_plugin(HtmlToVstPlugin
  COMPANY_NAME "AnalogExact"
  COMPANY_WEBSITE ""
  COMPANY_EMAIL ""

  # These match the values shown in your build log
  PLUGIN_MANUFACTURER_CODE Manu
  PLUGIN_CODE Lbir

  # Only build VST3 (matches your log)
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

# Include all project sources without you having to keep a list
set(_source_roots
  "${CMAKE_CURRENT_SOURCE_DIR}/Source"
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/source"
)

set(PLUGIN_SOURCES "")
foreach(root IN LISTS _source_roots)
  if (EXISTS "${root}" AND IS_DIRECTORY "${root}")
    file(GLOB_RECURSE _src CONFIGURE_DEPENDS
      "${root}/*.cpp"
      "${root}/*.c"
      "${root}/*.cc"
      "${root}/*.cxx"
      "${root}/*.mm"
      "${root}/*.m"
      "${root}/*.h"
      "${root}/*.hpp"
    )
    list(APPEND PLUGIN_SOURCES ${_src})
  endif()
endforeach()

list(REMOVE_DUPLICATES PLUGIN_SOURCES)
target_sources(HtmlToVstPlugin PRIVATE ${PLUGIN_SOURCES})

# Match the compile defs shown in your log
target_compile_definitions(HtmlToVstPlugin
  PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_FORCE_USE_LEGACY_PARAM_IDS=1
)

target_link_libraries(HtmlToVstPlugin
  PRIVATE
    HtmlUIData
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_extra
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
    juce::juce_recommended_lto_flags
)
