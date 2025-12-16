cmake_minimum_required(VERSION 3.22)

project(HtmlToVstPlugin VERSION 1.0.0 LANGUAGES C CXX)

# =========================
# Basic build settings
# =========================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# When configuring from: cmake -B build -S builder/juce-plugin
# repo root is two levels up
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# =========================
# JUCE (use bundled JUCE if present; otherwise fetch)
# =========================
set(JUCE_DIR "${CMAKE_CURRENT_LIST_DIR}/JUCE")

if (EXISTS "${JUCE_DIR}/CMakeLists.txt")
  message(STATUS "Using bundled JUCE at: ${JUCE_DIR}")
  add_subdirectory("${JUCE_DIR}" JUCE)
else()
  message(STATUS "Bundled JUCE not found; fetching JUCE via FetchContent")
  include(FetchContent)
  FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
  )
  FetchContent_MakeAvailable(juce)
endif()

# =========================
# Locate UI entry (index.html)
# =========================
set(UI_INDEX "")
set(UI_CANDIDATES
  "${REPO_ROOT}/ui/index.html"
  "${REPO_ROOT}/UI/index.html"
  "${REPO_ROOT}/frontend/index.html"
  "${REPO_ROOT}/web/index.html"
  "${REPO_ROOT}/app/index.html"
  "${REPO_ROOT}/public/index.html"
)

foreach(p IN LISTS UI_CANDIDATES)
  if (EXISTS "${p}")
    set(UI_INDEX "${p}")
    break()
  endif()
endforeach()

# Fallback: search the repo for index.html (but avoid common junk dirs)
if (UI_INDEX STREQUAL "")
  file(GLOB_RECURSE FOUND_INDEX_HTML
    LIST_DIRECTORIES false
    "${REPO_ROOT}/*index.html"
  )

  # Prefer paths containing /ui/ or /UI/
  foreach(p IN LISTS FOUND_INDEX_HTML)
    string(TOLOWER "${p}" pl)
    if (pl MATCHES "/ui/index\\.html$")
      set(UI_INDEX "${p}")
      break()
    endif()
  endforeach()

  # If still not found, just take the first index.html we found
  if (UI_INDEX STREQUAL "" AND FOUND_INDEX_HTML)
    list(GET FOUND_INDEX_HTML 0 UI_INDEX)
  endif()
endif()

if (UI_INDEX STREQUAL "")
  message(FATAL_ERROR
    "UI file not found.\n"
    "Expected something like:\n"
    "  ${REPO_ROOT}/ui/index.html\n"
    "Create that file (or put an index.html in a UI folder), then re-run CMake.\n"
    "This prevents BinaryData generation from crashing."
  )
endif()

get_filename_component(UI_DIR "${UI_INDEX}" DIRECTORY)
message(STATUS "Using UI entry: ${UI_INDEX}")
message(STATUS "Using UI dir:   ${UI_DIR}")

# Collect all UI assets so BinaryData includes everything (html/css/js/images/fonts)
# NOTE: If your UI folder is huge, keep it lean (don’t include node_modules here).
file(GLOB_RECURSE UI_ASSETS
  LIST_DIRECTORIES false
  "${UI_DIR}/*.*"
)

# Filter out obvious junk folders if they exist under UI_DIR
set(UI_ASSETS_FILTERED "")
foreach(f IN LISTS UI_ASSETS)
  string(TOLOWER "${f}" fl)
  if (NOT fl MATCHES "/node_modules/" AND
      NOT fl MATCHES "/\\.git/" AND
      NOT fl MATCHES "/dist/" AND
      NOT fl MATCHES "/build/")
    list(APPEND UI_ASSETS_FILTERED "${f}")
  endif()
endforeach()

# =========================
# BinaryData target for UI
# =========================
juce_add_binary_data(HtmlUIData SOURCES
  ${UI_ASSETS_FILTERED}
)

# =========================
# Plugin target
# =========================
# If you only want VST3, keep FORMATS VST3 (no VST2 anywhere).
juce_add_plugin(HtmlToVstPlugin
  COMPANY_NAME            "AnalogExact"
  BUNDLE_ID               "com.analogexact.htmltovst"
  PRODUCT_NAME            "HTMLtoVST"
  PLUGIN_MANUFACTURER_CODE "Manu"
  PLUGIN_CODE             "Lbir"

  IS_SYNTH                FALSE
  NEEDS_MIDI_INPUT        FALSE
  NEEDS_MIDI_OUTPUT       FALSE
  IS_MIDI_EFFECT          FALSE

  FORMATS                 VST3
)

# Source files: use explicit list if you prefer; this keeps it “drop-in” friendly.
target_sources(HtmlToVstPlugin PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/Source/PluginProcessor.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/Source/PluginProcessor.h"
  "${CMAKE_CURRENT_LIST_DIR}/Source/PluginEditor.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/Source/PluginEditor.h"
)

target_link_libraries(HtmlToVstPlugin PRIVATE
  HtmlUIData

  juce::juce_audio_utils
  juce::juce_audio_processors
  juce::juce_gui_extra
  juce::juce_gui_basics
  juce::juce_graphics
  juce::juce_events
  juce::juce_data_structures
  juce::juce_core

  juce::juce_recommended_config_flags
  juce::juce_recommended_lto_flags
  juce::juce_recommended_warning_flags
)

# =========================
# Fixes for your specific build errors
# =========================
target_compile_definitions(HtmlToVstPlugin PRIVATE
  JUCE_WEB_BROWSER=1
  JUCE_USE_CURL=0

  # Fixes the JUCE VST2/VST3 param automation conflict error
  JUCE_FORCE_USE_LEGACY_PARAM_IDS=1
  JUCE_VST3_CAN_REPLACE_VST2=0
)

# Make sure generated headers are found reliably
# (JuceHeader.h lives in the artefacts tree; BinaryData headers live in BinaryData build dir)
target_include_directories(HtmlToVstPlugin PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/HtmlToVstPlugin_artefacts/JuceLibraryCode"
  "${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_HtmlUIData/JuceLibraryCode"
)

# Some JUCE projects like this on macOS
set_target_properties(HtmlToVstPlugin PROPERTIES
  MACOSX_BUNDLE TRUE
)
