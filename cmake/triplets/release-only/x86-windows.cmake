# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
# https://github.com/Pen-NineCat

# CI-only variant of the built-in x86-windows triplet: release libraries only, so the LLVM port is
# built once instead of twice. VCPKG_BUILD_TYPE is a triplet variable, an environment variable of
# the same name is ignored. Selected with -DVCPKG_OVERLAY_TRIPLETS=cmake/triplets/release-only.
set(VCPKG_TARGET_ARCHITECTURE x86)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_PROVIDED_FORTRAN ON)
set(VCPKG_BUILD_TYPE release)
