#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
# https://github.com/Pen-NineCat
"""mdBook 预处理器：把 Pxc 规范里的自定义写法展开成 HTML。

它做四件事，对应 Rust Reference 的 `tools/mdbook-spec`：

1. 规则 ID —— 单独一行 `r[storage.kinds]`，渲染成正文左边距里的灰色小标签，
   锚点为 `#r-storage.kinds`。规则 ID 是引用锚点，不随章节重排而失效。
   中英两版同一条规则用同一个 ID（标识符本身是英文），所以 ID 是跨语言对齐的。

2. 提示块 —— `> [!WARNING]` 起头的引用块，渲染成无背景、只有 4px 左色条的
   alert。支持 NOTE / WARNING / EXAMPLE / INCOMPLETE / STALE 五种。

3. 跨文件规则引用 —— 链接定义的目标若是一个已存在的规则 ID

       [三种存储期]: storage.kinds

   就展开成 `storage.html#r-storage.kinds`。同一个 ID 在两种语言里都有时，
   优先取本语言那一份，正文因此不需要写语言目录。

4. 语言切换 —— 每页右上角注入「中文 / English」切换链接，按去掉语言目录后的
   相对路径配对；配对页不存在时只显示当前语言。

另外做两项检查，都在 stderr 输出：某种语言缺了另一种语言有的规则 ID 时报一行汇总；
链接定义的目标形如规则 ID 却查不到时（多半是笔误）也报一行汇总，并按原样保留，
免得渲染出一个点开即 404 的坏链。

代码围栏内的内容原样保留，也不参与规则 ID 的收集——示例里写的 `r[xxx]` 不会变成
一条真规则（那会给出一个有锚点却页面上不存在的引用目标）。
"""

import json
import posixpath
import re
import sys

# 语言目录名，顺序即切换链接的顺序
LANGS = ("zh", "en")

# 规则 ID → 该条目所在的章节路径（一个 ID 可在多种语言里各有一份）
RULE_RE = re.compile(
    r"(?m)^[ \t]*r\[(?P<id>[a-z][a-z0-9_-]*(?:\.[a-z0-9_-]+)*)\][ \t]*\r?$"
)

# `> [!KIND]` + 其后连续的引用行
ADM_RE = re.compile(
    r"(?m)^(?P<ind>[ ]*)> \[!(?P<kind>[A-Za-z][A-Za-z0-9_-]*)\][ \t]*\r?\n"
    r"(?P<body>(?:[ ]*>.*(?:\r?\n|$))+)"
)

# `[标签]: 目标`
LINK_DEF_RE = re.compile(
    r"(?m)^(?P<head>\[(?P<label>[^\]\n]+)\]:[ \t]*)(?P<target>[^\s]+)[ \t]*\r?$"
)

# 规则 ID 的形状：点分小写标识符
RULE_SHAPE_RE = re.compile(r"^[a-z][a-z0-9_-]*(?:\.[a-z0-9_-]+)+$")

RENDER_NOTE = (
    '<svg viewBox="0 0 16 16" width="18" height="18" aria-hidden="true">'
    '<circle cx="8" cy="8" r="6.6" fill="none" stroke="currentColor" stroke-width="1.5"/>'
    '<path d="M8 7.2v4.2M8 4.6v1.2" stroke="currentColor" stroke-width="1.6" '
    'stroke-linecap="round"/></svg>'
)

RENDER_WARNING = (
    '<svg viewBox="0 0 16 16" width="18" height="18" aria-hidden="true">'
    '<path d="M8 1.8 15 14H1z" fill="none" stroke="currentColor" stroke-width="1.4" '
    'stroke-linejoin="round"/>'
    '<path d="M8 6.2v3.6M8 11.4v1.2" stroke="currentColor" stroke-width="1.5" '
    'stroke-linecap="round"/></svg>'
)

RENDER_EXAMPLE = (
    '<svg viewBox="0 0 16 16" width="18" height="18" aria-hidden="true">'
    '<circle cx="8" cy="8" r="6.6" fill="none" stroke="currentColor" stroke-width="1.5"/>'
    '<path d="M6.6 5.6 10.8 8l-4.2 2.4z" fill="currentColor"/></svg>'
)

RENDER_INCOMPLETE = (
    '<svg viewBox="0 0 16 16" width="18" height="18" aria-hidden="true">'
    '<rect x="2.2" y="2.2" width="11.6" height="11.6" rx="2" fill="none" '
    'stroke="currentColor" stroke-width="1.4" stroke-dasharray="2.6 2"/>'
    '<path d="M5.6 5.6 10.4 10.4" stroke="currentColor" stroke-width="1.4" '
    'stroke-linecap="round"/></svg>'
)

