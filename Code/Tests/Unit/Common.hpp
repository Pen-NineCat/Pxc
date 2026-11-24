// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once
#include "PLang/Diagnostic/Diagnostic.hpp"

#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>

namespace Pxc::PLang::Test
{
	namespace Detail
	{
		inline std::string_view DiagnosticLevelName(DiagnosticLevel level)
		{
			switch (level)
			{
				case DiagnosticLevel::Critical:
					return "critical";
				case DiagnosticLevel::Error:
					return "error";
				case DiagnosticLevel::Warning:
					return "warning";
				case DiagnosticLevel::Note:
					return "note";
			}

			return "unknown";
		}

		inline void AppendDiagnosticArg(std::ostringstream& stream, const std::monostate&)
		{
		}

		inline void AppendDiagnosticArg(std::ostringstream& stream, I64 value)
		{
			stream << value;
		}

		inline void AppendDiagnosticArg(std::ostringstream& stream, U64 value)
		{
			stream << value;
		}

		inline void AppendDiagnosticArg(std::ostringstream& stream, const std::string& value)
		{
			stream << value;
		}

		inline void AppendDiagnosticArg(std::ostringstream& stream, const SourceView& view)
		{
			stream << view.Line << ':' << view.Column;
		}

		/// @brief Render every diagnostic label, one per line, for assertion messages.
		inline std::string FormatDiagnosticLabels(const DiagnosticLabelList& labels)
		{
			std::ostringstream stream;

			for (Usize index = 0; index < labels.size(); ++index)
			{
				const DiagnosticLabel& label = labels[index];

				stream << "\n  label[" << index << "]: level=" << DiagnosticLevelName(label.Level)
				       << ", code=" << static_cast<I32>(label.Code) << ", source=" << label.Source.Line << ':'
				       << label.Source.Column << ", args=(";

				for (Usize argIndex = 0; argIndex < label.Args.size(); ++argIndex)
				{
					if (argIndex != 0)
						stream << ", ";

					std::visit([&stream](const auto& arg)
					           { AppendDiagnosticArg(stream, arg); }, label.Args[argIndex]);
				}

				stream << ')';
			}

			return stream.str();
		}
	} // namespace Detail
} // namespace Pxc::PLang::Test

#define EXPECT_EMPTY_DIAGNOSTIC_LABELS(labels) EXPECT_TRUE((labels).empty()) << Pxc::PLang::Test::Detail::FormatDiagnosticLabels(labels)
