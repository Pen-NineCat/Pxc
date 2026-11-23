// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#include "PLang/Lex/Lexer.h"
#include "PLang/Lex/Token.hpp"

#include "../Common.hpp"

#include <gtest/gtest.h>
#include <initializer_list>
#include <string_view>
#include <variant>
#include <vector>

namespace Pxc::PLang::Test
{
	namespace
	{
		/// @brief Assert that the token type sequence matches exactly, including the final Eof.
		void ExpectTypes(const TokenList& tokens, std::initializer_list<TokenType> expected)
		{
			ASSERT_EQ(tokens.size(), expected.size());

			Usize index = 0;
			for (const TokenType type : expected)
			{
				EXPECT_EQ(tokens[index].Type, type) << "token at index " << index;
				++index;
			}
		}

		/// @brief String slice payload of the token at the given index.
		std::string_view TextAt(const std::vector<Token>& tokens, Usize index)
		{
			return std::get<std::string_view>(tokens[index].Payload);
		}

		/// @brief Numeric literal payload of the token at the given index.
		NumberLiteral NumberAt(const std::vector<Token>& tokens, Usize index)
		{
			return std::get<NumberLiteral>(tokens[index].Payload);
		}
	} // namespace

	TEST(Lexer, SampleFromAgentsMd)
	{
		// Expect: the full token sequence of the sample, with the println identifier and the
		// string payload located at the expected indices.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("import std.io;\n"
		                                   "\n"
		                                   "def main() -> int\n"
		                                   "{\n"
		                                   "\tI32 a = 0;\n"
		                                   "\tstring b = \"Hello,world\";\n"
		                                   "\tauto c = b + a;\n"
		                                   "\n"
		                                   // f-strings are not provided yet; the original sample used
		                                   // println(f"Hello,world,{a}")
		                                   "\tstd.io.println(\"Hello,world\");\n"
		                                   "\n"
		                                   "\treturn 0;\n"
		                                   "}\n",
		                                   &diagnosticLabelList, false);

		ExpectTypes(tokens,
		            { TokenType::K_Import, TokenType::Ident, TokenType::Dot, TokenType::Ident, TokenType::Semi,
		              TokenType::K_Def, TokenType::Ident, TokenType::LParen, TokenType::RParen, TokenType::Arrow,
		              TokenType::Ident, TokenType::LBrace, TokenType::Ident, TokenType::Ident, TokenType::Eq,
		              TokenType::Int, TokenType::Semi, TokenType::Ident, TokenType::Ident, TokenType::Eq,
		              TokenType::String, TokenType::Semi, TokenType::K_Auto, TokenType::Ident, TokenType::Eq,
		              TokenType::Ident, TokenType::Plus, TokenType::Ident, TokenType::Semi, TokenType::Ident,
		              TokenType::Dot, TokenType::Ident, TokenType::Dot, TokenType::Ident, TokenType::LParen,
		              TokenType::String, TokenType::RParen, TokenType::Semi, TokenType::K_Return, TokenType::Int,
		              TokenType::Semi, TokenType::RBrace, TokenType::Eof });

		EXPECT_EQ(TextAt(tokens, 33), "println");
		EXPECT_EQ(TextAt(tokens, 35), "Hello,world");
	}

	TEST(Lexer, ArrowIsSingleToken)
	{
		// Expect: '->' is one Arrow token spanning two characters.
		Lexer lexer;

		DiagnosticLabelList diagnosticLabelList;
		const TokenList tokens = lexer.Tokenize("->", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Arrow, TokenType::Eof });
		EXPECT_EQ(tokens[0].Source.Length, 2);
	}

	// f-strings are not provided yet (the lexer branch is commented out); re-enable this
	// section together with that branch.
#if 0
	TEST(Lexer, FStringIsSingleToken)
	{
		// Expect: (disabled) f"..." would lex as one FString token.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("f\"Hello,{a}\"", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::FString, TokenType::Eof });
		EXPECT_EQ(TextAt(tokens, 0), "Hello,{a}");
	}

	TEST(Lexer, FStringWithNestedStringInHole)
	{
		// Expect: (disabled) a nested string inside a hole would stay inside the FString token.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize(R"(f"{d["k"]}")", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::FString, TokenType::Eof });
		EXPECT_EQ(TextAt(tokens, 0), "{d[\"k\"]}");
	}
