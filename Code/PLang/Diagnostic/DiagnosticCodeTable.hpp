// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once

namespace Pxc::PLang
{
	enum class DiagnosticCode
	{
		E0001_ExpectedModuleDecl,
		E0002_UndefinedModuleIdent,
		E0003_ExpectSemi,
		E0004_UndefinedFunctionIdent,
		E0005_ExpectCharacterNotFound,
		E0006_UndefinedSymbol,
		E0007_ExclusiveTypeQualifiers,
		E0008_DuplicateTypeQualifiers,
		E0009_UndefinedCharcter,
		E00010_UnterminatedStringLiteral,
		E00011_PleaseInputDecl,
		E00012_ModuleMustBeAtHeader,
		E00013_PleaseInputIdent,
		E00014_UnterminatedCharLiteral,
		E00015_InvalidCharCodepoint,
		E00016_InvalidEscapeSequences,
		E00017_UnterminatedCommentLiteral,
		E00018_InvalidNumericSeparatorsPosition,
		E00019_InvalidNumericLiteralPosition,
		E00020_InvalidNonASCIICodepointPosition,
		E00021_ImportMustBeAtHeader,
	};
} // namespace Pxc::PLang
