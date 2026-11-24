# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
# https://github.com/Pen-NineCat

# CI-only variant of the built-in x64-linux triplet: release libraries only, so the LLVM port is
# built once instead of twice. VCPKG_BUILD_TYPE is a triplet variable, an environment variable of
# the same name is ignored. Selected with -DVCPKG_OVERLAY_TRIPLETS=cmake/triplets/release-only.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_BUILD_TYPE release)
