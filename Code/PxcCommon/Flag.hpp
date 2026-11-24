// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once
#include <type_traits>

#define PXC_BIT_ENUM(bit) (1 << (bit - 1))

#define PXC_DECL_ENUM_FLAG_OPERATORS(EnumType)                                               \
	constexpr Pxc::Flag<EnumType> operator|(EnumType lhs, EnumType rhs) noexcept             \
	{                                                                                        \
		return Pxc::Flag<EnumType>(lhs) | rhs;                                               \
	}                                                                                        \
	constexpr Pxc::Flag<EnumType> operator&(EnumType lhs, EnumType rhs) noexcept             \
	{                                                                                        \
		return Pxc::Flag<EnumType>(lhs) & rhs;                                               \
	}                                                                                        \
	constexpr Pxc::Flag<EnumType> operator^(EnumType lhs, EnumType rhs) noexcept             \
	{                                                                                        \
		return Pxc::Flag<EnumType>(lhs) ^ rhs;                                               \
	}                                                                                        \
	constexpr Pxc::Flag<EnumType> operator|(EnumType lhs, Pxc::Flag<EnumType> rhs) noexcept  \
	{                                                                                        \
		return rhs | lhs;                                                                    \
	}                                                                                        \
	constexpr Pxc::Flag<EnumType> operator&(EnumType lhs, Pxc::Flag<EnumType> rhs) noexcept  \
	{                                                                                        \
		return rhs & lhs;                                                                    \
	}                                                                                        \
	constexpr Pxc::Flag<EnumType> operator^(EnumType lhs, Pxc::Flag<EnumType> rhs) noexcept  \
	{                                                                                        \
		return rhs ^ lhs;                                                                    \
	}                                                                                        \
	constexpr Pxc::Flag<EnumType> operator~(EnumType value) noexcept                         \
	{                                                                                        \
		return static_cast<EnumType>(~static_cast<std::underlying_type_t<EnumType>>(value)); \
	}                                                                                        \
	constexpr bool operator!(EnumType value) noexcept                                        \
	{                                                                                        \
		return static_cast<std::underlying_type_t<EnumType>>(value) == 0;                    \
	}
#define PXC_DECL_ENUM_FLAG_TYPE(EnumType)       \
	using EnumType##Flag = Pxc::Flag<EnumType>; \
	PXC_DECL_ENUM_FLAG_OPERATORS(EnumType)

#define PXC_DECL_ENUM_FLAG_TYPE_WITH_NAME(EnumType, FlagType) \
	using FlagType = Pxc::Flag<EnumType>;                     \
	PXC_DECL_ENUM_FLAG_OPERATORS(EnumType)

#define PXC_DECL_ENUM_FLAG_FRIEND_OPERATORS(EnumType)                                              \
	friend constexpr Pxc::Flag<EnumType> operator|(EnumType lhs, EnumType rhs) noexcept            \
	{                                                                                              \
		return Pxc::Flag<EnumType>(lhs) | rhs;                                                     \
	}                                                                                              \
	friend constexpr Pxc::Flag<EnumType> operator&(EnumType lhs, EnumType rhs) noexcept            \
	{                                                                                              \
		return Pxc::Flag<EnumType>(lhs) & rhs;                                                     \
	}                                                                                              \
	friend constexpr Pxc::Flag<EnumType> operator^(EnumType lhs, EnumType rhs) noexcept            \
	{                                                                                              \
		return Pxc::Flag<EnumType>(lhs) ^ rhs;                                                     \
	}                                                                                              \
	friend constexpr Pxc::Flag<EnumType> operator|(EnumType lhs, Pxc::Flag<EnumType> rhs) noexcept \
	{                                                                                              \
		return rhs | lhs;                                                                          \
	}                                                                                              \
	friend constexpr Pxc::Flag<EnumType> operator&(EnumType lhs, Pxc::Flag<EnumType> rhs) noexcept \
	{                                                                                              \
		return rhs & lhs;                                                                          \
	}                                                                                              \
	friend constexpr Pxc::Flag<EnumType> operator^(EnumType lhs, Pxc::Flag<EnumType> rhs) noexcept \
	{                                                                                              \
		return rhs ^ lhs;                                                                          \
	}                                                                                              \
	friend constexpr Pxc::Flag<EnumType> operator~(EnumType value) noexcept                        \
	{                                                                                              \
		return static_cast<EnumType>(~static_cast<std::underlying_type_t<EnumType>>(value));       \
	}                                                                                              \
	friend constexpr bool operator!(EnumType value) noexcept                                       \
	{                                                                                              \
		return static_cast<std::underlying_type_t<EnumType>>(value) == 0;                          \
	}
#define PXC_DECL_ENUM_FLAG_FRIEND_TYPE(EnumType) \
	using EnumType##Flag = Flag<EnumType>;       \
	PXC_DECL_ENUM_FLAG_FRIEND_OPERATORS(EnumType)

