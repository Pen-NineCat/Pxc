# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
# https://github.com/Pen-NineCat

"""把 DiagnosticCode.yaml 里的每个诊断码写成 DiagnosticCodeTable.hpp 里的一个枚举值。

生成结果（成员名 = ``<code>_<name>``，顺序与 yaml 完全一致）：

    enum class DiagnosticCode
    {
        E0001_ExpectedModuleDecl,
        E0002_UnknownModuleIdent,
        ...
    };

整个文件允许每次重写，唯独"文件开头 → #pragma once"那一段会从磁盘上的旧文件
原样取回；旧文件不存在（或没有 #pragma once）时用内置的 SPDX 头兜底。

不动的：
    * yaml 里的 name 一律照抄，脚本不做改名、大小写转换或去前缀；
    * 除枚举之外什么都不生成（level / message 暂不进 hpp）。

用法（可在任意目录执行，路径默认相对脚本自身）：

    # 就地重写 DiagnosticCodeTable.hpp
    python Code/PLang/Diagnostic/GenerateDiagnosticTable.py

    # 只校验是否与 yaml 同步；不一致返回 1，可挂到 CI / pre-commit
    python Code/PLang/Diagnostic/GenerateDiagnosticTable.py --check

    # 只把结果打到 stdout，不写盘
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

# 旧文件不可用时写进新文件的文件头（与仓库其余文件一致）
DEFAULT_HEADER = (
	"// SPDX-License-Identifier: Apache-2.0\n"
	"// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)\n"
	"// https://github.com/Pen-NineCat\n"
	"\n"
	"#pragma once\n"
)

RE_IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


def TakeHeader(existing: str) -> str:
	"""取出旧文件"第一行 → #pragma once"这一段（含结尾换行），作为新文件的文件头。

	@param existing 旧 hpp 的全文；文件不存在时传空串
	@return 可原样拼到新文件开头的文本；旧文件里找不到 #pragma once 时返回 DEFAULT_HEADER
	"""
	lines = existing.splitlines()
	for index, line in enumerate(lines):
		if line.strip() == "#pragma once":
			return "\n".join(lines[: index + 1]) + "\n"
	return DEFAULT_HEADER


def LoadMembers(yamlPath: Path) -> list[str]:
	"""读出 yaml 中全部条目，返回 ``<code>_<name>`` 形式的枚举成员名，保持 yaml 顺序。

	@param yamlPath 诊断码表 yaml 的路径
	@return 枚举成员名列表，至少一条
	@note code 重复或某条缺 code / name 时直接报错退出，不做猜测
	"""
	data = yaml.safe_load(yamlPath.read_text(encoding="utf-8"))
	if not isinstance(data, list) or not data:
		raise SystemExit(f"{yamlPath.name}: 顶层必须是至少含一条的列表")

	members: list[str] = []
	seenCode: dict[str, int] = {}
	for lineno, entry in enumerate(data, start=1):
		if not isinstance(entry, dict):
			raise SystemExit(f"{yamlPath.name}: 第 {lineno} 条不是 mapping")
		code = str(entry.get("code") or "").strip()
		name = str(entry.get("name") or "").strip()
		if not RE_IDENT.match(code):
			raise SystemExit(f"{yamlPath.name}: 第 {lineno} 条的 code `{code}` 不是合法标识符")
		if not RE_IDENT.match(name):
			raise SystemExit(f"{yamlPath.name}: 第 {lineno} 条的 name `{name}` 不是合法标识符")
		if code in seenCode:
			raise SystemExit(
				f"{yamlPath.name}: code `{code}` 重复（第 {lineno} 条与第 {seenCode[code]} 条）",
			)
		seenCode[code] = lineno
		members.append(f"{code}_{name}")
	return members


def Render(header: str, members: list[str]) -> str:
	"""把文件头 + 枚举拼成完整的 hpp 文本。

	@param header TakeHeader 返回的文件头，必须以换行结尾
	@param members LoadMembers 返回的成员名列表
	@return 新 hpp 的全文（LF 换行，以换行结尾）
	"""
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
	ap = argparse.ArgumentParser(description="按 DiagnosticCode.yaml 重新生成 DiagnosticCodeTable.hpp")
	ap.add_argument("--check", action="store_true", help="只校验，不写文件；与磁盘不一致时返回 1")
	ap.add_argument("--dry-run", action="store_true", help="只把结果打印到 stdout，不写文件")
	args = ap.parse_args()

	existing = HPP_PATH.read_text(encoding="utf-8") if HPP_PATH.exists() else ""
	members = LoadMembers(YAML_PATH)
	text = Render(TakeHeader(existing), members)

	if args.dry_run:
		sys.stdout.write(text)
		return 0

	if args.check:
		if text != existing:
			print(f"{HPP_PATH.name} 已过期，请重新运行本脚本", file=sys.stderr)
			return 1
		print(f"{HPP_PATH.name} 与 {YAML_PATH.name} 已同步（{len(members)} 条）")
		return 0

	if text == existing:
		print(f"{HPP_PATH.name} 无变化（{len(members)} 条）")
		return 0

	HPP_PATH.write_text(text, encoding="utf-8", newline="\n")
	print(f"{HPP_PATH.name} 已写入 {len(members)} 条")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
