// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#include "Lexer.h"
#include "Keywords.hpp"
#include <algorithm>

namespace
{
	constexpr bool IsDigitChar(char c) noexcept
	{
		return c >= '0' and c <= '9';
	}

	constexpr bool IsIdentStartChar(char c) noexcept
	{
		return c == '_' or (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z');
	}

	constexpr bool IsWhitespace(char c) noexcept
	{
		return c == ' ' or c == '\t' or c == '\r' or c == '\v' or c == '\f' or c == '\n';
	}

	constexpr bool IsIdentFollowChar(char c) noexcept
	{
		return IsIdentStartChar(c) or IsDigitChar(c);
	}

	constexpr int DigitValue(char c, int radix) noexcept
	{
		int value = -1;
		if (c >= '0' and c <= '9')
		{
			value = c - '0';
		}
		else if (c >= 'a' and c <= 'f')
		{
			value = c - 'a' + 10;
		}
		else if (c >= 'A' and c <= 'F')
		{
			value = c - 'A' + 10;
		}
		return (value >= 0 and value < radix) ? value : -1;
	}
} // namespace

namespace Pxc::PLang
{
	TokenList Lexer::Tokenize(std::string_view source, DiagnosticLabelList* labels, bool needCommentToken)
	{
		m_source = source;
		m_needCommentToken = needCommentToken;
		m_diagnosticLabels = labels;
		m_tokens.clear();
		m_currentOffset = 0;
		m_currentColumn = 1;
		m_currentLine = 1;
	
		// UTF-8 BOM
		if (m_source.compare(m_currentOffset, 3, "\xEF\xBB\xBF") == 0)
			m_currentOffset += 3;

		while (true)
		{
			TrySkipWhitespace();

			if (AtEnd())
				break;

			char c = Peek();

			if (c == '/' and Peek(1) == '/')
				LexLineComment();
			else if (c == '/' and Peek(1) == '*')
				LexBlockComment();
			else if (c == 'f' and Peek(1) == '"')
				LexFString();
			else if (c == '"')
				LexString();
			else if (c == '\'')
				LexChar();
			else
			{
				// Skip Non ASCII Char
				const auto byte = static_cast<unsigned char>(Peek());
				if (byte >= 0x80)
				{
					const SourceView start = CreateSourceView();
					if (UTF8BlockLength() == 0)
					{
						m_currentOffset += 1;
						m_currentColumn += 1;
						ReportError(start, DiagnosticCode::E00015_InvalidCharCodepoint);
						continue;
					}

					Advance();
					ReportError(start, DiagnosticCode::E00020_InvalidNonASCIICodepointPosition);
					continue;
				}

				if (IsIdentStartChar(c))
					LexIdentOrKeyword();
				else if (IsDigitChar(c))
					LexNum();
				else
					LexBinaryOp();
			}
		}

		// EOF
		CreateToken(TokenType::Eof, CreateSourceView());

		return std::move(m_tokens);
	}

	void Lexer::Advance(Usize off) noexcept
	{
		for (int i = 0; i < off && !AtEnd(); ++i)
		{
			const char c = m_source[m_currentOffset];

			// \r\n 整体算一个换行，避免行号被计两次
			if (c == '\r')
			{
				m_currentOffset += (m_currentOffset + 1 < m_source.size() && m_source[m_currentOffset + 1] == '\n') ? 2 : 1;
				++m_currentLine;
				m_currentColumn = 1;
				continue;
			}

			if (c == '\n')
			{
				++m_currentOffset;
				++m_currentLine;
				m_currentColumn = 1;
				continue;
			}

			Usize length = UTF8BlockLength();
			if (length == 0)
			{
				length = 1;
			}

			m_currentOffset += length;
			++m_currentColumn;
		}
	}

	bool Lexer::AtEnd() const noexcept
	{
		return m_currentOffset >= m_source.size();
	}

	char Lexer::Peek(I64 off) const noexcept
	{
		Isize idx = m_currentOffset + off;

		if (idx >= 0 and static_cast<Usize>(idx) < m_source.size())
			return m_source[static_cast<Usize>(idx)];

		return '\0';
	}

