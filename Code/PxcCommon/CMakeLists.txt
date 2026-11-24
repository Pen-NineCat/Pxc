# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
# https://github.com/Pen-NineCat

set(PXC_COMMON_SOURCES
	"Environment.hpp"
	"StrUnorderedMap.hpp"
	"Flag.hpp"
)

add_library(Pxc-Common INTERFACE)

target_include_directories(Pxc-Common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})