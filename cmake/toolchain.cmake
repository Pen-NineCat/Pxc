# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
# https://github.com/Pen-NineCat

# Single toolchain entry point, loaded by CMakePresets.json before the top-level CMakeLists.txt.
# Presets only pass PXC_CMAKE_ARCHITECTURE and the vcpkg toolchain location, everything that has
# to differ between platforms is derived here.

set(PXC_CMAKE_ARCHITECTURE "x64" CACHE STRING "Target architecture, x64 or x86.")
set_property(CACHE PXC_CMAKE_ARCHITECTURE PROPERTY STRINGS x64 x86)

if(NOT PXC_CMAKE_ARCHITECTURE MATCHES "^(x64|x86)$")
    message(FATAL_ERROR "PXC_CMAKE_ARCHITECTURE must be 'x64' or 'x86', got '${PXC_CMAKE_ARCHITECTURE}'.")
endif()

# Where the vcpkg toolchain lives is a cache variable, so CMakeUserPresets.json or
# -DPXC_CMAKE_VCPKG_TOOLCHAIN_FILE=... can point at another checkout without touching the project.
set(PXC_CMAKE_VCPKG_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "vcpkg toolchain file to chain into this toolchain.")
if(NOT EXISTS "${PXC_CMAKE_VCPKG_TOOLCHAIN_FILE}")
    message(FATAL_ERROR "PXC_CMAKE_VCPKG_TOOLCHAIN_FILE='${PXC_CMAKE_VCPKG_TOOLCHAIN_FILE}' does not exist, set VCPKG_ROOT or override PXC_CMAKE_VCPKG_TOOLCHAIN_FILE.")
endif()

# A manifest install uninstalls everything outside the triplet it is running for, so every
# architecture gets its own tree: the trees then stay valid without vcpkg's binary cache. The host
# system is part of the key as well, because the same checkout can be configured from Windows and
# from Linux (WSL) and either run would otherwise uninstall the other one's ports. Every preset of
# one host and architecture shares the tree, which matters a lot while llvm is being built.
set(VCPKG_INSTALLED_DIR "${CMAKE_SOURCE_DIR}/out/vcpkg_installed/${CMAKE_HOST_SYSTEM_NAME}-${PXC_CMAKE_ARCHITECTURE}" CACHE PATH "vcpkg install tree, one per host system and architecture.")
set(VCPKG_INSTALL_OPTIONS "--clean-after-build" CACHE STRING "Extra options for vcpkg install.")

if(CMAKE_HOST_WIN32)
    # MSVC is picked by vcvarsall (x64 or x86) before CMake starts and both CMake and vcpkg read
    # the architecture from that environment, so nothing has to be forced here.
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    # vcpkg ships x64-linux but no x86-linux, project triplets fill the gap.
    set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_LIST_DIR}/triplets" CACHE STRING "vcpkg overlay triplet directories.")
    set(VCPKG_TARGET_TRIPLET "${PXC_CMAKE_ARCHITECTURE}-linux" CACHE STRING "vcpkg target triplet.")
    if(PXC_CMAKE_ARCHITECTURE STREQUAL "x86")
        # gcc and clang build for the 64-bit target by default, -m32 switches compiler and linker.
        # The guard keeps the flag single, CMake evaluates the toolchain more than once per configure.
        foreach(flagsVar CMAKE_C_FLAGS_INIT CMAKE_CXX_FLAGS_INIT CMAKE_EXE_LINKER_FLAGS_INIT CMAKE_SHARED_LINKER_FLAGS_INIT CMAKE_MODULE_LINKER_FLAGS_INIT)
            if(NOT "${${flagsVar}}" MATCHES "(^| )-m32( |$)")
                string(APPEND ${flagsVar} " -m32")
            endif()
        endforeach()
    endif()
else()
    message(FATAL_ERROR "Unsupported host '${CMAKE_HOST_SYSTEM_NAME}', only Windows and Linux are supported.")
endif()

# try_compile re-runs this file in a fresh cache, whose CMAKE_SOURCE_DIR is a throwaway project and
# which would therefore derive a throwaway vcpkg tree. Everything computed above has to be handed
# over so that the compiler tests reuse the real settings.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
    PXC_CMAKE_ARCHITECTURE
    PXC_CMAKE_VCPKG_TOOLCHAIN_FILE
    VCPKG_INSTALLED_DIR
    VCPKG_TARGET_TRIPLET
    VCPKG_OVERLAY_TRIPLETS
)

include("${PXC_CMAKE_VCPKG_TOOLCHAIN_FILE}")
