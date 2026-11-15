// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once
#include "../Common/SourceView.hpp"
#include "DiagnosticCodeTable.hpp"
#include <string>
#include <variant>
#include <vector>

namespace Pxc::PLang
{
	enum class DiagnosticLevel
	{
		Critical, // Must terminate compiling, and will convert to error at output.
		Error,
		Warning,
		Note,
	};

	using DiagnosticArgsVector = std::vector<std::variant<std::monostate, I64, U64, std::string, SourceView>>;

	template <typename T>
	auto NormalizeDiagnosticArgs(T&& v)
	{
		using U = std::decay_t<T>;
		if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*> ||
		              std::is_same_v<U, std::string_view>)
		{
			return std::string(std::forward<T>(v));
		}
		else if constexpr (std::is_same_v<U, char>)
		{
			return std::string(1, std::forward<T>(v));
		}
		else
		{
			return std::forward<T>(v);
		}
	}

	struct DiagnosticLabel
	{
		DiagnosticLevel Level = DiagnosticLevel::Error;
		DiagnosticCode Code;
		DiagnosticArgsVector Args;
		SourceView Source;
	};

	using DiagnosticLabelList = std::vector<DiagnosticLabel>;
} // namespace Pxc::PLang