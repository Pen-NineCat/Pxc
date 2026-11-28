// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once
#include "Node.hpp"

#include <string_view>
#include <vector>

namespace Pxc::PLang
{
	/// @brief Type prefix: `[const] [ref] [dyn] base`, or `mov base` alone.
	struct TypeQualifiers
	{
		bool IsConst = false;
		bool IsRef = false;
		bool IsDyn = false;
		bool IsMov = false;
	};

	/// @brief One suffix of a type: `*`, `[]` (view) or `[N]` (inline array).
	struct TypeSuffix
	{
		enum class Form : U8
		{
			Pointer,
			View,
			Array,
		};

		SourceViewRange View;
		Form Kind = Form::Pointer;
		ExprPtr Extent; // only Form::Array
	};

	/// @brief Function parameter; the name may be omitted.
	struct ParamVarDecl
	{
		SourceViewRange View;
		TypePtr Type;
		std::string_view Name;
	};

	/// @brief Type: a parenthesized type, or prefix + base type + suffixes, plus an
	///        optional trailing `?`.
	struct TypeRef : Node
	{
		enum class Base : U8
		{
			Named,        // name [ '<' type-arg-list '>' ]
			Function,     // 'def' '(' [param-list] ')' '->' type
			Auto,         // 'auto' ('dyn auto' carries Dyn in Prefix)
			Parenthesized // '(' type ')'
		};

		explicit TypeRef(SourceView view) : Node(NodeKind::Type, SourceViewRange::Point(view))
		{
		}

		TypeQualifiers Prefix;

		Base BaseKind = Base::Named;
		std::vector<std::string_view> NamePath;   // BaseKind::Named, e.g. Std.IO.Console
		std::vector<TypePtr> Arguments;           // BaseKind::Named
		bool HasArgumentList = false;             // distinguishes Foo from Foo<>
		std::vector<ParamVarDecl> FunctionParams; // BaseKind::Function
		TypePtr FunctionReturn;                   // BaseKind::Function
		TypePtr Operand;                          // BaseKind::Parenthesized

		std::vector<TypeSuffix> Suffixes; // '*' / '[]' / '[N]'
		bool IsNullable = false;          // trailing '?'
	};
} // namespace Pxc::PLang
