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

	/// @brief Source range covering [Offset(), EndOffset()).
	/// @note `First` is the first token of the construct and `Last` is its last token, so the
	///       range ends after the last token's own length. Both keep their start line/column.
	class SourceViewRange
	{
	public:
		constexpr SourceViewRange() noexcept = default;

		constexpr SourceViewRange(SourceView first, SourceView last) noexcept : m_first(first), m_last(last)
		{
		}

		/// @brief Range that covers exactly the given token.
		static constexpr SourceViewRange Point(SourceView view) noexcept
		{
			return SourceViewRange{ view, view };
		}

		constexpr const SourceView& First() const noexcept
		{
			return m_first;
		}

		constexpr const SourceView& Last() const noexcept
		{
			return m_last;
		}

		/// @brief Offset of the first byte in the range.
		constexpr Usize Offset() const noexcept
		{
			return m_first.Offset;
		}

		/// @brief Offset of the first byte after the range.
		constexpr Usize EndOffset() const noexcept
		{
			return m_last.Offset + m_last.Length;
		}

		/// @brief Number of bytes covered by the range.
		constexpr Usize Length() const noexcept
		{
			return EndOffset() - Offset();
		}

		constexpr bool Empty() const noexcept
		{
			return Length() == 0;
		}

	private:
		SourceView m_first{};
		SourceView m_last{};
	};
} // namespace Pxc::PLang