RENDER_STALE = (
    '<svg viewBox="0 0 16 16" width="18" height="18" aria-hidden="true">'
    '<circle cx="8" cy="8" r="6.6" fill="none" stroke="currentColor" stroke-width="1.5"/>'
    '<path d="M8 4.4V8l2.6 1.6" stroke="currentColor" stroke-width="1.5" '
    'stroke-linecap="round" stroke-linejoin="round"/></svg>'
)

# 写法 -> (CSS 类名, 标题, 图标)
ALERTS = {
    "note": ("note", "注", RENDER_NOTE),
    "warning": ("warning", "警告", RENDER_WARNING),
    "example": ("example", "示例", RENDER_EXAMPLE),
    "incomplete": ("incomplete", "该条目不完整", RENDER_INCOMPLETE),
    "stale": ("stale", "该条目可能过期", RENDER_STALE),
}

# 提示块标题按语言给一套，正文是哪种语言就显示哪种
TITLES_EN = {
    "note": "Note",
    "warning": "Warning",
    "example": "Example",
    "incomplete": "Incomplete entry",
    "stale": "May be outdated",
}

LANG_LABEL = {"zh": "中文", "en": "English"}


def warn(message):
    print(f"pxc-spec: {message}", file=sys.stderr)


def language_of(path):
    """`zh/storage.md` -> `zh`；不在语言目录下时返回空串。"""
    head, sep, _ = path.partition("/")
    return head if sep and head in LANGS else ""


def split_fences(text):
    """把正文切成 [(是否代码围栏, 片段)]，代码围栏内的内容不参与改写。"""
    segments = []
    pending = []
    fenced = []
    in_fence = False
    marker = ""

    for line in text.splitlines(keepends=True):
        stripped = line.lstrip()
        if not in_fence and (stripped.startswith("```") or stripped.startswith("~~~")):
            if pending:
                segments.append((False, "".join(pending)))
                pending = []
            in_fence = True
            marker = stripped[:3]
            fenced = [line]
            continue
        if in_fence:
            fenced.append(line)
            if stripped.startswith(marker):
                segments.append((True, "".join(fenced)))
                fenced = []
                in_fence = False
            continue
        pending.append(line)

    if fenced:
        segments.append((True, "".join(fenced)))
    if pending:
        segments.append((False, "".join(pending)))
    return segments


def non_code_text(content):
    """代码围栏以外的那部分正文；收集规则 ID 时用它，避免把示例里的
    `r[xxx]` 当成真规则登记（那会造出没有锚点的引用目标）。"""
    return "".join(chunk for is_code, chunk in split_fences(content) if not is_code)


def looks_like_rule_id(target):
    """判断链接目标是不是「规则 ID 的形状」，而不是路径或 URL。

    `.md` / `.html` 结尾、带斜杠冒号井号的都是普通链接，一律放过；
    其余形如 `storage.kinds` 的目标都是笔误，值得报出来。
    """
    if any(ch in target for ch in "/:#"):
        return False
    if target.endswith((".md", ".html")):
        return False
    return RULE_SHAPE_RE.match(target) is not None


def render_rule(match):
    """`r[storage.kinds]` -> 左边距里的规则标签。"""
    rule_id = match.group("id")
    # 每个点后面插入 <wbr>，窄栏里只在点处换行
    label = "<wbr>.".join(rule_id.split("."))
    return (
        f'<div class="rule" id="r-{rule_id}">'
        f'<a class="rule-link" href="#r-{rule_id}" title="{rule_id}">'
        f"<span>[{label}]</span></a></div>\n"
    )


def render_alert(match, lang):
    """`> [!KIND]` 引用块 -> `<div class="alert alert-kind">`。"""
    kind = match.group("kind").lower()
    if kind not in ALERTS:
        warn(f"未知的提示块类型 [!{match.group('kind')}]，已原样保留")
        return match.group(0)

    css_class, title, icon = ALERTS[kind]
    if lang == "en":
        title = TITLES_EN[kind]
    indent = match.group("ind")
    body = match.group("body")
    return (
        f'{indent}<div class="alert alert-{css_class}">\n'
        f"{indent}\n"
        f'{indent}> <p class="alert-title">{icon}{title}</p>\n'
        f"{indent}>\n"
        f"{body}"
        f"{indent}\n"
        f'{indent}</div>\n'
    )


def pick_owner(target, chapter_path, rule_owner):
    """同一个规则 ID 在多种语言里都有时，优先取本语言那一份。"""
    owners = rule_owner.get(target)
    if not owners:
        return None
    lang = language_of(chapter_path)
    same_lang = [path for path in owners if language_of(path) == lang]
    return (same_lang or owners)[0]


