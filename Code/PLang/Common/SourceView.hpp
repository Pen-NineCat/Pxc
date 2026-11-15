// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once

#include "Environment.hpp"

namespace Pxc::PLang
{
	struct SourceView
	{
		Usize Line;
		Usize Column;
		Usize Offset;
		Usize Length;
	};
}