// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once

#include <utility>
#include "Environment.hpp"

#ifdef PXC_BUILD_DEBUG
#define PXC_DEBUG_REPORT_HANDLE(message)                                                                          \
	(void)((1 != _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, nullptr, "%s", message)) || (_CrtDbgBreak(), 0)); \
	__fastfail(5);                                                                                                \
	std::unreachable();
#else
#define PXC_DEBUG_REPORT_HANDLE(message)
#endif

#define PXC_DEBUG_CHECK(cond,message)               \
	if (!(cond))                                    \
	{                                               \
		PXC_DEBUG_REPORT_HANDLE(message);           \
		std::unreachable();                         \
	}

#define PXC_DEBUG_TERMINATE(message);               \
	PXC_DEBUG_REPORT_HANDLE(message)                \