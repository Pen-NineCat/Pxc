// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once

namespace Pxc::PLang
{
	enum class DiagnosticCode
	{
		E0006_UndefinedSymbol,
		E00010_UnterminatedStringLiteral,
		E00014_UnterminatedCharLiteral,
		E00015_InvalidCharCodepoint,
		E00016_InvalidEscapeSequences,
		E00017_UnterminatedCommentLiteral,
		E00018_InvalidNumericSeparatorsPosition,
		E00019_InvalidNumericLiteralPosition,
		E00020_InvalidNonASCIICodepointPosition,
		E0100_ModuleMustBeFirst,
		E0101_ExpectSemi,
		E0102_ExpectCharacterNotFound,
		E0103_ExclusiveTypeQualifiers,
		E0104_DuplicateTypeQualifiers,
		E0106_PleaseInputIdent,
		E0107_ImportMustBeAtHeader,
		E0108_InvalidNullableType,
		E0109_UnclosedTemplateArgumentList,
		E0110_ExpectType,
		E0111_InvalidTypeQualifierOrder,
		E0112_UnmatchedGreater,
		E0113_InvalidImportPrefixOrder,
	};
} // namespace Pxc::PLang
