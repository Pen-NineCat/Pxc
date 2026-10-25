# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
# https://github.com/Pen-NineCat

"""在 Pxc 规范文档里插入一个章节，并自动重排其后的所有编号。

做三件事：
    1. 把插入点之后所有 ``## N.`` 的 N 加 1、``### N.M`` 的 N 加 1；
    2. 把插入点之后所有内部交叉引用 ``§N`` / ``§N.M`` 的 N 加 1；
    3. 在插入点放入新章节，其子节按 N.1、N.2… 自动编号。

不动的：
    * 别的文档的引用（``Lexical-Spec §5``、``[Grammar-Spec.md](...) §4`` 之类）；
    * 代码围栏里的内容；
    * 正文措辞 —— 本脚本只碰"编号"。

用法（在仓库根目录执行）：
    # 先预览：在现有 §4 之前插入新的一节，正文由 body.md 提供
    python Tools/insert_section.py Code/PLang/Docs/Semantics-Spec.md \
        --at 4 --title "类型别名" --body body.md --dry-run

    # 确认无误后去掉 --dry-run 真正写入（默认先备份 .bak）
    python Tools/insert_section.py Code/PLang/Docs/Semantics-Spec.md \
        --at 4 --title "类型别名" --body body.md

body.md 的写法：
    * 普通 Markdown，**不要**写 ``## N.`` 标题；
    * 子节写成 ``### 标题``（不带编号），脚本会自动编号为 N.1、N.2…；
      若已写成 ``### N.M 标题``，则原样保留。

外部引用的识别规则（唯一需要判断的地方）：
    * 该 § 必须紧挨着某个 ``*-Spec`` 文档名（中间不超过 40 个字符）；
    * 且中间不含全角 ``）`` —— 本文的内部引用一律写成 ``（§N.M）``，
      而 Markdown 链接里的半角 ``)`` 不算分界。
    脚本每次都会逐条打印被跳过的外部引用；若发现漏改，用
    ``--assume-all-internal`` 重跑即可。
"""

from __future__ import annotations

import argparse
import re
import shutil
import sys
from pathlib import Path

# ## 12. 标题
RE_H2 = re.compile(r"^## (\d+)\.\s(.*)$")
# ### 12.3 标题
RE_H3 = re.compile(r"^### (\d+)\.(\d+)\s(.*)$")
# 不带编号的子节标题：### 标题
RE_H3_PLAIN = re.compile(r"^###\s+(?![0-9]+\.)(.+)$")
# 内部引用：§12 或 §12.3
RE_REF = re.compile(r"§(\d+)(?:\.(\d+))?")
# 代码围栏
RE_FENCE = re.compile(r"^\s*(```|~~~)")
# 别的文档的名字
RE_FOREIGN_DOC = re.compile(r"(?:Lexical|Grammar|IR|Library|Semantics)-Spec(?:\.md)?")


def fenced_mask(lines: list[str]) -> list[bool]:
	"""标出每个行是否位于代码围栏内。"""
	mask: list[bool] = []
	inside = False
	for line in lines:
		if RE_FENCE.match(line):
			mask.append(True)
			inside = not inside
			continue
		mask.append(inside)
	return mask


def is_external_ref(line: str, pos: int) -> bool:
	"""判断 line 中下标 pos 处的 § 是不是在引用别的文档。

	判据：这个 § 必须**紧挨着**某个文档名，且中间没有全角 ） 隔着。
	例如 ``（Lexical-Spec §4）`` 或 ``| [Grammar-Spec.md](…) | §4 …``。
	一旦中间出现全角 ） ，说明前一个文档名已在括号里收尾，
	后面的 § 就回到本文自己的引用。
	"""
	doc = None
	for m in RE_FOREIGN_DOC.finditer(line[:pos]):
		doc = m
	if doc is None:
		return False
	between = line[doc.end():pos]
	# 只看全角右括号：Markdown 链接里的半角 ) 不能当作分界，
	# 否则附录 A 的表格会全部误判。
	if "）" in between:
		return False
	# 距离太远也不算（防止一句话里先提文档名、隔很远再写 §N）
	return len(between.strip()) <= 40


def scan_sections(lines: list[str], mask: list[bool]) -> list[tuple[int, int, str]]:
	"""返回 [(节号, 行下标, 标题)]。"""
	out: list[tuple[int, int, str]] = []
	for i, line in enumerate(lines):
		if mask[i]:
			continue
		m = RE_H2.match(line)
		if m:
			out.append((int(m.group(1)), i, m.group(2)))
	return out


def shift_ref(m: re.Match[str], at: int) -> str:
	"""把匹配到的引用编号 +1（若 >= at）。"""
	major = int(m.group(1))
	if major < at:
		return m.group(0)
	minor = m.group(2)
	if minor is None:
		return f"§{major + 1}"
	return f"§{major + 1}.{minor}"