	bool Lexer::TrySkipWhitespace() noexcept
	{
		bool skipped = false;

		while (!AtEnd())
		{
			char c = Peek();

			if (IsWhitespace(c))
			{
				skipped = true;
				Advance();
				continue;
			}

			break;
		}

		return skipped;
	}

	bool Lexer::TrySkip(std::string_view block) noexcept
	{
		if (m_source.size() < block.size())
			return false;

		if (m_source.compare(m_currentOffset, block.size(), block) == 0)
		{
			Advance(block.size());
			return true;
		}

		return false;
	}

	void Lexer::LexFString()
	{
		// Consume "f""
		Advance(2);

		Usize depth = 0;
		Usize start = m_currentOffset;

		SourceView view = CreateSourceView();
		bool valid = true;

		while (true)
		{
			if (AtEnd() or Peek() == '\n' or Peek() == '\r')
			{
				ReportError(view, DiagnosticCode::E00010_UnterminatedStringLiteral);
				return;
			}

			char c = Peek();

			if (depth == 0)
			{
				// F-String end '"'
				if (c == '"')
				{
					std::string_view text = ViewFrom(start);
					Advance();
					if (valid)
						CreateToken(TokenType::FString, view, text);
					return;
				}

				// Ignore {{ and }}, both are literal
				if ((c == '{' and Peek(1) == '{') or (c == '}' and Peek(1) == '}'))
				{
					Advance(2);
					continue;
				}

				if (c == '{')
				{
					++depth;
					Advance();
					continue;
				}

				if (c == '\\')
				{
					valid = LexEscape(nullptr) && valid;
					continue;
				}

				Advance();
				continue;
			}
			else
			{
				// In block {...}

				// "{"..."}"
				if (c == '"')
				{
					Advance();
					bool closed = false;
					while (!AtEnd())
					{
						char inner = Peek();
						if (inner == '\n' or inner == '\r')
							break;

						if (inner == '\\')
						{
							Advance(2);
							continue;
						}

						if (inner == '"')
						{
							Advance();
							closed = true;
							break;
						}
						Advance();
					}

					if (!closed)
						return ReportError(view, DiagnosticCode::E00010_UnterminatedStringLiteral);

					continue;
				}

				if (c == '{')
					depth++;

				if (c == '}')
					depth--;

				Advance();
			}
		}
	}

	void Lexer::LexString()
	{
		// Consume '"'
		Advance(1);

		SourceView view = CreateSourceView();
		Usize start = m_currentOffset;

		while (true)
		{
			char c = Peek();

			if (c == '\n' or c == '\r' or AtEnd())
			{
				ReportError(view, DiagnosticCode::E00010_UnterminatedStringLiteral);
				return;
			}

			if (c == '"')
			{
				std::string_view text = ViewFrom(start);
				CreateToken(TokenType::String, view, text);
				Advance();
				return;
			}

			Advance();
		}
	}

	void Lexer::LexChar()
	{
		const SourceView start = CreateSourceView();

		// Consume '\''
		Advance();

		// Panic restore, jump to '\'' or line end
		const auto Resync = [&]()
		{
			while (!AtEnd() and Peek() != '\'' and Peek() != '\n' and Peek() != '\r')
			{
				Advance();
			}
			if (!AtEnd() and Peek() == '\'')
			{
				Advance();
			}
		};

		if (AtEnd() or Peek() == '\'' or Peek() == '\n' or Peek() == '\r')
		{
			const bool unterminated = AtEnd() or Peek() != '\'';
			Resync();
			ReportError(start, unterminated ? DiagnosticCode::E00014_UnterminatedCharLiteral : DiagnosticCode::E00015_InvalidCharCodepoint);
			return;
		}

		char32_t value = 0;
		if (Peek() == '\\')
		{
			if (!LexEscape(&value))
			{
				Resync();
				return;
			}
		}
		else
		{
			const Usize length = UTF8BlockLength();
			if (length == 0)
			{
				Resync();
				ReportError(start, DiagnosticCode::E00015_InvalidCharCodepoint);
				return;
			}

			const auto lead = static_cast<unsigned char>(Peek());
			U32 code = 0;
			switch (length)
			{
				case 1:
					code = lead;
					break;
				case 2:
					code = lead & 0x1F;
					break;
				case 3:
					code = lead & 0x0F;
					break;
				default:
					code = lead & 0x07;
					break;
			}

			for (Usize i = 1; i < length; ++i)
			{
				code = (code << 6) | (static_cast<unsigned char>(Peek(static_cast<int>(i))) & 0x3F);
			}
			Advance();
			value = code;
		}

		if (AtEnd() or Peek() != '\'')
		{
			const bool unterminated = AtEnd() or Peek() == '\n' or Peek() == '\r';
			Resync();
			ReportError(start, unterminated ? DiagnosticCode::E00014_UnterminatedCharLiteral : DiagnosticCode::E00015_InvalidCharCodepoint);
			return;
		}

		Advance(); // '"'
		CreateToken(TokenType::Char, start, value);
	}