#define PXC_DECL_ENUM_FLAG_FRIEND_TYPE_WITH_NAME(EnumType, FlagType) \
	using FlagType = Flag<EnumType>;                                 \
	PXC_DECL_ENUM_FLAG_FRIEND_OPERATORS(EnumType)

namespace Pxc
{
	template <typename EnumType>
	    requires std::is_enum_v<EnumType>
	class Flag
	{
	public:
		using EnumUnderlyingType = std::underlying_type_t<EnumType>;

		constexpr Flag() noexcept = default;
		constexpr Flag(const Flag&) noexcept = default;
		constexpr Flag(Flag&&) noexcept = default;
		constexpr Flag& operator=(const Flag&) noexcept = default;
		constexpr Flag& operator=(Flag&&) noexcept = default;
		constexpr ~Flag() noexcept = default;

		/* implicit */ constexpr Flag(EnumType flag) noexcept : m_value(flag)
		{
		}

		constexpr Flag& operator=(EnumType flag) noexcept
		{
			m_value = flag;
			return *this;
		}

		constexpr Flag operator|(EnumType flag) const noexcept
		{
			return Flag(static_cast<EnumUnderlyingType>(m_value) | static_cast<EnumUnderlyingType>(flag));
		}

		constexpr Flag operator|(Flag other) const noexcept
		{
			return Flag(static_cast<EnumUnderlyingType>(m_value) | static_cast<EnumUnderlyingType>(other.m_value));
		}

		constexpr Flag& operator|=(EnumType flag) noexcept
		{
			m_value = static_cast<EnumType>(static_cast<EnumUnderlyingType>(m_value) | static_cast<EnumUnderlyingType>(flag));
			return *this;
		}

		constexpr Flag& operator|=(Flag other) noexcept
		{
			m_value = static_cast<EnumType>(static_cast<EnumUnderlyingType>(m_value) | static_cast<EnumUnderlyingType>(other.m_value));
			return *this;
		}

		constexpr Flag operator&(EnumType flag) const noexcept
		{
			return Flag(static_cast<EnumType>(static_cast<EnumUnderlyingType>(m_value) & static_cast<EnumUnderlyingType>(flag)));
		}

		constexpr Flag operator&(Flag other) const noexcept
		{
			return Flag(static_cast<EnumUnderlyingType>(m_value) & static_cast<EnumUnderlyingType>(other.m_value));
		}

		constexpr Flag& operator&=(EnumType flag) noexcept
		{
			m_value = static_cast<EnumType>(static_cast<EnumUnderlyingType>(m_value) & static_cast<EnumUnderlyingType>(flag));
			return *this;
		}

		constexpr Flag& operator&=(Flag other) noexcept
		{
			m_value = static_cast<EnumType>(static_cast<EnumUnderlyingType>(m_value) & static_cast<EnumUnderlyingType>(other.m_value));
			return *this;
		}

		constexpr Flag operator^(EnumType flag) const noexcept
		{
			return Flag(static_cast<EnumUnderlyingType>(m_value) ^ static_cast<EnumUnderlyingType>(flag));
		}

		constexpr Flag operator^(Flag other) const noexcept
		{
			return Flag(static_cast<EnumUnderlyingType>(m_value) ^ static_cast<EnumUnderlyingType>(other.m_value));
		}

		constexpr Flag& operator^=(EnumType flag) noexcept
		{
			m_value = static_cast<EnumType>(static_cast<EnumUnderlyingType>(m_value) ^ static_cast<EnumUnderlyingType>(flag));
			return *this;
		}

		constexpr Flag& operator^=(Flag other) noexcept
		{
			m_value = static_cast<EnumType>(static_cast<EnumUnderlyingType>(m_value) ^ static_cast<EnumUnderlyingType>(other.m_value));
			return *this;
		}

		constexpr Flag operator~() const noexcept
		{
			return Flag(~static_cast<EnumUnderlyingType>(m_value));
		}

		constexpr bool Test(EnumType flag) const noexcept
		{
			return (static_cast<EnumUnderlyingType>(m_value) & static_cast<EnumUnderlyingType>(flag)) != 0;
		}

		constexpr bool TestAll(Flag flag) const noexcept
		{
			return (static_cast<EnumUnderlyingType>(m_value) & static_cast<EnumUnderlyingType>(flag.m_value)) == static_cast<EnumUnderlyingType>(flag.m_value);
		}

		constexpr bool Any() const noexcept
		{
			return static_cast<EnumUnderlyingType>(m_value) != 0;
		}

		constexpr bool operator==(const Flag&) const noexcept = default;
		constexpr bool operator==(EnumType flag) const noexcept
		{
			return static_cast<EnumUnderlyingType>(m_value) == static_cast<EnumUnderlyingType>(flag);
		}

		constexpr EnumType EnumValue() const noexcept
		{
			return m_value;
		}

		explicit operator bool() const noexcept
		{
			return Any();
		}

	private:
		explicit constexpr Flag(EnumUnderlyingType value) noexcept : m_value(static_cast<EnumType>(value))
		{
		}

		EnumType m_value = EnumType(0);
	};
} // namespace Pxc