def rewrite(
	lines: list[str],
	mask: list[bool],
	at: int,
	assume_internal: bool,
) -> tuple[list[str], list[str], list[str]]:
	"""重排标题与引用。返回 (新行, 改动的引用报告, 跳过的外部引用报告)。"""
	out: list[str] = []
	changed: list[str] = []
	skipped: list[str] = []

	for i, line in enumerate(lines):
		if mask[i]:
			out.append(line)
			continue

		# 二级标题
		m2 = RE_H2.match(line)
		if m2:
			n = int(m2.group(1))
			out.append(line if n < at else f"## {n + 1}. {m2.group(2)}")
			continue

		# 三级标题
		m3 = RE_H3.match(line)
		if m3:
			n = int(m3.group(1))
			out.append(line if n < at else f"### {n + 1}.{m3.group(2)} {m3.group(3)}")
			continue

		# 正文里的引用
		def sub(m: re.Match[str]) -> str:
			major = int(m.group(1))
			if major < at:
				return m.group(0)
			if not assume_internal and is_external_ref(line, m.start()):
				skipped.append(f"L{i + 1}: {m.group(0)}  ← {line.strip()[:60]}")
				return m.group(0)
			new = shift_ref(m, at)
			changed.append(f"L{i + 1}: {m.group(0)} → {new}")
			return new

		out.append(RE_REF.sub(sub, line))

	return out, changed, skipped


def build_block(at: int, title: str, body_lines: list[str]) -> list[str]:
	"""拼出新章节的文本块，给不带编号的 ### 自动编号。"""
	block: list[str] = [f"## {at}. {title}", ""]
	sub = 0
	for line in body_lines:
		m = RE_H3_PLAIN.match(line)
		if m:
			sub += 1
			block.append(f"### {at}.{sub} {m.group(1)}")
			continue
		block.append(line)
	while block and block[-1] == "":
		block.pop()
	block.append("")
	return block


def main() -> int:
	ap = argparse.ArgumentParser(description="插入章节并重排其后的编号")
	ap.add_argument("path", type=Path, help="要修改的 Markdown 文件")
	ap.add_argument("--at", type=int, required=True, help="新章节的编号；现有该号及之后的章节整体 +1")
	ap.add_argument("--title", required=True, help="新章节的标题")
	ap.add_argument("--body", type=Path, help="新章节正文（不含 ## N. 标题）")
	ap.add_argument("--dry-run", action="store_true", help="只报告，不写文件")
	ap.add_argument(
		"--assume-all-internal",
		action="store_true",
		help="把所有 § 都当本文引用（跳过外部引用识别）",
	)
	ap.add_argument("--no-backup", action="store_true", help="不做 .bak 备份")
	args = ap.parse_args()

	text = args.path.read_text(encoding="utf-8")
	lines = text.splitlines()
	mask = fenced_mask(lines)

	sections = scan_sections(lines, mask)
	nums = [n for n, _, _ in sections]
	if nums != list(range(1, len(nums) + 1)):
		print("警告：现有节号不是 1..N 连续的，脚本仍会按 >= --at 的规则处理。", file=sys.stderr)

	if args.at < 1 or args.at > len(nums) + 1:
		print(f"错误：--at 必须在 1..{len(nums) + 1} 之间（现有 {len(nums)} 节）", file=sys.stderr)
		return 2

	body_lines: list[str] = []
	if args.body:
		body_lines = args.body.read_text(encoding="utf-8").splitlines()

	new_lines, changed, skipped = rewrite(lines, mask, args.at, args.assume_all_internal)

	# 插入点：原文件里第一个节号 == --at 的标题行；若 --at 超过最后一节，则追加到末尾
	insert_at = next((i for n, i, _ in sections if n == args.at), len(new_lines))
	block = build_block(args.at, args.title, body_lines)
	new_lines[insert_at:insert_at] = block

	print(f"插入点：原文件第 {insert_at + 1} 行之前")
	print(f"标题重排：{len([n for n in nums if n >= args.at])} 节")
	print(f"引用重排：{len(changed)} 处")
	if skipped:
		print(f"跳过（外部引用）：{len(skipped)} 处")
		for s in skipped:
			print(f"    {s}")

	if args.dry_run:
		print("\n--- 预览（dry-run，未写文件）---")
		for line in block:
			print(line)
		return 0

	if not args.no_backup:
		backup = args.path.with_suffix(args.path.suffix + ".bak")
		shutil.copy2(args.path, backup)
		print(f"\n已备份：{backup}")

	args.path.write_text("\n".join(new_lines) + "\n", encoding="utf-8")
	print(f"已写入：{args.path}")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
