// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once
#include "../Diagnostic/Diagnostic.hpp"
#include "Token.hpp"

#include <expected>
#include <format>
#include <vector>

namespace Pxc::PLang
{
	class Lexer
	{
	public:
		TokenList Tokenize(std::string_view source, DiagnosticLabelList* labels, bool needCommentToken);

	private:
		void Advance(Usize off = 1) noexcept;

		bool AtEnd() const noexcept;

		char Peek(I64 off = 0) const noexcept;

		bool TrySkipWhitespace() noexcept;
		bool TrySkip(std::string_view block) noexcept;

		void LexFString();
		void LexString();
		void LexChar();
		void LexIdentOrKeyword();
		void LexNum();
		void LexLineComment();
		void LexBlockComment();
		void LexBinaryOp();

		bool LexEscape(char32_t* outValue);

		Usize UTF8BlockLength();

		SourceView CreateSourceView();

		void CreateToken(TokenType type, const SourceView& start, TokenPayload payload = {});

		template <typename... Args>
		void ReportError(SourceView view, DiagnosticCode code, Args&&... args);
		template <typename... Args>
		void ReportWarning(SourceView view, DiagnosticCode code, Args&&... args);

		std::string_view ViewFrom(Usize from) const noexcept;

		std::vector<Token> m_tokens;
		std::string_view m_source;

		DiagnosticLabelList* m_diagnosticLabels = nullptr;
		bool m_needCommentToken = false;

		Usize m_currentOffset = 0;
		Usize m_currentLine = 0;
		Usize m_currentColumn = 0;
	};

	template <typename... Args>
	void Lexer::ReportError(SourceView view, DiagnosticCode code, Args&&... args)
	{
		DiagnosticArgsVector argVector;

		(argVector.emplace_back(NormalizeDiagnosticArgs(std::forward<Args>(args))), ...);
		m_diagnosticLabels->emplace_back(DiagnosticLevel::Error, code, std::move(argVector), view);
	}

	template <typename... Args>
	void Lexer::ReportWarning(SourceView view, DiagnosticCode code, Args&&... args)
	{
		DiagnosticArgsVector argVector;

		(argVector.emplace_back(NormalizeDiagnosticArgs(std::forward<Args>(args))), ...);
		m_diagnosticLabels->emplace_back(DiagnosticLevel::Warning, code, std::move(argVector), view);
	}
} // namespace Pxc::PLang