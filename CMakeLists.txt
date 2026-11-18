# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
# https://github.com/Pen-NineCat

cmake_minimum_required (VERSION 4.0)
project ("Pxc")

# Presets only pass the architecture by name, the effective target is selected by the toolchain
# (vcvarsall on Windows, -m32 on Linux). Fail early instead of silently building another one.
if(DEFINED PXC_CMAKE_ARCHITECTURE)
    if(PXC_CMAKE_ARCHITECTURE STREQUAL "x86")
        set(expectedPointerSize 4)
    else()
        set(expectedPointerSize 8)
    endif()

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL expectedPointerSize)
        message(FATAL_ERROR "PXC_CMAKE_ARCHITECTURE is '${PXC_CMAKE_ARCHITECTURE}' but the compiler targets ${CMAKE_SIZEOF_VOID_P}-byte pointers, check the vcvarsall (Windows) or multilib (Linux) environment.")
    endif()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PXC_CMAKE_ENABLE_ASAN "Set ON to build the Debug configuration with AddressSanitizer(MSVC ONLY)." OFF)
option(PXC_CMAKE_ENABLE_TESTS "Set ON to build tests." OFF)
option(PXC_CMAKE_ENABLE_SEGMENT_HEAP "Set ON to use the Windows segment heap(MSVC ONLY)." OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # /fsanitize=address has to reach both the compiler and the linker, and ASAN does not work with
    # incremental linking. $<CONFIG:Debug> keeps the flags attached to the configuration instead of
    # to CMAKE_BUILD_TYPE, which stays empty for multi-config generators.
    if(PXC_CMAKE_ENABLE_ASAN)
        message(STATUS "Enable ASAN")
        add_compile_options($<$<CONFIG:Debug>:/fsanitize=address>)
        add_link_options($<$<CONFIG:Debug>:/fsanitize=address> $<$<CONFIG:Debug>:/INCREMENTAL:NO>)
        # Reserved for the runtime side of the sanitizer hookup, no source reads it yet.
        add_compile_definitions($<$<CONFIG:Debug>:PXC_ENABLE_ASAN>)
    else()
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
    endif()

    # MSVC Segment Heap support
    # MinGW doesn't have this script, so we can't just use CMakePresets.json to check, because it can only check OS, not the complier.
    if(PXC_CMAKE_ENABLE_SEGMENT_HEAP)
        set(segmentHeapScript "$ENV{VSINSTALLDIR}/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/Microsoft/SegmentHeap.cmake")
        if(NOT EXISTS "${segmentHeapScript}")
            message(FATAL_ERROR "${segmentHeapScript} not found, set VSINSTALLDIR to the Visual Studio installation directory that contains Common7.")
        endif()
        # The script defers a scan of every executable target, so including it is enough.
        include("${segmentHeapScript}")
    endif()

    add_compile_options(/utf-8) 
    add_compile_options(/Zc:preprocessor)
    add_compile_options(/diagnostics:caret)
    add_compile_options(/FC)
else()
    if(PXC_CMAKE_ENABLE_ASAN)
        message(WARNING "PXC_CMAKE_ENABLE_ASAN is only implemented for MSVC, ignoring it.")
    endif()

    if(PXC_CMAKE_ENABLE_SEGMENT_HEAP)
        message(WARNING "PXC_CMAKE_ENABLE_SEGMENT_HEAP is only implemented for MSVC, ignoring it.")
    endif()
endif()

add_subdirectory(Code/PxcCommon)
add_subdirectory(Code/PLang)

if(PXC_CMAKE_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(Code/Tests)
endif()
