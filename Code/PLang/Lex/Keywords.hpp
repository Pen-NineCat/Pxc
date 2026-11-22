// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once
#include "Token.hpp"

#include "PxcCommon/StrUnorderedMap.hpp"
#include <array>
#include <string>
#include <string_view>

namespace Pxc::PLang
{
	const static inline StrUnorderedMap<TokenType> g_kwMap = {
		{ "true", TokenType::K_True },
		{ "false", TokenType::K_False },
		{ "null", TokenType::K_Null },
		{ "module", TokenType::K_Module },
		{ "import", TokenType::K_Import },
		{ "with", TokenType::K_With },
		{ "friend", TokenType::K_Friend },
		{ "def", TokenType::K_Def },
		{ "operator", TokenType::K_Operator },
		{ "auto", TokenType::K_Auto },
		{ "const", TokenType::K_Const },
		{ "class", TokenType::K_Class },
		{ "interface", TokenType::K_Interface },
		{ "enum", TokenType::K_Enum },
		{ "union", TokenType::K_Union },
		{ "throw", TokenType::K_Throw },
		{ "try", TokenType::K_Try },
		{ "catch", TokenType::K_Catch },
		{ "dyn", TokenType::K_Dyn },
		{ "mov", TokenType::K_Mov },
		{ "ref", TokenType::K_Ref },
		{ "using", TokenType::K_Using },
		{ "switch", TokenType::K_Switch },
		{ "case", TokenType::K_Case },
		{ "do", TokenType::K_Do },
		{ "unsafe", TokenType::K_Unsafe },
		{ "template", TokenType::K_Template },
		{ "concept", TokenType::K_Concept },
		{ "requires", TokenType::K_Requires },
		{ "as", TokenType::K_As },
		{ "is", TokenType::K_Is },
		{ "sizeof", TokenType::K_Sizeof },
		{ "alignof", TokenType::K_Alignof },
		{ "this", TokenType::K_This },
		{ "new", TokenType::K_New },
		{ "public", TokenType::K_Public },
		{ "private", TokenType::K_Private },
		{ "protected", TokenType::K_Protected },
		{ "internal", TokenType::K_Internal },
		{ "virtual", TokenType::K_Virtual },
		{ "override", TokenType::K_Override },
		{ "abstract", TokenType::K_Abstract },
		{ "final", TokenType::K_Final },
		{ "default", TokenType::K_Default },
		{ "delete", TokenType::K_Delete },
		{ "noexcept", TokenType::K_Noexcept },
		{ "static", TokenType::K_Static },
		{ "constexpr", TokenType::K_Constexpr },
		{ "implicit", TokenType::K_Implicit },
		{ "if", TokenType::K_If },
		{ "else", TokenType::K_Else },
		{ "while", TokenType::K_While },
		{ "for", TokenType::K_For },
		{ "break", TokenType::K_Break },
		{ "continue", TokenType::K_Continue },
		{ "return", TokenType::K_Return },
	};

	using BinrayPair = std::pair<std::string_view, TokenType>;

	constexpr auto BinrayPairList = std::to_array({ // 4 字符
	                                                BinrayPair{ ">>>=", TokenType::ShrUEq },

	                                                // 3 字符
	                                                BinrayPair{ "<<=", TokenType::ShlEq },
	                                                BinrayPair{ ">>=", TokenType::ShrEq },
	                                                BinrayPair{ "<=>", TokenType::Spaceship },
	                                                BinrayPair{ ">>>", TokenType::ShrU },
	                                                BinrayPair{ "===", TokenType::EqEqEq },
	                                                BinrayPair{ "!==", TokenType::BangEqEq },
	                                                BinrayPair{ "...", TokenType::Ellipsis },

	                                                // 2 字符
	                                                BinrayPair{ "<<", TokenType::Shl },
	                                                BinrayPair{ ">>", TokenType::Shr },
	                                                BinrayPair{ "==", TokenType::EqEq },
	                                                BinrayPair{ "!=", TokenType::BangEq },
	                                                BinrayPair{ "<=", TokenType::Le },
	                                                BinrayPair{ ">=", TokenType::Ge },
	                                                BinrayPair{ "+=", TokenType::PlusEq },
	                                                BinrayPair{ "-=", TokenType::MinusEq },
	                                                BinrayPair{ "*=", TokenType::StarEq },
	                                                BinrayPair{ "/=", TokenType::SlashEq },
	                                                BinrayPair{ "%=", TokenType::PercentEq },
	                                                BinrayPair{ "&=", TokenType::AmpEq },
	                                                BinrayPair{ "|=", TokenType::PipeEq },
	                                                BinrayPair{ "^=", TokenType::CaretEq },
	                                                BinrayPair{ "&&", TokenType::AmpAmp },
	                                                BinrayPair{ "||", TokenType::PipePipe },
	                                                BinrayPair{ "??", TokenType::QuestionQuestion },
	                                                BinrayPair{ ":=", TokenType::ColonEq },
	                                                BinrayPair{ "::", TokenType::ColonColon },
	                                                BinrayPair{ "->", TokenType::Arrow },
	                                                BinrayPair{ "?.", TokenType::QuestionDot },
	                                                BinrayPair{ "?[", TokenType::QuestionLBracket },

	                                                // 1 字符
	                                                BinrayPair{ "+", TokenType::Plus },
	                                                BinrayPair{ "-", TokenType::Minus },
	                                                BinrayPair{ "*", TokenType::Star },
	                                                BinrayPair{ "/", TokenType::Slash },
	                                                BinrayPair{ "%", TokenType::Percent },
	                                                BinrayPair{ "&", TokenType::Amp },
	                                                BinrayPair{ "|", TokenType::Pipe },
	                                                BinrayPair{ "^", TokenType::Caret },
	                                                BinrayPair{ "~", TokenType::Tilde },
	                                                BinrayPair{ "!", TokenType::Bang },
	                                                BinrayPair{ "<", TokenType::Lt },
	                                                BinrayPair{ ">", TokenType::Gt },
	                                                BinrayPair{ "=", TokenType::Eq },
	                                                BinrayPair{ "(", TokenType::LParen },
	                                                BinrayPair{ ")", TokenType::RParen },
	                                                BinrayPair{ "{", TokenType::LBrace },
	                                                BinrayPair{ "}", TokenType::RBrace },
	                                                BinrayPair{ "[", TokenType::LBracket },
	                                                BinrayPair{ "]", TokenType::RBracket },
	                                                BinrayPair{ ";", TokenType::Semi },
	                                                BinrayPair{ ",", TokenType::Comma },
	                                                BinrayPair{ ".", TokenType::Dot },
	                                                BinrayPair{ ":", TokenType::Colon },
	                                                BinrayPair{ "?", TokenType::Question } });
} // namespace Pxc::PLang
