cmake_minimum_required(VERSION 3.24)
# Windows: link the MSVC runtime statically, so no Visual C++ Redistributable
# has to be installed on the user's PC.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(ArtilodySampleforge VERSION 0.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(SAMPLEFORGE_DSP_ONLY "Build only portable audio-engine tests" OFF)
include(CTest)
if(BUILD_TESTING)
    add_executable(SampleforgeDspTests Tests/dsp_test.cpp)
    if(MSVC)
        target_compile_options(SampleforgeDspTests PRIVATE /UNDEBUG)
    else()
        target_compile_options(SampleforgeDspTests PRIVATE -UNDEBUG)
    endif()
    add_test(NAME SampleforgeDsp COMMAND SampleforgeDspTests)
endif()
if(SAMPLEFORGE_DSP_ONLY)
    return()
endif()
set(JUCE_PATH "" CACHE PATH "Optional existing JUCE 8.0.4 source checkout")
if(JUCE_PATH)
    add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/juce")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        URL https://codeload.github.com/juce-framework/JUCE/tar.gz/refs/tags/8.0.4
        URL_HASH SHA256=0effe9823a6fd8a4f98e5c08be7d308350e417b2d15dcf34011e778fefbbf005
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(JUCE)
    # JUCE 8.0.4: this pointer is used only as a condition. Keep the call and
    # its null check, without declaring an unused variable (AppleClang).
    if(APPLE)
        set(peer_file "${juce_SOURCE_DIR}/modules/juce_gui_basics/native/juce_NSViewComponentPeer_mac.mm")
        file(READ "${peer_file}" peer_source)
        set(old_condition "if (auto* target = findCurrentTextInputTarget())\n                if (const auto* inputContext = [view inputContext])")
        set(new_condition "if (findCurrentTextInputTarget() != nullptr)\n                if (const auto* inputContext = [view inputContext])")
        string(REPLACE "${old_condition}" "${new_condition}" patched_peer "${peer_source}")
        if(NOT patched_peer STREQUAL peer_source)
            file(WRITE "${peer_file}" "${patched_peer}")
        endif()
    endif()
endif()
juce_add_gui_app(Sampleforge
    PRODUCT_NAME "Artilody Sampleforge"
    COMPANY_NAME "Artilody"
    BUNDLE_ID "dk.artilody.sampleforge"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon.png")
juce_add_binary_data(SampleforgeAssets SOURCES Assets/icon.svg)
target_sources(Sampleforge PRIVATE Source/Main.cpp Source/Dsp.h Source/Sfz.h)
target_compile_definitions(Sampleforge PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
find_package(Threads REQUIRED)
target_link_libraries(Sampleforge PRIVATE
    SampleforgeAssets juce::juce_audio_utils juce::juce_gui_extra Threads::Threads
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
