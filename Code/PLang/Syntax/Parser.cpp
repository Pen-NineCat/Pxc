// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#include "Parser.h"

#include "PLang/Lex/Keywords.hpp"

#include <algorithm>
#include <variant>

namespace
{
	using namespace Pxc::PLang;

	/// @brief Recover the spelling of a token type from the lexer keyword and operator tables.
	/// @note Identifiers, literals and Eof have no fixed spelling; they get a placeholder.
	std::string_view TokenSpelling(TokenType type)
	{
		for (const auto& [text, tokenType] : g_kwMap)
		{
			if (tokenType == type)
				return text;
		}

		for (const auto& [text, tokenType] : BinrayPairList)
		{
			if (tokenType == type)
				return text;
		}

		switch (type)
		{
			case TokenType::Ident:
				return "<identifier>";
			case TokenType::Int:
				return "<integer>";
			case TokenType::Float:
				return "<float>";
			case TokenType::String:
				return "<string>";
			case TokenType::Char:
				return "<character>";
			case TokenType::Eof:
				return "<end of file>";
			default:
				return "<token>";
		}
	}

	/// @brief Picks the string part of a token payload; everything else is empty.
	struct PayloadVisitor
	{
		std::string_view operator()(const std::string& text) const
		{
			return text;
		}

		std::string_view operator()(const std::string_view& text) const
		{
			return text;
		}

		template <typename T>
		std::string_view operator()(const T&) const
		{
			return {};
		}
	};

	/// @brief String payload of an identifier-like token; empty for other payloads.
	std::string_view ExtractPayload(const Token& token)
	{
		return std::visit(PayloadVisitor{}, token.Payload);
	}

	/// @brief Name of the first qualifier already present in a type prefix.
	std::string_view FirstPresentQualifier(const TypeQualifiers& qualifiers)
	{
		if (qualifiers.IsConst)
			return "const";
		if (qualifiers.IsRef)
			return "ref";
		return "dyn";
	}

	/// @brief Tracks how many template argument lists are currently open.
	struct TemplateDepthGuard
	{
		explicit TemplateDepthGuard(Pxc::Usize& depth) : Depth(depth)
		{
			++Depth;
		}

		~TemplateDepthGuard()
		{
			--Depth;
		}

		Pxc::Usize& Depth;
	};
} // namespace

namespace Pxc::PLang
{
	std::unique_ptr<Module> Parser::Parse(std::span<const Token> tokens, DiagnosticLabelList* labels)
	{
		if (tokens.empty() or labels == nullptr)
			return nullptr;

		m_tokens = tokens;
		m_labels = labels;
		m_offset = 0;
		m_pendingGreater = 0;
		m_templateDepth = 0;

		return ParseModule();
	}

	// ============================================================
	// unit level
	// ============================================================

	std::unique_ptr<Module> Parser::ParseModule()
	{
		// unit := [ module-directive ] ( import | friend | declaration )*
		// Header declarations must all precede any other declaration (author ruling).
		const SourceView start = Peek().Source;
		auto module = std::make_unique<Module>(start);

		if (Check(TokenType::K_Module))
			module->Directive = ParseModuleDecl();

		while (IsHeaderDeclStart())
		{
			const Usize before = m_offset;

			DeclPtr decl = Check(TokenType::K_Friend) ? ParseFriendDecl() : ParseImportDecl();
			if (decl != nullptr)
				module->Declarations.push_back(std::move(decl));

			// A malformed header must not spin the loop.
			if (m_offset == before)
				Advance();
		}

		while (not Check(TokenType::Eof))
		{
			const Usize before = m_offset;

			if (Check(TokenType::K_Module))
			{
				ReportError(Peek(), DiagnosticCode::E0100_ModuleMustBeFirst);
				ParseModuleDecl(); // consume the misplaced directive
			}
			else if (IsHeaderDeclStart())
			{
				// Imports and friend module declarations are header-only.
				ReportError(Peek(), DiagnosticCode::E0107_ImportMustBeAtHeader);
				if (Check(TokenType::K_Friend))
					ParseFriendDecl();
				else
					ParseImportDecl();
			}
			else if (Check(TokenType::K_Using))
			{
				// `using import` was consumed above as a header; what remains is the alias.
				DeclPtr alias = ParseAliasDecl();
				if (alias != nullptr)
					module->Declarations.push_back(std::move(alias));
			}
			else
			{
				// TODO: class / interface / enum / def / field / variable declarations are
				// not parsed yet. Skip one token, then resync so the header rules remain
				// testable.
				Advance();
				Synchronize();
			}

			if (m_offset == before)
				Advance();
		}

		module->View = MakeRange(start);
		return module;
	}

