// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#include "PLang/Lex/Lexer.h"
#include "PLang/Lex/Token.hpp"
#include <gtest/gtest.h>
#include <initializer_list>
#include <string_view>
#include <variant>
#include <vector>

namespace Pxc::PLang::Test
{
	namespace
	{
		/// @brief 断言 Token 类型序列与期望完全一致（含末尾 Eof）
		void ExpectTypes(const TokenList& tokens, std::initializer_list<TokenType> expected)
		{
			ASSERT_EQ(tokens.size(), expected.size());

			Usize index = 0;
			for (const TokenType type : expected)
			{
				EXPECT_EQ(tokens[index].Type, type) << "第 " << index << " 个 Token";
				++index;
			}
		}

		/// @brief 取指定下标 Token 的字符串切片载荷
		std::string_view TextAt(const std::vector<Token>& tokens, Usize index)
		{
			return std::get<std::string_view>(tokens[index].Payload);
		}

		/// @brief 取指定下标 Token 的数值字面量载荷
		NumberLiteral NumberAt(const std::vector<Token>& tokens, Usize index)
		{
			return std::get<NumberLiteral>(tokens[index].Payload);
		}
	} // namespace

	TEST(Lexer, SampleFromAgentsMd)
	{
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
		                                   "\tstd.io.println(f\"Hello,world,{a}\");\n"
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
		              TokenType::FString, TokenType::RParen, TokenType::Semi, TokenType::K_Return, TokenType::Int,
		              TokenType::Semi, TokenType::RBrace, TokenType::Eof });

		EXPECT_EQ(TextAt(tokens, 33), "println");
		EXPECT_EQ(TextAt(tokens, 35), "Hello,world,{a}");
	}

	TEST(Lexer, ArrowIsSingleToken)
	{
		Lexer lexer;

		DiagnosticLabelList diagnosticLabelList;
		const TokenList tokens = lexer.Tokenize("->", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Arrow, TokenType::Eof });
		EXPECT_EQ(tokens[0].Source.Length, 2);
	}

	TEST(Lexer, FStringIsSingleToken)
	{
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("f\"Hello,{a}\"", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::FString, TokenType::Eof });
		EXPECT_EQ(TextAt(tokens, 0), "Hello,{a}");
	}

	TEST(Lexer, FStringWithNestedStringInHole)
	{
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize(R"(f"{d["k"]}")", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::FString, TokenType::Eof });
		EXPECT_EQ(TextAt(tokens, 0), "{d[\"k\"]}");
	}

	TEST(Lexer, StringKeepsRawEscapeSequence)
	{
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize(R"("ab\tc")", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::String, TokenType::Eof });
		EXPECT_EQ(TextAt(tokens, 0), "ab\\tc");
	}

	TEST(Lexer, CharDecodesCodePoint)
	{
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
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("1_000_000", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Int, TokenType::Eof });
		EXPECT_EQ(NumberAt(tokens, 0).Text, "1_000_000");
	}

	TEST(Lexer, NumberSuffixes)
	{
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

		// 后缀不属于原文
		EXPECT_EQ(NumberAt(tokens, 0).Text, "1");
		EXPECT_EQ(NumberAt(tokens, 3).Text, "1.0");
	}

	TEST(Lexer, FloatNeedsDigitsOnBothSides)
	{
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
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("012", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Eof });
		EXPECT_EQ(diagnosticLabelList.size(), 1);
		EXPECT_EQ(diagnosticLabelList[0].Code, DiagnosticCode::E00019_InvalidNumericLiteralPosition);
	}

	TEST(Lexer, GluedIdentifierIsError)
	{
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("123abc", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Eof });
		EXPECT_EQ(diagnosticLabelList.size(), 1);
		EXPECT_EQ(diagnosticLabelList[0].Code, DiagnosticCode::E00019_InvalidNumericLiteralPosition);
	}

	TEST(Lexer, UnterminatedStringIsErrorAndScanContinues)
	{
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("\"abc\nvar", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Ident, TokenType::Eof });
		EXPECT_EQ(diagnosticLabelList.size(), 1);
		EXPECT_EQ(diagnosticLabelList[0].Code, DiagnosticCode::E00010_UnterminatedStringLiteral);
	}

	TEST(Lexer, CommentsAreSkipped)
	{
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("// 行注释\n/* 块\n注释 */ a /*b*/ c /// 文档\n", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Ident, TokenType::Ident, TokenType::Eof });
		EXPECT_EQ(TextAt(tokens, 0), "a");
		EXPECT_EQ(TextAt(tokens, 1), "c");
	}

	TEST(Lexer, Utf8ColumnsCountCodePoints)
	{
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("int \xE4\xB8\xAD = \"\xE4\xB8\xAD\xE6\x96\x87\";", &diagnosticLabelList, false);

		// 非 ASCII 标识符当前报 Error，但列号必须按码点推进：'中' 占 1 列，字符串内的两个汉字各占 1 列
		ASSERT_GE(tokens.size(), 5);
		EXPECT_EQ(tokens[4].Type, TokenType::Eof);
		EXPECT_EQ(tokens[4].Source.Column, 14);
	}

	TEST(Lexer, BomAndCrLf)
	{
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
		Lexer lexer;
		DiagnosticLabelList diagnosticLabelList;
		const auto tokens = lexer.Tokenize("var", &diagnosticLabelList, false);

		ExpectTypes(tokens, { TokenType::Ident, TokenType::Eof });
		EXPECT_EQ(TextAt(tokens, 0), "var");
	}
} // namespace Pxc::PLang::Test
