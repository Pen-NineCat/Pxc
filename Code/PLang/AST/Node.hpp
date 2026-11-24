// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once
#include "PLang/Common/SourceView.hpp"

#include <memory>

namespace Pxc::PLang
{
	/// @brief One enumerator per concrete syntax node.
	/// @note Kept flat so dispatch is a switch on Kind; no RTTI is needed.
	enum class NodeKind : U16
	{
		// unit level declarations
		Module,
		ModuleDecl,
		ImportDecl,
		FriendModuleDecl,
		AliasDecl,

		// type syntax
		Type,
	};

	struct Expr;
	struct Stmt;
	struct Decl;
	struct TypeRef;
	struct Module;

	using ExprPtr = std::unique_ptr<Expr>;
	using StmtPtr = std::unique_ptr<Stmt>;
	using DeclPtr = std::unique_ptr<Decl>;
	using TypePtr = std::unique_ptr<TypeRef>;

	/// @brief Base of every syntax node: its kind and its source range.
	struct Node
	{
		Node(NodeKind kind, SourceViewRange view) : Kind(kind), View(view)
		{
		}

		virtual ~Node() = default;

		NodeKind Kind;
		SourceViewRange View;
	};

	struct Expr : Node
	{
		using Node::Node;
	};

	struct Stmt : Node
	{
		using Node::Node;
	};

	struct Decl : Node
	{
		using Node::Node;
	};
} // namespace Pxc::PLang