	std::unique_ptr<ModuleDecl> Parser::ParseModuleDecl()
	{
		// Caller is on 'module'.
		const SourceView start = Peek().Source;
		auto decl = std::make_unique<ModuleDecl>(start);
		Advance();

		decl->Path = ParseDottedPath();
		if (decl->Path.empty())
			ReportError(Peek(), DiagnosticCode::E0106_PleaseInputIdent);

		ExpectSemi();
		decl->View = MakeRange(start);
		return decl;
	}

	DeclPtr Parser::ParseImportDecl()
	{
		const SourceView start = Peek().Source;
		auto decl = std::make_unique<ImportDecl>(start);

		if (Match(TokenType::K_Public))
			decl->ImportForm |= ImportDecl::Form::PublicImport;

		// `using` is a separate flag: `public using import` is legal and carries both.
		if (Match(TokenType::K_Using))
			decl->ImportForm |= ImportDecl::Form::UsingImport;

		if (not Expect(TokenType::K_Import))
			return nullptr;

		decl->Path = ParseDottedPath();
		if (decl->Path.empty())
		{
			ReportError(Peek(), DiagnosticCode::E0106_PleaseInputIdent);
			return nullptr;
		}

		// `import M.*`
		if (Match(TokenType::Dot))
			decl->IsWildcard = Expect(TokenType::Star);

		if ((decl->ImportForm & ImportDecl::Form::PublicImport) and Match(TokenType::K_With))
		{
			decl->HasWithClause = true;

			if (Expect(TokenType::LBrace))
			{
				while (not Check(TokenType::RBrace) and not Check(TokenType::Eof))
				{
					ImportItem item = ParseImportItem();
					if (item.Name.empty())
					{
						Synchronize();
						break;
					}

					decl->Items.push_back(std::move(item));

					if (not Match(TokenType::Comma))
						break;
				}

				Expect(TokenType::RBrace);
			}
		}

		// Every import directive ends with ';'.
		ExpectSemi();
		decl->View = MakeRange(start);
		return decl;
	}

	DeclPtr Parser::ParseFriendDecl()
	{
		// Caller is on 'friend'.
		const SourceView start = Peek().Source;
		auto decl = std::make_unique<FriendModuleDecl>(start);
		Advance();

		if (not Expect(TokenType::K_Module))
			return nullptr;

		decl->Path = ParseDottedPath();
		if (decl->Path.empty())
			ReportError(Peek(), DiagnosticCode::E0106_PleaseInputIdent);

		ExpectSemi();
		decl->View = MakeRange(start);
		return decl;
	}

	ImportItem Parser::ParseImportItem()
	{
		// import-item := Ident [ 'as' Ident ]
		const SourceView start = Peek().Source;
		ImportItem item;
		item.View = SourceViewRange::Point(start);

		std::optional<std::string_view> name = ParseName();
		if (not name)
		{
			ReportError(Peek(), DiagnosticCode::E0106_PleaseInputIdent);
			return item;
		}

		item.Name = *name;

		if (Match(TokenType::K_As))
		{
			std::optional<std::string_view> alias = ParseName();
			if (alias)
				item.Alias = *alias;
			else
				ReportError(Peek(), DiagnosticCode::E0106_PleaseInputIdent);
		}

		item.View = MakeRange(start);
		return item;
	}

	bool Parser::IsHeaderDeclStart() const
	{
		if (Check(TokenType::K_Import) or Check(TokenType::K_Friend))
			return true;

		if (Check(TokenType::K_Using) and CheckNext(TokenType::K_Import))
			return true;

		// `public import` is a forwarded import, not an access modifier;
		// `public using import` combines both flags.
		if (Check(TokenType::K_Public))
		{
			if (CheckNext(TokenType::K_Import))
				return true;

			if (Peek(1).Type == TokenType::K_Using and Peek(2).Type == TokenType::K_Import)
				return true;
		}

		return false;
	}

