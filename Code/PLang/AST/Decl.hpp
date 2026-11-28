// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once
#include "Node.hpp"
#include "PxcCommon/Flag.hpp"
#include "Type.hpp"

#include <memory>
#include <string_view>
#include <vector>

namespace Pxc::PLang
{
	/// @brief `module A.B.C;` directive.
	struct ModuleDecl : Decl
	{
		explicit ModuleDecl(SourceView view) : Decl(NodeKind::ModuleDecl, SourceViewRange::Point(view))
		{
		}

		std::vector<std::string_view> Path;
	};

	/// @brief One entry of a `public import ... with { A as B }` list.
	struct ImportItem
	{
		SourceViewRange View;
		std::string_view Name;
		std::string_view Alias; // empty when the item is not forwarded under another name
	};

	/// @brief Import directive; `public` and `using` are independent flags, so
	///        `public using import` carries both.
	struct ImportDecl : Decl
	{
		enum class Form : U8
		{
			Import = 1 << 0,
			UsingImport = 1 << 1,
			PublicImport = 1 << 2,
		};

		PXC_DECL_ENUM_FLAG_FRIEND_TYPE(Form);

		explicit ImportDecl(SourceView view) : Decl(NodeKind::ImportDecl, SourceViewRange::Point(view))
		{
		}

		FormFlag ImportForm = Form::Import;
		std::vector<std::string_view> Path;
		bool IsWildcard = false; // import M.*
		bool HasWithClause = false;
		std::vector<ImportItem> Items;
	};

	/// @brief `friend module A.B.C;` directive.
	struct FriendModuleDecl : Decl
	{
		explicit FriendModuleDecl(SourceView view) : Decl(NodeKind::FriendModuleDecl, SourceViewRange::Point(view))
		{
		}

		std::vector<std::string_view> Path;
	};

	/// @brief Type alias: `using Type as Name;`.
	struct AliasDecl : Decl
	{
		explicit AliasDecl(SourceView view) : Decl(NodeKind::AliasDecl, SourceViewRange::Point(view))
		{
		}

		TypePtr Target;
		std::string_view Name;
	};

	/// @brief Whole module unit.
	struct Module : Node
	{
		explicit Module(SourceView view) : Node(NodeKind::Module, SourceViewRange::Point(view))
		{
		}

		std::unique_ptr<ModuleDecl> Directive;
		std::vector<DeclPtr> Declarations;
	};
} // namespace Pxc::PLang