	void Lexer::LexIdentOrKeyword()
	{
		SourceView view = CreateSourceView();
		Usize start = m_currentOffset;

		while (not AtEnd() and IsIdentFollowChar(Peek()))
			Advance();

		std::string_view text = ViewFrom(start);

		if (auto it = g_kwMap.find(text); it != g_kwMap.end())
		{
			CreateToken(it->second, view);
			return;
		}

		CreateToken(TokenType::Ident, view, text);
	}

	void Lexer::LexNum()
	{
		const SourceView start = CreateSourceView();

		/// 依次消费「数字或下划线」，并校验下划线两侧都必须有数字
		///
		const auto consumeDigits = [&](int radix, bool& sawDigit, bool& underscoreOk)
		{
			sawDigit = false;
			underscoreOk = true;
			bool previousIsDigit = false;

			while (!AtEnd())
			{
				const char c = Peek();
				if (c == '_')
				{
					underscoreOk = underscoreOk and previousIsDigit;
					previousIsDigit = false;
					Advance();
					continue;
				}

				if (DigitValue(c, radix) < 0)
				{
					break;
				}

				previousIsDigit = true;
				sawDigit = true;
				Advance();
			}

			underscoreOk = underscoreOk and previousIsDigit;
		};

		/// 数字后紧跟标识符字符视为粘连，吃掉这些字符并报错；返回是否已报错
		const auto rejectGlue = [&]() -> bool
		{
			if (AtEnd() or !IsIdentFollowChar(Peek()))
			{
				return false;
			}

			while (!AtEnd() and IsIdentFollowChar(Peek()))
			{
				Advance();
			}
			ReportError(start, DiagnosticCode::E00019_InvalidNumericLiteralPosition);
			return true;
		};

		/// Number literal u / ul / l / f
		const auto readSuffix = [&](bool isFloatLiteral) -> LiteralSuffix
		{
			if (isFloatLiteral)
			{
				if (Peek() == 'f')
				{
					Advance();
					return LiteralSuffix::F32;
				}
				return LiteralSuffix::None;
			}

			if (Peek() == 'u')
			{
				Advance();
				if (Peek() == 'l')
				{
					Advance();
					return LiteralSuffix::U64;
				}
				return LiteralSuffix::U32;
			}

			if (Peek() == 'l')
			{
				Advance();
				return LiteralSuffix::I64;
			}
			return LiteralSuffix::None;
		};

		// 0x / 0b / 0o
		if (Peek() == '0')
		{
			const char second = Peek(1);
			int radix = 0;
			if (second == 'x' or second == 'X')
			{
				radix = 16;
			}
			else if (second == 'b' or second == 'B')
			{
				radix = 2;
			}
			else if (second == 'o' or second == 'O')
			{
				radix = 8;
			}

			if (radix != 0)
			{
				const Usize begin = m_currentOffset;
				Advance(2);

				bool sawDigit = false;
				bool underscoreOk = true;
				consumeDigits(radix, sawDigit, underscoreOk);

				if (!sawDigit)
				{
					rejectGlue();
					ReportError(start, DiagnosticCode::E00019_InvalidNumericLiteralPosition);
					return;
				}
				if (!underscoreOk)
				{
					ReportError(start, DiagnosticCode::E00018_InvalidNumericSeparatorsPosition);
					return;
				}

				const Usize textEnd = m_currentOffset;
				const LiteralSuffix suffix = readSuffix(false);
				if (rejectGlue())
				{
					return;
				}

				CreateToken(TokenType::Int, start, NumberLiteral{ m_source.substr(begin, textEnd - begin), suffix });
				return;
			}
		}

		// 前导零十进制（012）：避免 C 的八进制歧义
		if (Peek() == '0' and IsDigitChar(Peek(1)))
		{
			while (!AtEnd() and (IsDigitChar(Peek()) or Peek() == '_'))
			{
				Advance();
			}
			while (!AtEnd() and IsIdentFollowChar(Peek()))
			{
				Advance();
			}
			ReportError(start, DiagnosticCode::E00019_InvalidNumericLiteralPosition);
			return;
		}

		const Usize begin = m_currentOffset;
		bool sawIntegerDigit = false;
		bool integerUnderscoreOk = true;
		consumeDigits(10, sawIntegerDigit, integerUnderscoreOk);

		bool isFloat = false;

		// 小数点只在两侧都有数字时归入浮点，否则是 Dot（如 1.toString()）
		if (Peek() == '.' and IsDigitChar(Peek(1)))
		{
			isFloat = true;
			Advance();

			bool sawFractionDigit = false;
			bool fractionUnderscoreOk = true;
			consumeDigits(10, sawFractionDigit, fractionUnderscoreOk);
			if (!fractionUnderscoreOk)
			{
				ReportError(start, DiagnosticCode::E00018_InvalidNumericSeparatorsPosition);
				return;
			}
		}

		// 指数部分
		if (Peek() == 'e' or Peek() == 'E')
		{
			isFloat = true;
			Advance();
			if (Peek() == '+' or Peek() == '-')
			{
				Advance();
			}

			bool sawExponentDigit = false;
			bool exponentUnderscoreOk = true;
			consumeDigits(10, sawExponentDigit, exponentUnderscoreOk);
			if (!sawExponentDigit or !exponentUnderscoreOk)
			{
				ReportError(start, DiagnosticCode::E00019_InvalidNumericLiteralPosition);
				return;
			}
		}

		if (!integerUnderscoreOk)
		{
			ReportError(start, DiagnosticCode::E00018_InvalidNumericSeparatorsPosition);
			return;
		}

		const Usize textEnd = m_currentOffset;
		const LiteralSuffix suffix = readSuffix(isFloat);
		if (rejectGlue())
		{
			return;
		}

		CreateToken(isFloat ? TokenType::Float : TokenType::Int, start, NumberLiteral{ m_source.substr(begin, textEnd - begin), suffix });
	}

