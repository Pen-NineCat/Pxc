<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright(c) 2026-Present Pen-NineCat(PenNineCat) -->
<!-- https://github.com/Pen-NineCat -->

# The Pxc Reference Site

*[中文版](README.md)*

This directory is `Docs/book`. It is rendered with [mdBook](https://rust-lang.github.io/mdBook/) and
styled after the [Rust Reference](https://doc.rust-lang.org/reference/): a collapsible sidebar
(numbering is switched off here), rule identifiers in the left margin of the text column, and
admonitions drawn as type-coloured rounded boxes.

## Building

Requires **mdBook 0.5 or newer** (0.5 changed the preprocessor protocol) and Python 3:

```shell
# Live preview, rebuilds on save: http://localhost:3000
mdbook serve

# Build into book/ only (that directory is not committed)
mdbook build
```

Install mdBook once:

```shell
cargo install mdbook --locked
```

## Layout

```
Docs/book/
├─ book.toml          # mdBook config: CSS, preprocessor, fold depth
├─ src/
│  ├─ SUMMARY.md      # table of contents: two top-level trees, 中文 / English
│  ├─ zh/             # Chinese (source edition)
│  └─ en/             # English (translation)
├─ theme/pxc.css      # site theme, the only stylesheet in the book
└─ tools/pxc_spec.py  # preprocessor: rule IDs / admonitions / rule refs / language switch
```

`book/` holds the build output and is excluded in `.gitignore`.

## The three notations

### 1. Rule identifiers

A line of its own attaches a citable anchor to **the block that follows** (a heading or a paragraph):

```markdown
r[storage.kinds]
## Three storage durations
```

It renders as a grey label `[storage.kinds]` in the left margin, with the anchor `#r-storage.kinds`.
Identifiers are dotted, start with lowercase English, and **are never changed once something cites
them**. The same rule must use the same identifier in both editions.

An `r[xxx]` inside a fenced code block is not treated as a rule, so demonstrating the notation inside
a fence is safe.

### 2. Admonitions

```markdown
> [!WARNING]
> There are only two roads across……
```

Five kinds are supported: `NOTE`, `WARNING`, `EXAMPLE`, `INCOMPLETE` and `STALE`. The heading follows
the language of the page (a Chinese page shows 「警告」, an English page shows "Warning"). A
`> [!NOTE]` inside a fenced code block is left alone, so it can be used to demonstrate the syntax.

### 3. Referring to another rule

Write only the phrase in the prose, and put the rule identifier in the link definition:

```markdown
See [three storage durations].

[three storage durations]: storage.kinds
```

The preprocessor expands it into `storage.html#r-storage.kinds`. When the same identifier exists in
both editions, it points at the edition being read. If the target looks like a rule identifier but
does not exist (usually a typo), the build reports it in one summary line and leaves the definition
untouched, so a broken link that 404s on click is never produced silently.

## Not implemented: grammar blocks and railroad diagrams

The Rust Reference renders a `grammar` fence as two things: a **grammar block** (literals in rounded
chips, non-terminals coloured) and **railroad diagrams** generated from the same BNF, toggled by the
"Show syntax diagrams" button. Neither is implemented here — Pxc has no formal grammar artefact yet,
so an ordinary code fence is enough:

````markdown
```ebnf
Expression -> ExpressionWithoutBlock | ExpressionWithBlock
```
````

Adding that appearance later means styling in `theme/pxc.css` plus a fence branch in
`tools/pxc_spec.py`; the prose would not change.

## Bilingual conventions

- Chinese is the source edition and English is the translation; chapter order, rule identifiers and
  admonition kinds must correspond one to one.
- Pages are paired by their path with the language directory stripped (`zh/storage.md` ↔
  `en/storage.md`); a successful pair shows the 「中文 / English」 switch in the top-right corner of
  the page, so a new chapter needs a sibling file in both trees.
- If one edition lacks a rule identifier that the other has, the preprocessor reports it in one
  summary line on stderr.

## License

- Site code and configuration (`book.toml`, `theme/`, `tools/`, this file): **Apache-2.0**, see
  `LICENSE.txt` at the repository root
- Specification text (Markdown under `src/`): **[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)**
- Third-party works and attribution: [THIRD-PARTY-NOTICES.md (English)](THIRD-PARTY-NOTICES.en.md)
  next to these sources, its [Chinese counterpart](THIRD-PARTY-NOTICES.md), or the copy at the
  repository root ([THIRD-PARTY-NOTICES.md](../../THIRD-PARTY-NOTICES.md)); the three must stay
  identical
