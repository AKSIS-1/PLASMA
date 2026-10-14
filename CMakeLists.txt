cmake_minimum_required(VERSION 3.22)
project(Plasma VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Static MSVC runtime so users need no Visual C++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(PLASMA_BUILD_PLUGIN "Build the VST3 plugin (needs JUCE)" ON)
option(PLASMA_BUILD_TESTS "Build the headless DSP tests" ON)

# ---- DSP core (header only, framework independent)
add_library(plasma_core INTERFACE)
target_include_directories(plasma_core INTERFACE core)
if(NOT MSVC)
  target_compile_options(plasma_core INTERFACE -Wall -Wextra -Wno-unused-parameter)
endif()

if(PLASMA_BUILD_TESTS)
  add_executable(plasma_tests tests/core_tests.cpp)
  target_link_libraries(plasma_tests PRIVATE plasma_core)
  enable_testing()
  add_test(NAME core COMMAND plasma_tests)
endif()

if(PLASMA_BUILD_PLUGIN)
  include(FetchContent)
  if(DEFINED ENV{JUCE_DIR_OVERRIDE})
    add_subdirectory($ENV{JUCE_DIR_OVERRIDE} juce EXCLUDE_FROM_ALL)
  else()
    FetchContent_Declare(juce
      GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
      GIT_TAG 8.0.4
      GIT_SHALLOW ON)
    FetchContent_MakeAvailable(juce)
  endif()

  juce_add_plugin(Plasma
    COMPANY_NAME "Plasma"
    PLUGIN_MANUFACTURER_CODE Plsm
    PLUGIN_CODE Plas
    FORMATS VST3 Standalone
    PRODUCT_NAME "Plasma"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth)

  juce_add_binary_data(PlasmaAssets SOURCES
    assets/fonts/BarlowSemiCondensed-SemiBold.ttf
    assets/fonts/BarlowSemiCondensed-Medium.ttf
    assets/fonts/IBMPlexMono-Regular.ttf
    assets/fonts/IBMPlexMono-Medium.ttf
    assets/fonts/VarelaRound-Regular.ttf)

  target_sources(Plasma PRIVATE
    plugin/PluginProcessor.cpp
    plugin/PluginEditor.cpp)
  target_compile_definitions(Plasma PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
  target_link_libraries(Plasma
    PRIVATE plasma_core PlasmaAssets juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
