// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once

#include "Environment.hpp"

#include <string>
#include <unordered_map>

namespace Pxc
{
	struct TranHash
	{
		using is_transparent = void;

		static Usize operator()(const std::string& str)
		{
			return std::hash<std::string>{}(str);
		}

		static Usize operator()(std::string_view str)
		{
			return std::hash<std::string_view>{}(str);
		}

		static Usize operator()(const char* str)
		{
			return std::hash<std::string_view>{}(str);
		}
	};

	template <typename V>
	using StrUnorderedMap = std::unordered_map<std::string, V, TranHash, std::equal_to<>>;

	template <typename V>
	using StrUnorderedMultiMap = std::unordered_multimap<std::string, V, TranHash, std::equal_to<>>;
} // namespace Pxc