	void Lexer::LexLineComment()
	{
		SourceView view = CreateSourceView();
		// Consume "//"
		Advance(2);

		Usize start = m_currentOffset;

		while (not AtEnd() and Peek() != '\n' and Peek() != '\r')
			Advance();

		if (m_needCommentToken)
			CreateToken(TokenType::LineComment, view, ViewFrom(start));
	}

	void Lexer::LexBlockComment()
	{
		SourceView view = CreateSourceView();
		Advance(2);

		Usize start = m_currentOffset;

		while (true)
		{
			if (AtEnd())
			{
				ReportError(view, DiagnosticCode::E00017_UnterminatedCommentLiteral);
				return;
			}

			if (Peek() == '*' and Peek(1) == '/')
			{
				Advance(2);
				break;
			}

			Advance();
		}

		if (m_needCommentToken)
			CreateToken(TokenType::BlockComment, view, ViewFrom(start));
	}

	void Lexer::LexBinaryOp()
	{
		SourceView view = CreateSourceView();

		for (auto& [text, op] : BinrayPairList)
		{
			if (TrySkip(text))
			{
				CreateToken(op, view);
				return;
			}
		}

		ReportError(view, DiagnosticCode::E0006_UndefinedSymbol, Peek());
		Advance();
	}

	bool Lexer::LexEscape(char32_t* outValue)
	{
		const SourceView start = CreateSourceView();
		Advance(); // 反斜杠

		if (AtEnd())
		{
			ReportError(start, DiagnosticCode::E00016_InvalidEscapeSequences);
			return false;
		}

		char32_t value = 0;
		switch (Peek())
		{
			case '\\':
				value = U'\\';
				Advance();
				break;
			case '"':
				value = U'"';
				Advance();
				break;
			case '\'':
				value = U'\'';
				Advance();
				break;
			case '0':
				value = U'\0';
				Advance();
				break;
			case 'a':
				value = 0x07;
				Advance();
				break;
			case 'b':
				value = 0x08;
				Advance();
				break;
			case 'f':
				value = 0x0C;
				Advance();
				break;
			case 'n':
				value = U'\n';
				Advance();
				break;
			case 'r':
				value = U'\r';
				Advance();
				break;
			case 't':
				value = U'\t';
				Advance();
				break;
			case 'v':
				value = 0x0B;
				Advance();
				break;
			case 'x':
			{
				Advance(); // 'x'
				U32 code = 0;
				for (int i = 0; i < 2; ++i)
				{
					const int digit = DigitValue(Peek(), 16);
					if (digit < 0)
					{
						ReportError(start, DiagnosticCode::E00016_InvalidEscapeSequences);
						return false;
					}
					code = code * 16 + static_cast<U32>(digit);
					Advance();
				}
				value = code;
				break;
			}
			case 'u':
			{
				Advance(); // 'u'
				if (Peek() != '{')
				{
					ReportError(start, DiagnosticCode::E00016_InvalidEscapeSequences);
					return false;
				}
				Advance();

				U32 code = 0;
				int digits = 0;
				while (!AtEnd() and Peek() != '}')
				{
					const int digit = DigitValue(Peek(), 16);
					if (digit < 0 or digits >= 6)
					{
						ReportError(start, DiagnosticCode::E00016_InvalidEscapeSequences);
						return false;
					}
					code = code * 16 + static_cast<U32>(digit);
					++digits;
					Advance();
				}

				if (digits == 0 or AtEnd() or Peek() != '}' or code > 0x10FFFF)
				{
					ReportError(start, DiagnosticCode::E00016_InvalidEscapeSequences);
					return false;
				}

				Advance(); // '}'
				value = code;
				break;
			}
			default:
				Advance(); // Eat invalid sequences to continue
				ReportError(start, DiagnosticCode::E00016_InvalidEscapeSequences);
				return false;
		}

		if (outValue != nullptr)
		{
			*outValue = value;
		}
		return true;
	}

