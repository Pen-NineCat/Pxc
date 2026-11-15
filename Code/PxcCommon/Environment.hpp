// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once

#ifdef _WIN32
#define PXC_OS_WIN32
#endif

#ifdef _WIN64
#define PXC_OS_WIN64
#endif

#if defined(__APPLE__) and defined(__MACH__)
#include <TargetConditionals.h>

// iOS (Simulator or Device)
#if TARGET_OS_IPHONE
#define PXC_OS_IOS
// macOS
#elif TARGET_OS_MAC
#define PXC_OS_MACOS
#else
#error "Unknown Apple platform"
#endif
#endif

#if defined(__linux__) or defined(__linux)
#define PXC_OS_LINUX
#endif

#ifdef __ANDROID__
#define PXC_OS_ANDROID
#endif

#ifdef _MSC_VER
#define PXC_COMPILER_MSVC
#endif

#if defined(__GNUC__) and !defined(__clang__)
#define PXC_COMPILER_GCC
#endif

#ifdef __clang__
#define PXC_COMPILER_CLANG
#endif

#if defined(_M_IX86) or defined(__i386__)
#define PXC_ARCH_X86
#define PXC_ARCH_32BIT
#elif defined(_M_X64) or defined(__x86_64__)
#define PXC_ARCH_X64
#define PXC_ARCH_64BIT
#endif

#if defined(_M_ARM) or defined(__arm__)
#define PXC_ARCH_ARM
#define PXC_ARCH_32BIT
#elif defined(_M_ARM64) or defined(__aarch64__)
#define PXC_ARCH_ARM64
#define PXC_ARCH_64BIT
#endif

#ifdef __riscv
#define PXC_ARCH_RISCV
#endif

#if defined(PXC_ARCH_X86) or defined(PXC_ARCH_X64)
#define PXC_INTRINSIC_SSE
#endif

#if defined(PXC_ARCH_ARM) or defined(PXC_ARCH_ARM64)
#define PXC_INTRINSIC_NEON
#endif

#if defined(_DEBUG) or defined(DEBUG)
#define PXC_BUILD_DEBUG
#endif

#if defined(NODEBUG) or defined(NDEBUG)
#define PXC_BUILD_RELEASE
#endif

#ifdef PXC_COMPILER_MSVC
#define PXC_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#define PXC_NO_VTABLE __declspec(novtable)
#define PXC_FORCE_INLINE __forceinline
#else
#define PXC_NO_UNIQUE_ADDRESS [[no_unique_address]]
#define PXC_NO_VTABLE
#define PXC_FORCE_INLINE __attribute__((always_inline))
#endif // PXC_COMPILER_MSVC

#ifdef PXC_OS_WIN32
#define PXC_EXPORT_API __declspec(dllexport)
#define PXC_IMPORT_API __declspec(dllimport)
#else
#define PXC_EXPORT_API __attribute__((visibility("default")))
#define PXC_IMPORT_API __attribute__((visibility("default")))
#endif // PXC_OS_WIN32

#ifdef PXC_SUPPORT_CONSTEXPR_EXCEPTION
#define PXC_CONSTEXPR_EXCEPTION_FUNCTION constexpr
#else
#define PXC_CONSTEXPR_EXCEPTION_FUNCTION
#endif // PXC_CONSTEXPR_EXCEPTION_FUNCTION

#define PXC_STL_DEPRECATED_IF_false(message)
#define PXC_STL_DEPRECATED_IF_true(message) [[deprecated(message)]]

#define PXC_STL_DEPRECATED_PICK_(cond, message) \
	PXC_STL_DEPRECATED_IF_##cond(message)

#define PXC_STL_DEPRECATED_PICK(cond, message) \
	PXC_STL_DEPRECATED_PICK_(cond, message)

#ifdef PXC_COMPILER_MSVC
/// @brief 检查对象在不同的编译器上是否应该被标记为deprecated
/// @param msvc 对象在MSVC编译器上是否应该被标记为deprecated
/// @param clang 对象在Clang编译器上是否应该被标记为deprecated
/// @param gcc 对象在GCC编译器上是否应该被标记为deprecated
/// @param message deprecated消息
#define PXC_STL_DEPRECATED(msvc, clang, gcc, message) \
	PXC_STL_DEPRECATED_PICK(msvc, message)
#elifdef PXC_COMPILER_CLANG
/// @brief 检查对象在不同的编译器上是否应该被标记为deprecated
/// @param msvc 对象在MSVC编译器上是否应该被标记为deprecated
/// @param clang 对象在Clang编译器上是否应该被标记为deprecated
/// @param gcc 对象在GCC编译器上是否应该被标记为deprecated
/// @param message deprecated消息
#define PXC_STL_DEPRECATED(msvc, clang, gcc, message) \
	PXC_STL_DEPRECATED_PICK(clang, message)
#elifdef PXC_COMPILER_GCC
/// @brief 检查对象在不同的编译器上是否应该被标记为deprecated
/// @param msvc 对象在MSVC编译器上是否应该被标记为deprecated
/// @param clang 对象在Clang编译器上是否应该被标记为deprecated
/// @param gcc 对象在GCC编译器上是否应该被标记为deprecated
/// @param message deprecated消息
#define PXC_STL_DEPRECATED(msvc, clang, gcc, message) \
	PXC_STL_DEPRECATED_PICK(gcc, message)
#endif

#include <cstddef>
#include <cstdint>

namespace Pxc
{
	using Byte = uint8_t;
	using U8 = uint8_t;
	using U16 = uint16_t;
	using U32 = uint32_t;
	using U64 = uint64_t;
	using I8 = int8_t;
	using I16 = int16_t;
	using I32 = int32_t;
	using I64 = int64_t;
	using Usize = size_t;
	using Isize = ptrdiff_t;

	constexpr static Usize BitsPerBytes = 8;
} // namespace Pxc