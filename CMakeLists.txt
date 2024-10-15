cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 99)

# Force static runtime on MSVC.
if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG_INIT "/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1")
    set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT "/MT /O1 /Ob1 /D NDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE_INIT "/MT /O2 /Ob2 /D NDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "/MT /Zi /O2 /Ob1 /D NDEBUG")
endif()

project(ToneFilter-VST LANGUAGES CXX C)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(VST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VST_SDK")

add_library(vst-sdk STATIC
    ${VST_DIR}/public.sdk/source/vst2.x/audioeffect.cpp
    ${VST_DIR}/public.sdk/source/vst2.x/audioeffectx.cpp
    ${VST_DIR}/public.sdk/source/vst2.x/vstplugmain.cpp)
target_include_directories(vst-sdk PUBLIC ${VST_DIR}/public.sdk/source/vst2.x ${VST_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

if (APPLE)
    set(build-type MODULE)
else()
    set(build-type SHARED)
endif()

add_library(ToneFilterVST ${build-type}
    simd_headers.hpp
    dsp.hpp
    pole_zero_filter_design.cpp
    pole_zero_filter_design.hpp
    aligned_alloc.hpp
    aligned_alloc.cpp
    tone_filter.cpp
    tone_filter.hpp
    vst_plugin.cpp
    vst_plugin.hpp)

target_include_directories(ToneFilterVST PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ToneFilterVST PRIVATE vst-sdk)

if (APPLE OR CMAKE_COMPILER_IS_GNUCXX)
	target_compile_options(ToneFilterVST PRIVATE -Wall -Wextra -Wno-unused-parameter -fvisibility=hidden)
	target_compile_options(vst-sdk PRIVATE -Wno-unused-parameter -fvisibility=hidden)
elseif (MSVC)
    target_compile_options(ToneFilterVST PRIVATE /D_CRT_SECURE_NO_WARNINGS /wd4267 /wd4244 /MP /DNOMINMAX)
endif()

target_compile_definitions(ToneFilterVST PRIVATE "ASSETS=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/\"")
set_target_properties(ToneFilterVST PROPERTIES PREFIX "" OUTPUT_NAME "ToneFilterVST")

if (WIN32 AND CMAKE_COMPILER_IS_GNUCXX)
	set_target_properties(ToneFilterVST PROPERTIES LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/link.T -static")
elseif (APPLE)
    set_target_properties(ToneFilterVST PROPERTIES BUNDLE TRUE)
    set_target_properties(ToneFilterVST PROPERTIES BUNDLE_EXTENSION "vst")
    set_target_properties(ToneFilterVST PROPERTIES LINK_FLAGS "-Wl,-all_load")
endif()