#endif

	TEST(Lexer, StringKeepsRawEscapeSequence)
	{
		// Expect: the escape sequence is kept raw in the string payload.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize(R"("ab\tc")", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::String, TokenType::Eof });
		EXPECT_EQ(TextAt(tokens, 0), "ab\\tc");
	}

	TEST(Lexer, InvalidStringEscapeIsError)
	{
		// Expect: an invalid escape reports the escape error and produces no string token.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize(R"("a\qc")", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Eof });
		ASSERT_EQ(diagnosticLabelList.size(), 1);
		EXPECT_EQ(diagnosticLabelList[0].Code, DiagnosticCode::E00016_InvalidEscapeSequences);
	}

	TEST(Lexer, StringEndingWithBackslashIsUnterminated)
	{
		// Expect: a string ending in a backslash before a newline reports the unterminated
		// string error.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("\"ab\\\nnext", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Ident, TokenType::Eof });
		ASSERT_EQ(diagnosticLabelList.size(), 1);
		EXPECT_EQ(diagnosticLabelList[0].Code, DiagnosticCode::E00010_UnterminatedStringLiteral);
	}

	TEST(Lexer, FStringSplitsIntoIdentAndString)
	{
		// Expect: f"..." is split into an Ident 'f' followed by a String token.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("f\"Hello,{a}\"", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Ident, TokenType::String, TokenType::Eof });
		EXPECT_EQ(TextAt(tokens, 0), "f");
		EXPECT_EQ(TextAt(tokens, 1), "Hello,{a}");
	}

	TEST(Lexer, SpecOperatorTokens)
	{
		// Expect: the spec operators lex as single tokens with no diagnostics.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize(":= === !== ?? >>>= ...", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::ColonEq, TokenType::EqEqEq, TokenType::BangEqEq, TokenType::QuestionQuestion,
		                      TokenType::ShrUEq, TokenType::Ellipsis, TokenType::Eof });
		EXPECT_EMPTY_DIAGNOSTIC_LABELS(diagnosticLabelList);
	}

	TEST(Lexer, SpecKeywordsAreReserved)
	{
		// Expect: every listed word lexes as its keyword token, with no diagnostics.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("switch do friend with operator sizeof alignof noexcept implicit default delete",
		                                   &diagnosticLabelList, false);

		ExpectTypes(tokens,
		            { TokenType::K_Switch, TokenType::K_Do, TokenType::K_Friend, TokenType::K_With, TokenType::K_Operator,
		              TokenType::K_Sizeof, TokenType::K_Alignof, TokenType::K_Noexcept, TokenType::K_Implicit,
		              TokenType::K_Default, TokenType::K_Delete, TokenType::Eof });
		EXPECT_EMPTY_DIAGNOSTIC_LABELS(diagnosticLabelList);
	}

	TEST(Lexer, DotsAreNotEllipsis)
	{
		// Expect: '.' and '..' split into Dot tokens; only '...' becomes Ellipsis.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize(". .. ...", &diagnosticLabelList, false);

		ExpectTypes(tokens,
		            { TokenType::Dot, TokenType::Dot, TokenType::Dot, TokenType::Ellipsis, TokenType::Eof });
		EXPECT_EMPTY_DIAGNOSTIC_LABELS(diagnosticLabelList);
	}

	TEST(Lexer, CharDecodesCodePoint)
	{
		// Expect: each character literal decodes to its code point.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize(R"('a' '\n' '\x41' '\u{4F60}')", &diagnosticLabelList, false);

		ExpectTypes(tokens,
		            { TokenType::Char, TokenType::Char, TokenType::Char, TokenType::Char, TokenType::Eof });
		EXPECT_EQ(std::get<char32_t>(tokens[0].Payload), U'a');
		EXPECT_EQ(std::get<char32_t>(tokens[1].Payload), U'\n');
		EXPECT_EQ(std::get<char32_t>(tokens[2].Payload), U'A');
		EXPECT_EQ(std::get<char32_t>(tokens[3].Payload), U'\u4F60');
	}

	TEST(Lexer, RadixIntegerLiterals)
	{
		// Expect: hex, binary and octal literals keep their source text.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("0x1F 0b1010 0o755", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Int, TokenType::Int, TokenType::Int, TokenType::Eof });
		EXPECT_EQ(NumberAt(tokens, 0).Text, "0x1F");
		EXPECT_EQ(NumberAt(tokens, 1).Text, "0b1010");
		EXPECT_EQ(NumberAt(tokens, 2).Text, "0o755");
	}

	TEST(Lexer, UnderscoreSeparators)
	{
		// Expect: underscores between digits are accepted and kept in the text.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("1_000_000", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Int, TokenType::Eof });
		EXPECT_EQ(NumberAt(tokens, 0).Text, "1_000_000");
	}

	TEST(Lexer, NumberSuffixes)
	{
		// Expect: suffixes are recorded separately and excluded from the literal text.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("1u 1l 1ul 1.0f 1.0", &diagnosticLabelList, false);

		ExpectTypes(tokens,
		            { TokenType::Int, TokenType::Int, TokenType::Int, TokenType::Float, TokenType::Float,
		              TokenType::Eof });
		EXPECT_EQ(NumberAt(tokens, 0).Suffix, LiteralSuffix::U32);
		EXPECT_EQ(NumberAt(tokens, 1).Suffix, LiteralSuffix::I64);
		EXPECT_EQ(NumberAt(tokens, 2).Suffix, LiteralSuffix::U64);
		EXPECT_EQ(NumberAt(tokens, 3).Suffix, LiteralSuffix::F32);
		EXPECT_EQ(NumberAt(tokens, 4).Suffix, LiteralSuffix::None);

		// The suffix is not part of the literal text.
		EXPECT_EQ(NumberAt(tokens, 0).Text, "1");
		EXPECT_EQ(NumberAt(tokens, 3).Text, "1.0");
	}

	TEST(Lexer, FloatNeedsDigitsOnBothSides)
	{
		// Expect: '1.' is Int followed by Dot and '.5' is Dot followed by Int; only '3.14'
		// becomes a Float.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("1. .5 3.14", &diagnosticLabelList, false);

		ExpectTypes(tokens,
		            { TokenType::Int, TokenType::Dot, TokenType::Dot, TokenType::Int, TokenType::Float,
		              TokenType::Eof });
		EXPECT_EQ(NumberAt(tokens, 4).Text, "3.14");
	}

	TEST(Lexer, LeadingZeroIsError)
	{
		// Expect: a leading zero reports the numeric literal position error.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("012", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Eof });
		EXPECT_EQ(diagnosticLabelList.size(), 1);
		EXPECT_EQ(diagnosticLabelList[0].Code, DiagnosticCode::E00019_InvalidNumericLiteralPosition);
	}

	TEST(Lexer, GluedIdentifierIsError)
	{
		// Expect: a number immediately followed by an identifier reports the numeric literal
		// position error.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("123abc", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Eof });
		EXPECT_EQ(diagnosticLabelList.size(), 1);
		EXPECT_EQ(diagnosticLabelList[0].Code, DiagnosticCode::E00019_InvalidNumericLiteralPosition);
	}

	TEST(Lexer, UnterminatedStringIsErrorAndScanContinues)
	{
		// Expect: the unterminated string reports an error and scanning resumes on the next
		// line, producing the identifier.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("\"abc\nvar", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Ident, TokenType::Eof });
		EXPECT_EQ(diagnosticLabelList.size(), 1);
		EXPECT_EQ(diagnosticLabelList[0].Code, DiagnosticCode::E00010_UnterminatedStringLiteral);
	}

	TEST(Lexer, CommentsAreSkipped)
	{
		// Expect: line, block and documentation comments are skipped, leaving the two
		// identifiers.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens =
		    lexer.Tokenize("// line comment\n/* block\ncomment */ a /*b*/ c /// doc\n", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Ident, TokenType::Ident, TokenType::Eof });
		EXPECT_EQ(TextAt(tokens, 0), "a");
		EXPECT_EQ(TextAt(tokens, 1), "c");
	}

	TEST(Lexer, Utf8ColumnsCountCodePoints)
	{
		// Expect: columns count code points rather than bytes, so the final Eof sits at
		// column 14. Non-ASCII identifiers currently report an error, which this test ignores.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("int \xE4\xB8\xAD = \"\xE4\xB8\xAD\xE6\x96\x87\";", &diagnosticLabelList, false);

		ASSERT_GE(tokens.size(), 5);
		EXPECT_EQ(tokens[4].Type, TokenType::Eof);
		EXPECT_EQ(tokens[4].Source.Column, 14);
	}

	TEST(Lexer, BomAndCrLf)
	{
		// Expect: the BOM is skipped, and CRLF advances the line while keeping column 1.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("\xEF\xBB\xBFint a = 1;\r\nint b = 2;\r\n", &diagnosticLabelList, false);

		ASSERT_GE(tokens.size(), 11);
		EXPECT_EQ(tokens[0].Type, TokenType::Ident);
		EXPECT_EQ(tokens[0].Source.Line, 1);
		EXPECT_EQ(tokens[0].Source.Column, 1);
		EXPECT_EQ(tokens[0].Source.Offset, 3);

		EXPECT_EQ(tokens[5].Type, TokenType::Ident);
		EXPECT_EQ(tokens[5].Source.Line, 2);
		EXPECT_EQ(tokens[5].Source.Column, 1);
	}

	TEST(Lexer, VarIsNoLongerKeyword)
	{
		// Expect: 'var' is a plain identifier.
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("var", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Ident, TokenType::Eof });
		EXPECT_EQ(TextAt(tokens, 0), "var");
	}
} // namespace Pxc::PLang::Test