	DeclPtr Parser::ParseAliasDecl()
	{
		// alias-decl := 'using' type 'as' Ident ';'
		// The `using import` form was already consumed as a header declaration.
		const SourceView start = Peek().Source;
		if (not Expect(TokenType::K_Using))
			return nullptr;

		// `public` / `using` must come before the import keyword: `using public import`
		// is a prefix order error, not an alias whose type happens to be a keyword.
		if ((Check(TokenType::K_Public) or Check(TokenType::K_Using)) and CheckNext(TokenType::K_Import))
		{
			ReportError(Peek(), DiagnosticCode::E0113_InvalidImportPrefixOrder);
			return ParseImportDecl();
		}

		TypePtr target = ParseType();
		if (target == nullptr)
		{
			Synchronize();
			return nullptr;
		}

		if (not Expect(TokenType::K_As))
		{
			Synchronize();
			return nullptr;
		}

		std::optional<std::string_view> name = ParseName();
		if (not name)
		{
			ReportError(Peek(), DiagnosticCode::E0106_PleaseInputIdent);
			Synchronize();
			return nullptr;
		}

		auto decl = std::make_unique<AliasDecl>(start);
		decl->Target = std::move(target);
		decl->Name = *name;

		ExpectSemi();
		decl->View = MakeRange(start);
		return decl;
	}

	// ============================================================
	// names and types
	// ============================================================

	std::optional<std::string_view> Parser::ParseName()
	{
		if (not Check(TokenType::Ident))
			return std::nullopt;

		return ExtractPayload(Advance());
	}

	std::vector<std::string_view> Parser::ParseDottedPath()
	{
		std::vector<std::string_view> path;

		std::optional<std::string_view> name = ParseName();
		if (not name)
			return path;

		path.push_back(*name);

		while (Check(TokenType::Dot) and CheckNext(TokenType::Ident))
		{
			Advance(); // '.'
			path.push_back(*ParseName());
		}

		return path;
	}

	TypePtr Parser::ParseType()
	{
		// A type is either a parenthesized type or a prefixed base type with suffixes.
		const SourceView start = Peek().Source;

		if (Check(TokenType::LParen))
			return ParseParenthesizedType();

		TypeQualifiers qualifiers = ParseTypeQualifiers();

		TypePtr type = ParseBaseType();
		if (type == nullptr)
			return nullptr;

		type->Prefix = qualifiers;
		type->Suffixes = ParseTypeSuffixes();
		ParseTypeNullableSuffix(*type);

		type->View = MakeRange(start);
		return type;
	}

	TypePtr Parser::ParseParenthesizedType()
	{
		// paren-type := '(' type ')' [ '?' ]
		const SourceView start = Peek().Source;
		Advance(); // '('

		TypePtr operand = ParseType();
		Expect(TokenType::RParen);

		if (operand == nullptr)
			return nullptr;

		auto type = std::make_unique<TypeRef>(start);
		type->BaseKind = TypeRef::Base::Parenthesized;
		type->Operand = std::move(operand);

		ParseTypeNullableSuffix(*type);
		type->View = MakeRange(start);
		return type;
	}

	void Parser::ParseTypeNullableSuffix(TypeRef& type)
	{
		if (Match(TokenType::Question))
		{
			// `(T?)?` puts two '?' on one type, which is the same shape as the forbidden `T??`.
			if (type.BaseKind == TypeRef::Base::Parenthesized and type.Operand != nullptr and type.Operand->IsNullable)
			{
				ReportError(Previous(), DiagnosticCode::E0108_InvalidNullableType);
				return;
			}

			type.IsNullable = true;
		}
		else if (Check(TokenType::QuestionQuestion))
		{
			// `??` is one token; a type may end with at most one '?'.
			ReportError(Peek(), DiagnosticCode::E0108_InvalidNullableType);
			Advance();
		}
	}

	TypeQualifiers Parser::ParseTypeQualifiers()
	{
		// Type prefix: `[const] [ref] [dyn] base`, or `mov base` on its own.
		// `mov` never combines with another qualifier, whichever side it is written on.
		TypeQualifiers qualifiers;

		for (;;)
		{
			if (Match(TokenType::K_Mov))
			{
				if (qualifiers.IsMov)
					ReportError(Previous(), DiagnosticCode::E0104_DuplicateTypeQualifiers, "mov");
				else if (qualifiers.IsConst or qualifiers.IsRef or qualifiers.IsDyn)
					ReportError(Previous(),
					            DiagnosticCode::E0103_ExclusiveTypeQualifiers,
					            "mov",
					            FirstPresentQualifier(qualifiers));
				else
					qualifiers.IsMov = true;

				continue;
			}

			if (Match(TokenType::K_Const))
			{
				if (qualifiers.IsConst)
					ReportError(Previous(), DiagnosticCode::E0104_DuplicateTypeQualifiers, "const");
				else if (qualifiers.IsMov)
					ReportError(Previous(), DiagnosticCode::E0103_ExclusiveTypeQualifiers, "mov", "const");
				else if (qualifiers.IsRef or qualifiers.IsDyn)
					ReportError(Previous(), DiagnosticCode::E0111_InvalidTypeQualifierOrder);
				else
					qualifiers.IsConst = true;

				continue;
			}

			if (Match(TokenType::K_Ref))
			{
				if (qualifiers.IsRef)
					ReportError(Previous(), DiagnosticCode::E0104_DuplicateTypeQualifiers, "ref");
				else if (qualifiers.IsMov)
					ReportError(Previous(), DiagnosticCode::E0103_ExclusiveTypeQualifiers, "mov", "ref");
				else if (qualifiers.IsDyn)
					ReportError(Previous(), DiagnosticCode::E0111_InvalidTypeQualifierOrder);
				else
					qualifiers.IsRef = true;

				continue;
			}

			if (Match(TokenType::K_Dyn))
			{
				if (qualifiers.IsDyn)
					ReportError(Previous(), DiagnosticCode::E0104_DuplicateTypeQualifiers, "dyn");
				else if (qualifiers.IsMov)
					ReportError(Previous(), DiagnosticCode::E0103_ExclusiveTypeQualifiers, "mov", "dyn");
				else
					qualifiers.IsDyn = true;

				continue;
			}

			break;
		}

		return qualifiers;
	}