	Usize Lexer::UTF8BlockLength()
	{
		if (AtEnd())
		{
			return 0;
		}

		const auto lead = static_cast<unsigned char>(m_source[m_currentOffset]);
		if (lead < 0x80)
		{
			return 1;
		}

		Usize length = 0;
		if ((lead & 0xE0) == 0xC0)
		{
			length = 2;
		}
		else if ((lead & 0xF0) == 0xE0)
		{
			length = 3;
		}
		else if ((lead & 0xF8) == 0xF0)
		{
			length = 4;
		}
		else
		{
			return 0;
		}

		if (m_currentOffset + length > m_source.size())
		{
			return 0;
		}

		for (Usize i = 1; i < length; ++i)
		{
			if ((static_cast<unsigned char>(m_source[m_currentOffset + i]) & 0xC0) != 0x80)
			{
				return 0;
			}
		}

		return length;
	}

	SourceView Lexer::CreateSourceView()
	{
		return SourceView{
			.Line = m_currentLine,
			.Column = m_currentColumn,
			.Offset = m_currentOffset,
			.Length = 0
		};
	}

	void Lexer::CreateToken(TokenType type, const SourceView& start, TokenPayload payload)
	{
		Token token;
		token.Type = type;
		token.Payload = std::move(payload);
		token.Source = start;
		token.Source.Length = static_cast<U32>(m_currentOffset - start.Offset);
		m_tokens.push_back(std::move(token));
	}

	std::string_view Lexer::ViewFrom(Usize from) const noexcept
	{
		return m_source.substr(from, m_currentOffset - from);
	}
} // namespace Pxc::PLang