def render_link_def(match, chapter_path, rule_owner, missing):
    """`[标签]: storage.kinds` -> `[标签]: storage.html#r-storage.kinds`。"""
    target = match.group("target")
    owner = pick_owner(target, chapter_path, rule_owner)
    if owner is None:
        # 目标像规则 ID 却查不到，说明是笔误；记下来统一报一行。
        # 普通路径与 URL 留原样，由 mdBook 自己处理。
        if looks_like_rule_id(target):
            missing.setdefault(target, set()).add(chapter_path)
        return match.group(0)

    head = match.group("head")
    if owner == chapter_path:
        return f"{head}#r-{target}"
    here = posixpath.dirname(chapter_path) or "."
    # 这里保留 .md，mdBook 渲染时会把它改写成 .html
    return f"{head}{posixpath.relpath(owner, here)}#r-{target}"


def render_lang_switch(chapter_path, pages):
    """注入「中文 / English」切换链接；配对页不存在时只显示当前语言。"""
    lang = language_of(chapter_path)
    if not lang:
        return ""

    rest = chapter_path[len(lang) + 1 :]
    links = []
    for other in LANGS:
        target = pages.get((other, rest))
        if other == lang:
            links.append(f"<span>{LANG_LABEL[other]}</span>")
        elif target is not None:
            href = posixpath.relpath(target, posixpath.dirname(chapter_path) or ".")
            links.append(f'<a href="{href[:-3]}.html">{LANG_LABEL[other]}</a>')
    if len(links) < 2:
        return ""
    return f'<div class="lang-switch">{"".join(links)}</div>\n\n'


def transform(content, chapter_path, rule_owner, pages, missing):
    lang = language_of(chapter_path)
    out = [render_lang_switch(chapter_path, pages)]
    for is_code, chunk in split_fences(content):
        if is_code:
            out.append(chunk)
            continue
        chunk = RULE_RE.sub(render_rule, chunk)
        chunk = ADM_RE.sub(lambda m: render_alert(m, lang), chunk)
        chunk = LINK_DEF_RE.sub(
            lambda m: render_link_def(m, chapter_path, rule_owner, missing), chunk
        )
        out.append(chunk)
    return "".join(out)


def walk(items):
    """按顺序遍历 book 里所有章节（含子章节）。"""
    for item in items:
        chapter = item.get("Chapter")
        if chapter is None:
            continue
        yield chapter
        yield from walk(chapter.get("sub_items") or [])


def report_missing_translations(ids_by_lang):
    """某种语言缺了另一种语言有的规则 ID 时，报一行汇总。"""
    for lang in LANGS:
        if lang not in ids_by_lang:
            continue
        others = set()
        for other, ids in ids_by_lang.items():
            if other != lang:
                others |= set(ids)
        missing = sorted(others - set(ids_by_lang[lang]))
        if missing:
            sample = "、".join(missing[:6]) + ("…" if len(missing) > 6 else "")
            warn(f"{lang} 缺少 {len(missing)} 条别处已有的规则 ID：{sample}")


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "supports":
        renderer = sys.argv[2] if len(sys.argv) > 2 else ""
        return 0 if renderer == "html" else 1

    # mdBook 送进来的是 UTF-8 字节流；不显式指定编码时 Python 按 locale 解码，
    # 在非 UTF-8 区域（例如 cp936 的 Windows）上含中文的 payload 会直接解码失败。
    for stream in (sys.stdin, sys.stdout):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8")

    # mdBook 0.5 送进来的是一对 (ctx, book)；0.4 是 {"book": ...}。
    # 两者都收，输出统一用 0.5 的 Book 形状。
    payload = json.load(sys.stdin)
    book = payload[1] if isinstance(payload, list) else payload["book"]
    items = book.get("items") if "items" in book else book["sections"]

    chapters = [c for c in walk(items) if c.get("path")]

    # 第一遍：收集规则 ID（按语言分组）与「语言 + 去语言相对路径」的页面对照表
    rule_owner = {}
    ids_by_lang = {}
    pages = {}
    for chapter in chapters:
        path = chapter["path"]
        lang = language_of(path)
        if lang:
            pages[(lang, path[len(lang) + 1 :])] = path
        # 只看围栏外的正文：示例里写的 `r[xxx]` 不是真规则，登记了会造出死锚点
        for match in RULE_RE.finditer(non_code_text(chapter.get("content") or "")):
            rule_id = match.group("id")
            owners = rule_owner.setdefault(rule_id, [])
            if any(language_of(p) == lang for p in owners):
                warn(f"规则 ID 重复：[{rule_id}] 在 {path} 与 {owners[0]}")
            owners.append(path)
            if lang:
                ids_by_lang.setdefault(lang, set()).add(rule_id)

    # 第二遍：改写正文
    missing = {}
    for chapter in chapters:
        chapter["content"] = transform(
            chapter.get("content") or "", chapter["path"], rule_owner, pages, missing
        )

    if missing:
        sample = "、".join(sorted(missing)[:6]) + ("…" if len(missing) > 6 else "")
        warn(f"引用了不存在的规则 ID（{len(missing)} 个，已原样保留）：{sample}")

    report_missing_translations(ids_by_lang)
    json.dump({"items": items}, sys.stdout)
    return 0


if __name__ == "__main__":
    sys.exit(main())