	TypePtr Parser::ParseBaseType()
	{
		const SourceView start = Peek().Source;

		// 'def' '(' [ param-list ] ')' '->' type
		if (Match(TokenType::K_Def))
		{
			auto type = std::make_unique<TypeRef>(start);
			type->BaseKind = TypeRef::Base::Function;

			if (Expect(TokenType::LParen))
				type->FunctionParams = ParseParameterList();

			Expect(TokenType::RParen);
			Expect(TokenType::Arrow);
			type->FunctionReturn = ParseType();

			return type;
		}

		// 'auto' — `dyn auto` carries Dyn in the qualifiers
		if (Match(TokenType::K_Auto))
		{
			auto type = std::make_unique<TypeRef>(start);
			type->BaseKind = TypeRef::Base::Auto;
			return type;
		}

		// name ( '.' name )* [ '<' type-arg-list '>' ]
		std::vector<std::string_view> path = ParseDottedPath();
		if (path.empty())
		{
			ReportError(Peek(), DiagnosticCode::E0110_ExpectType);
			return nullptr;
		}

		auto type = std::make_unique<TypeRef>(start);
		type->BaseKind = TypeRef::Base::Named;
		type->NamePath = std::move(path);

		if (Match(TokenType::Lt))
		{
			type->HasArgumentList = true;
			type->Arguments = ParseTypeArgumentList();
		}

		return type;
	}

	std::vector<TypeSuffix> Parser::ParseTypeSuffixes()
	{
		std::vector<TypeSuffix> suffixes;

		for (;;)
		{
			if (Match(TokenType::Star))
			{
				suffixes.push_back(TypeSuffix{ SourceViewRange::Point(Previous().Source), TypeSuffix::Form::Pointer, nullptr });
				continue;
			}

			if (Check(TokenType::LBracket))
			{
				const SourceView suffixStart = Peek().Source;
				TypeSuffix suffix{ SourceViewRange::Point(suffixStart), TypeSuffix::Form::View, nullptr };
				Advance();

				if (not Check(TokenType::RBracket))
				{
					// TODO: the extent is a constant expression. Until expressions are
					// parsed, skip the balanced token run so the type still ends correctly.
					Usize depth = 1;
					while (depth > 0 and not Check(TokenType::Eof))
					{
						if (Check(TokenType::LBracket))
							++depth;
						else if (Check(TokenType::RBracket))
							--depth;

						if (depth > 0)
							Advance();
					}

					suffix.Kind = TypeSuffix::Form::Array;
				}

				Expect(TokenType::RBracket);
				suffix.View = MakeRange(suffixStart);
				suffixes.push_back(std::move(suffix));
				continue;
			}

			break;
		}

		return suffixes;
	}

	std::vector<TypePtr> Parser::ParseTypeArgumentList()
	{
		// Caller consumed '<'; this helper also consumes the closing '>' (which may be
		// half of a `>>` / `>>>`), so nested template arguments need no special casing.
		TemplateDepthGuard depthGuard(m_templateDepth);

		std::vector<TypePtr> arguments;
		if (AcceptGreater())
			return arguments; // Foo<>

		for (;;)
		{
			TypePtr argument = ParseType();
			if (argument == nullptr)
				break;

			arguments.push_back(std::move(argument));

			if (not Match(TokenType::Comma))
				break;

			if (AcceptGreater())
				return arguments; // trailing comma: Foo<A,>
		}

		if (not AcceptGreater())
			ReportError(Peek(), DiagnosticCode::E0109_UnclosedTemplateArgumentList);

		return arguments;
	}

