// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once
#include "../Common/SourceView.hpp"

#include <string>
#include <string_view>
#include <variant>
#include <vector>

#define PXC_TOKEN_TYPE_LIST(X) \
	X(Int)                     \
	X(Float)                   \
	X(String)                  \
	X(FString)                 \
	X(Char)                    \
	X(Ident)                   \
                               \
	X(K_True)                  \
	X(K_False)                 \
	X(K_Null)                  \
                               \
	X(K_Module)                \
	X(K_Import)                \
	X(K_With)                  \
	X(K_Friend)                \
	X(K_Def)                   \
	X(K_Operator)              \
	X(K_Auto)                  \
	X(K_Const)                 \
	X(K_Class)                 \
	X(K_Interface)             \
	X(K_Enum)                  \
	X(K_Union)                 \
	X(K_Throw)                 \
	X(K_Try)                   \
	X(K_Catch)                 \
                               \
	X(K_Dyn)                   \
	X(K_Mov)                   \
	X(K_Ref)                   \
                               \
	X(K_Using)                 \
	X(K_Switch)                \
	X(K_Case)                  \
	X(K_Do)                    \
	X(K_Unsafe)                \
	X(K_Template)              \
	X(K_Concept)               \
	X(K_Requires)              \
	X(K_As)                    \
	X(K_Is)                    \
	X(K_Sizeof)                \
	X(K_Alignof)               \
	X(K_This)                  \
	X(K_New)                   \
                               \
	X(K_Public)                \
	X(K_Private)               \
	X(K_Protected)             \
	X(K_Internal)              \
                               \
	X(K_Virtual)               \
	X(K_Override)              \
	X(K_Abstract)              \
	X(K_Final)                 \
	X(K_Default)               \
	X(K_Delete)                \
	X(K_Noexcept)              \
                               \
	X(K_Static)                \
	X(K_Constexpr)             \
	X(K_Implicit)              \
                               \
	X(K_If)                    \
	X(K_Else)                  \
	X(K_While)                 \
	X(K_For)                   \
	X(K_Break)                 \
	X(K_Continue)              \
	X(K_Return)                \
                               \
	X(Plus)                    \
	X(Minus)                   \
	X(Star)                    \
	X(Slash)                   \
	X(Percent)                 \
	X(Amp)                     \
	X(Pipe)                    \
	X(Caret)                   \
	X(Tilde)                   \
	X(Bang)                    \
	X(Shl)                     \
	X(Shr)                     \
	X(ShrU)                    \
	X(Spaceship)               \
	X(Lt)                      \
	X(Le)                      \
	X(Gt)                      \
	X(Ge)                      \
	X(EqEq)                    \
	X(BangEq)                  \
	X(EqEqEq)                  \
	X(BangEqEq)                \
	X(AmpAmp)                  \
	X(PipePipe)                \
	X(Eq)                      \
	X(PlusEq)                  \
	X(MinusEq)                 \
	X(StarEq)                  \
	X(SlashEq)                 \
	X(PercentEq)               \
	X(AmpEq)                   \
	X(PipeEq)                  \
	X(CaretEq)                 \
	X(ShlEq)                   \
	X(ShrEq)                   \
	X(ShrUEq)                  \
	X(Arrow)                   \
	X(LParen)                  \
	X(RParen)                  \
	X(LBrace)                  \
	X(RBrace)                  \
	X(LBracket)                \
	X(RBracket)                \
	X(Semi)                    \
	X(Comma)                   \
	X(Dot)                     \
	X(Ellipsis)                \
	X(Colon)                   \
	X(ColonEq)                 \
	X(ColonColon)              \
	X(Question)                \
	X(QuestionDot)             \
	X(QuestionLBracket)        \
	X(QuestionQuestion)        \
	X(Eof)                     \
	X(Error)

namespace Pxc::PLang
{

	enum class TokenType
	{
#define PXC_ENUM_ITEM(NAME) NAME,
		PXC_TOKEN_TYPE_LIST(PXC_ENUM_ITEM)
#undef PXC_ENUM_ITEM
		    Count,
		BlockComment,
		LineComment,
	};

	enum class LiteralSuffix : U8
	{
		None,
		U32, // u
		I64, // l
		U64, // ul
		F32, // f
	};

	struct NumberLiteral
	{
		std::string_view Text;
		LiteralSuffix Suffix;
	};

	using TokenPayload = std::variant<std::monostate, NumberLiteral, std::string_view, std::string, char32_t>;

	struct Token
	{
		TokenType Type;
		TokenPayload Payload;
		SourceView Source;
	};

	using TokenList = std::vector<Token>;
} // namespace Pxc::PLang