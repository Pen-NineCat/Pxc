# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
# https://github.com/Pen-NineCat

"""按 Token.hpp 的 PXC_TOKEN_TYPE_LIST 重新生成 Keywords.hpp 里的 g_kwMap。

取 ``PXC_TOKEN_TYPE_LIST`` 里所有 ``K_`` 开头的项，每项生成一条

    { "<去掉 K_ 后全小写>", TokenType::<项名> },

放进 ``g_kwMap`` 的初始化块；顺序与 Token.hpp 的宏列表完全一致。

只重写 g_kwMap 的**大括号内部**：文件头 / #include / BinrayPairList 一字节不碰。

用法（可在任意目录执行，路径默认相对脚本自身）：

    # 就地重写 Keywords.hpp
    python Code/PLang/Lex/GenerateKeywordMap.py

    # 只校验是否与 Token.hpp 同步；不一致返回 1，可挂到 CI / pre-commit
    python Code/PLang/Lex/GenerateKeywordMap.py --check

    # 只把结果打到 stdout，不写盘
    python Code/PLang/Lex/GenerateKeywordMap.py --dry-run
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
TOKEN_PATH = HERE / "Token.hpp"
KEYWORDS_PATH = HERE / "Keywords.hpp"

# 词形例外表：K_xxx 推不出正确关键字词形时在此登记
# 键 = TokenType 里的项名，值 = 真正的关键字
WORD_OVERRIDE: dict[str, str] = {}

RE_MACRO_DEF = re.compile(r"^#define\s+PXC_TOKEN_TYPE_LIST\s*\(\s*X\s*\)\s*\\?\s*$")
RE_ITEM = re.compile(r"^\s*X\(\s*(\w+)\s*\)\s*\\?\s*$")
RE_MAP_DECL = re.compile(r"^([ \t]*)const static inline StrUnorderedMap<TokenType> g_kwMap = \{[ \t]*$")


def WordOf(name: str) -> str:
	"""去掉 K_ 前缀并转小写，得到关键字词形。

	@param name TokenType 的项名，如 ``K_Constexpr``
	@return 关键字词形，如 ``constexpr``；例外见 WORD_OVERRIDE
	"""
	return WORD_OVERRIDE.get(name, name[len("K_") :].lower())


def LoadKeywordEntries(tokenPath: Path) -> list[tuple[str, str]]:
	"""从 Token.hpp 的 PXC_TOKEN_TYPE_LIST 里取出全部 K_ 项。

	@param tokenPath Token.hpp 的路径
	@return ``(项名, 关键字词形)`` 列表，顺序与宏列表一致
	@note 两个 K_ 项推出同一个词形时报错退出，不猜该保留哪个
	"""
	lines = tokenPath.read_text(encoding="utf-8").splitlines()

	start = None
	for index, line in enumerate(lines):
		if RE_MACRO_DEF.match(line):
			start = index
			break
	if start is None:
		raise SystemExit(f"{tokenPath.name}: 找不到 #define PXC_TOKEN_TYPE_LIST(X)")

	items: list[str] = []
	for line in lines[start + 1 :]:
		if match := RE_ITEM.match(line):
			items.append(match.group(1))
		# 宏条目以行尾 \ 续行，遇到不续行的就是列表结束
		if not line.rstrip().endswith("\\"):
			break

	entries: list[tuple[str, str]] = []
	seenWord: dict[str, str] = {}
	for name in (item for item in items if item.startswith("K_")):
		word = WordOf(name)
		if word in seenWord:
			raise SystemExit(f"{tokenPath.name}: 关键字 `{word}` 同时对应 {seenWord[word]} 与 {name}")
		seenWord[word] = name
		entries.append((name, word))
	return entries


def ReplaceMapBody(text: str, entries: list[tuple[str, str]]) -> str:
	"""把 Keywords.hpp 里 g_kwMap 的大括号内部整体换掉，其余部分原样保留。

	@param text 现有 Keywords.hpp 的全文
	@param entries ``(项名, 关键字词形)`` 列表
	@return 替换后的全文（LF 换行，以换行结尾）
	"""
	lines = text.splitlines()

	declIndex = None
	indent = ""
	for index, line in enumerate(lines):
		if match := RE_MAP_DECL.match(line):
			declIndex = index
			indent = match.group(1)
			break
	if declIndex is None:
		raise SystemExit("Keywords.hpp: 找不到 `const static inline StrUnorderedMap<TokenType> g_kwMap = {`")

	endIndex = None
	for index in range(declIndex + 1, len(lines)):
		if lines[index].strip() == "};":
			endIndex = index
			break
	if endIndex is None:
		raise SystemExit("Keywords.hpp: g_kwMap 的初始化块没有闭合的 `};`")

	bodyIndent = indent + "\t"
	body = [f'{bodyIndent}{{ "{word}", TokenType::{name} }},' for name, word in entries]
	lines[declIndex + 1 : endIndex] = body
	return "\n".join(lines) + "\n"


def main() -> int:
	ap = argparse.ArgumentParser(description="按 Token.hpp 的 PXC_TOKEN_TYPE_LIST 重新生成 Keywords.hpp 的 g_kwMap")
	ap.add_argument("--check", action="store_true", help="只校验，不写文件；与磁盘不一致时返回 1")
	ap.add_argument("--dry-run", action="store_true", help="只把结果打印到 stdout，不写文件")
	args = ap.parse_args()

	existing = KEYWORDS_PATH.read_text(encoding="utf-8")
	entries = LoadKeywordEntries(TOKEN_PATH)
	text = ReplaceMapBody(existing, entries)

	if args.dry_run:
		sys.stdout.write(text)
		return 0

	if args.check:
		if text != existing:
			print(f"{KEYWORDS_PATH.name} 已过期，请重新运行本脚本", file=sys.stderr)
			return 1
		print(f"{KEYWORDS_PATH.name} 与 {TOKEN_PATH.name} 已同步（{len(entries)} 条）")
		return 0

	if text == existing:
		print(f"{KEYWORDS_PATH.name} 无变化（{len(entries)} 条）")
		return 0

	KEYWORDS_PATH.write_text(text, encoding="utf-8", newline="\n")
	print(f"{KEYWORDS_PATH.name} 已写入 {len(entries)} 条")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