	std::vector<ParamVarDecl> Parser::ParseParameterList()
	{
		// Caller consumed '('; ')' is left to it.
		std::vector<ParamVarDecl> parameters;

		if (Check(TokenType::RParen))
			return parameters;

		for (;;)
		{
			ParamVarDecl parameter = ParseParameter();
			if (parameter.Type == nullptr)
				break;

			parameters.push_back(std::move(parameter));

			if (not Match(TokenType::Comma))
				break;

			if (Check(TokenType::RParen))
				break; // trailing comma
		}

		return parameters;
	}

	ParamVarDecl Parser::ParseParameter()
	{
		// param := type [ Ident ]
		const SourceView start = Peek().Source;
		ParamVarDecl parameter;
		parameter.View = SourceViewRange::Point(start);
		parameter.Type = ParseType();

		if (parameter.Type == nullptr)
			return parameter;

		if (Check(TokenType::Ident))
			parameter.Name = *ParseName();

		parameter.View = MakeRange(start);
		return parameter;
	}

	// ============================================================
	// cursor
	// ============================================================

	const Token& Parser::Peek(Usize offset) const
	{
		// Clamp at Eof: callers may look past the end without checking it first.
		return m_tokens[std::min(m_offset + offset, m_tokens.size() - 1)];
	}

	const Token& Parser::Previous() const
	{
		return m_tokens[m_offset == 0 ? 0 : m_offset - 1];
	}

	const Token& Parser::Advance()
	{
		const Token& token = Peek();
		if (m_offset + 1 < m_tokens.size())
			++m_offset;

		return token;
	}

	bool Parser::Check(TokenType type) const
	{
		return Peek().Type == type;
	}

	bool Parser::CheckNext(TokenType type) const
	{
		return Peek(1).Type == type;
	}

	bool Parser::Match(TokenType type)
	{
		if (not Check(type))
			return false;

		Advance();
		return true;
	}

	bool Parser::Expect(TokenType type)
	{
		if (Match(type))
			return true;

		ReportError(Peek(), DiagnosticCode::E0102_ExpectCharacterNotFound, TokenSpelling(type), TokenSpelling(Peek().Type));
		return false;
	}

	bool Parser::ExpectSemi()
	{
		if (Match(TokenType::Semi))
			return true;

		ReportError(Peek(), DiagnosticCode::E0101_ExpectSemi);
		return false;
	}

	SourceViewRange Parser::MakeRange(SourceView begin) const
	{
		return SourceViewRange{ begin, Previous().Source };
	}

	Parser::Checkpoint Parser::Mark() const noexcept
	{
		return Checkpoint{ m_offset, m_pendingGreater, m_templateDepth, m_labels->size() };
	}

	void Parser::Rewind(Checkpoint checkpoint) noexcept
	{
		m_offset = checkpoint.Offset;
		m_pendingGreater = checkpoint.PendingGreater;
		m_templateDepth = checkpoint.TemplateDepth;

		// Drop diagnostics produced by the abandoned parse; keep what came before Mark().
		m_labels->resize(checkpoint.LabelCount);
	}

	bool Parser::AcceptGreater()
	{
		// `>>` / `>>>` are single tokens, but each '>' may close one open template list.
		if (m_pendingGreater > 0)
		{
			--m_pendingGreater;
			return true;
		}

		if (m_templateDepth == 0)
			return false;

		if (Check(TokenType::Gt))
		{
			Advance();
			return true;
		}

		if (Check(TokenType::Shr) or Check(TokenType::ShrU))
		{
			const Usize closers = Check(TokenType::Shr) ? 2 : 3;
			const Usize usable = std::min(closers, m_templateDepth);

			// More '>' than open lists: report the surplus and drop it. Keeping it as
			// pending would make the next declaration believe a '>' is still available.
			if (usable < closers)
				ReportError(Peek(), DiagnosticCode::E0112_UnmatchedGreater);

			m_pendingGreater = usable - 1;
			Advance();
			return true;
		}

		return false;
	}

	// ============================================================
	// recovery
	// ============================================================

	void Parser::Synchronize()
	{
		while (not Check(TokenType::Eof))
		{
			if (Match(TokenType::Semi))
				return;

			if (Check(TokenType::RBrace))
				return;

			if (IsHeaderDeclStart())
				return;

			Advance();
		}
	}
} // namespace Pxc::PLang
