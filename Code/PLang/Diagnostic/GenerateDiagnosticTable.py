# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
# https://github.com/Pen-NineCat

""" Generate hpp table based on the source yaml.
	Result:
    enum class DiagnosticCode
    {
        E0001_ExpectedModuleDecl,
        E0002_UnknownModuleIdent,
        ...
    };
Usage:
    # Edit DiagnosticCodeTable.hpp in place
    python Code/PLang/Diagnostic/GenerateDiagnosticTable.py

    # Verify the source yaml and generated hpp file; If it's incompatible, return 1.
    python Code/PLang/Diagnostic/GenerateDiagnosticTable.py --check

    # Just print to stdout
    python Code/PLang/Diagnostic/GenerateDiagnosticTable.py --dry-run
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import yaml

HERE = Path(__file__).resolve().parent
YAML_PATH = HERE / "DiagnosticCode.yaml"
HPP_PATH = HERE / "DiagnosticCodeTable.hpp"

# If target file is not found(new or deleted), copy SPDX header.
DEFAULT_HEADER = (
	"// SPDX-License-Identifier: Apache-2.0\n"
	"// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)\n"
	"// https://github.com/Pen-NineCat\n"
	"\n"
	"#pragma once\n"
)

RE_IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


def TakeHeader(existing: str) -> str:
	lines = existing.splitlines()
	for index, line in enumerate(lines):
		if line.strip() == "#pragma once":
			return "\n".join(lines[: index + 1]) + "\n"
	return DEFAULT_HEADER


def LoadMembers(yamlPath: Path) -> list[str]:
	"read yaml"

	data = yaml.safe_load(yamlPath.read_text(encoding="utf-8"))
	if not isinstance(data, list) or not data:
		raise SystemExit(f"{yamlPath.name}: the top level has to be a list with at least one entry")

	members: list[str] = []
	seenCode: dict[str, int] = {}
	for lineno, entry in enumerate(data, start=1):
		if not isinstance(entry, dict):
			raise SystemExit(f"{yamlPath.name}: entry {lineno} is not a mapping")
		code = str(entry.get("code") or "").strip()
		name = str(entry.get("name") or "").strip()
		if not RE_IDENT.match(code):
			raise SystemExit(f"{yamlPath.name}: entry {lineno} has code `{code}`, which is not a valid identifier")
		if not RE_IDENT.match(name):
			raise SystemExit(f"{yamlPath.name}: entry {lineno} has name `{name}`, which is not a valid identifier")
		if code in seenCode:
			raise SystemExit(
				f"{yamlPath.name}: code `{code}` is duplicated (entry {lineno} and entry {seenCode[code]})",
			)
		seenCode[code] = lineno
		members.append(f"{code}_{name}")
	return members


def Render(header: str, members: list[str]) -> str:
	body = "".join(f"\t\t{member},\n" for member in members)
	return (
		f"{header}"
		"\n"
		"namespace Pxc::PLang\n"
		"{\n"
		"\tenum class DiagnosticCode\n"
		"\t{\n"
		f"{body}"
		"\t};\n"
		"} // namespace Pxc::PLang\n"
	)


def main() -> int:
	# Windows runners do not use a UTF-8 console encoding (cp1252 there), so any non-ASCII output
	# would raise UnicodeEncodeError and fail the CMake configure step. Pin both streams instead of
	# depending on the locale of the machine that runs the script.
	sys.stdout.reconfigure(encoding="utf-8", errors="replace")
	sys.stderr.reconfigure(encoding="utf-8", errors="replace")

	ap = argparse.ArgumentParser(description="Regenerate DiagnosticCodeTable.hpp from DiagnosticCode.yaml")
	ap.add_argument("--check", action="store_true", help="only verify, do not write; return 1 if the file on disk differs")
	ap.add_argument("--dry-run", action="store_true", help="only print the result to stdout, do not write the file")
	args = ap.parse_args()

	existing = HPP_PATH.read_text(encoding="utf-8") if HPP_PATH.exists() else ""
	members = LoadMembers(YAML_PATH)
	text = Render(TakeHeader(existing), members)

	if args.dry_run:
		sys.stdout.write(text)
		return 0

	if args.check:
		if text != existing:
			print(f"{HPP_PATH.name} is out of date, rerun this script", file=sys.stderr)
			return 1
		print(f"{HPP_PATH.name} is in sync with {YAML_PATH.name} ({len(members)} entries)")
		return 0

	if text == existing:
		print(f"{HPP_PATH.name} is unchanged ({len(members)} entries)")
		return 0

	HPP_PATH.write_text(text, encoding="utf-8", newline="\n")
	print(f"{HPP_PATH.name} written with {len(members)} entries")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
