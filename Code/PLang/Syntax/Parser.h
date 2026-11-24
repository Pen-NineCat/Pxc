// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once
#include "PLang/AST/Decl.hpp"
#include "PLang/Diagnostic/Diagnostic.hpp"
#include "PLang/Lex/Token.hpp"

#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace Pxc::PLang
{
	/// @brief Recursive-descent parser: token list in, syntax tree plus diagnostics out.
	/// @note Syntax only. Name resolution, type checking and constant folding are later passes.
	class Parser
	{
	public:
		Parser() noexcept = default;

		/// @brief Parse one module unit.
		/// @param tokens output of Lexer::Tokenize, must end with TokenType::Eof
		/// @param labels diagnostic sink, must outlive the returned tree
		/// @return the parsed unit, or nullptr when the token list is unusable
		/// @note Current coverage: types, the unit header and alias declarations.
		///       Other declarations, statements and expressions are not parsed yet; tokens
		///       belonging to them are skipped for now.
		std::unique_ptr<Module> Parse(std::span<const Token> tokens, DiagnosticLabelList* labels);

	private:
		/// @brief Cursor state that speculative parsing must restore.
		/// @note Rewind also drops the diagnostics emitted after Mark, so an abandoned
		///       speculative parse does not leak errors.
		struct Checkpoint
		{
			Usize Offset;
			Usize PendingGreater;
			Usize TemplateDepth;
			Usize LabelCount;
		};

		// ---------- token cursor ----------

		const Token& Peek(Usize offset = 0) const;

		/// @brief Token right before the cursor.
		const Token& Previous() const;

		const Token& Advance();

		bool Check(TokenType type) const;
		bool CheckNext(TokenType type) const;

		/// @brief Consume the current token when it has the given type.
		bool Match(TokenType type);

		/// @brief Consume the current token when it has the given type, otherwise report a diagnostic.
		bool Expect(TokenType type);

		bool ExpectSemi();

		/// @brief Source range from `begin` to the end of the last consumed token.
		SourceViewRange MakeRange(SourceView begin) const;

		Checkpoint Mark() const noexcept;
		void Rewind(Checkpoint checkpoint) noexcept;

		/// @brief Consume one '>' that closes a template argument list.
		/// @return false when the cursor is not on a '>' at all
		/// @note `>>` / `>>>` are one token and are split according to the current
		///       template nesting depth; surplus '>' tokens are reported.
		bool AcceptGreater();

		// ---------- unit level ----------

		std::unique_ptr<Module> ParseModule();
		std::unique_ptr<ModuleDecl> ParseModuleDecl();
		DeclPtr ParseImportDecl();
		DeclPtr ParseFriendDecl();
		ImportItem ParseImportItem();

		/// @brief Does a declaration of the header group start here?
		/// @note Header group: import, import M.*, using import, public import,
		///       public using import, friend module.
		bool IsHeaderDeclStart() const;

		DeclPtr ParseAliasDecl();

		// ---------- names and types ----------

		std::optional<std::string_view> ParseName();
		std::vector<std::string_view> ParseDottedPath();

		TypePtr ParseType();
		TypePtr ParseParenthesizedType();

		/// @brief Consume the optional trailing '?' of a type, reporting a repeated one.
		void ParseTypeNullableSuffix(TypeRef& type);

		TypeQualifiers ParseTypeQualifiers();
		TypePtr ParseBaseType();
		std::vector<TypeSuffix> ParseTypeSuffixes();

		/// @brief Parse template arguments after the opening '<' and consume the closing '>'.
		std::vector<TypePtr> ParseTypeArgumentList();

		std::vector<ParamVarDecl> ParseParameterList(); // ')' is left to the caller
		ParamVarDecl ParseParameter();

		// ---------- diagnostics and recovery ----------

		/// @brief Skip to the next declaration boundary: consume ';', stop before '}'.
		void Synchronize();

		template <typename... Args>
		void ReportError(const Token& token, DiagnosticCode code, Args&&... args)
		{
			DiagnosticArgsVector argsVector;
			(argsVector.emplace_back(NormalizeDiagnosticArgs(std::forward<Args>(args))), ...);
			m_labels->emplace_back(DiagnosticLevel::Error, code, std::move(argsVector), token.Source);
		}

		std::span<const Token> m_tokens;
		DiagnosticLabelList* m_labels = nullptr;

		Usize m_offset = 0;

		/// @brief Extra '>' still owed to enclosing template argument lists.
		Usize m_pendingGreater = 0;

		/// @brief Number of currently open template argument lists.
		Usize m_templateDepth = 0;
	};
} // namespace Pxc::PLang
