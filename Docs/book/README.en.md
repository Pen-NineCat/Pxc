<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright(c) 2026-Present Pen-NineCat(PenNineCat) -->
<!-- https://github.com/Pen-NineCat -->

# The Pxc Reference Site

*[中文版](README.md)*

This directory is `Docs/book`. It is rendered with [mdBook](https://rust-lang.github.io/mdBook/) and
styled after the [Rust Reference](https://doc.rust-lang.org/reference/): a collapsible sidebar
(numbering is switched off here), rule identifiers in the left margin of the text column,
admonitions drawn as type-coloured rounded boxes, and tables drawn as three-line tables.

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
├─ theme/
│  ├─ pxc.css         # the only stylesheet in the book
│  ├─ rule-fit.js     # swaps rule labels that do not fit for [*]
│  └─ cjk-search.js   # takes over the search index so Chinese is searchable
└─ tools/pxc_spec.py  # preprocessor: rule IDs / admonitions / rule refs / language switch
```

`book/` holds the build output and is excluded in `.gitignore`.

`src/` is empty for now — the prose has not been written yet; the author splits the chapters out of the
specification documents. Without `src/SUMMARY.md`, `mdbook build` fails outright (`failed to read
src/SUMMARY.md`), so use the sandbox copy in `PersonalWorkaround/RefStyleResearch/demo` to look at the
styling in the meantime.

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

Six kinds are supported: `NOTE`, `WARNING`, `EXAMPLE`, `INCOMPLETE` (incomplete entry), `EVOLVING`
(entry still evolving) and `DEPRECATED-<version>` (deprecated API; the version goes in the marker, see
§2.2). The heading follows the language of the page (a Chinese page shows 「警告」, an English page
shows "Warning"). A `> [!NOTE]` inside a fenced code block is left alone, so it can be used to
demonstrate the syntax.

### 2.1 Version difference blocks

```markdown
> [!VERSION-1.0]
> Before Version:
> How it worked before 1.0; several paragraphs are fine.
>
> After Version:
> How it works from 1.0 on.
```

Renders as one outer box whose title shows the version `1.0` inside a bold rounded outline, holding two
separately bordered parts titled "Before Version:" and "After Version:". The parts reuse the same border
recipe as the code outline inside a block. The markers may be written in English
(`Before Version:` / `After Version:`) or in Chinese; if one side is missing the build warns and falls
back to a plain admonition.

Use it only for major-version updates whose changes break existing code; the difference between major
and minor versions belongs in the prose, not in another block kind.

### 2.2 Deprecated APIs

```markdown
> [!DEPRECATED-1.2]
> `ValueMoveConvert` is deprecated; use `MoveConstruct` instead.
```

The version goes in the marker and is carried into the heading: a Chinese page shows
「弃用的 API（自 `1.2` 起）」and an English page shows “Deprecated API (since `1.2`)”. The version sits in
the same bold rounded outline as in a version difference block — both express the same thing: which
release this content belongs to. A bare `[!DEPRECATED]` makes the build warn and renders the heading
without a version.

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

## How tables and code look

Neither needs a notation of its own: the appearance lives in `theme/pxc.css`, and the prose is plain
Markdown.

A **table** renders as a three-line table: a grey rounded frame around it and one thin rule under the
header row. The frame's top and bottom edges are the table's top and bottom rules, so the whole table
has exactly three horizontal lines. No fill, no vertical lines, no zebra striping; headers are
left-aligned like the body (a `| ---: |` column still right-aligns).

| Storage duration | Where the object lives | Who creates it |
| --- | --- | --- |
| Automatic | At the declaration (on the stack, or inside the host object) | The compiler |
| Managed | The collector heap | The collector |

**Code**, inline and fenced, uses the **body font at body size** — not a monospace face, and not a
smaller size. A fenced block is separated from the prose by its light fill, while blocks and inline
code inside admonitions and tables get an outline instead of a fill (a translucent fill over a
coloured box looks dirty). The only cost: trailing comments aligned with spaces in the source no
longer line up in proportional text, so examples should stop aligning them in columns.

## Search

mdBook builds the search index at **build time**, and its tokenizer only knows whitespace and
punctuation: a whole run of Chinese becomes one token and is then dropped by the trimmer. Barely any
token in the index contains Chinese (when this was written: 30 sections, 695 tokens, of which 2
contain Chinese — the same leftover embedded in English text), so 「存储期」 finds nothing in
「元数据存储期」.

`theme/cjk-search.js` patches this in the browser. The searcher touches the index through one call,
`elasticlunr.Index.load(index).search(term, options)`, so the script swaps `Index.load` for its own
implementation and works from the raw text kept in the index (`documentStore.docs`: title / body /
breadcrumbs). **No mdBook change, no build artefact change, no build-time dependency.**

- Chinese is split into **adjacent two-character terms** (a one-character query stays one character);
  English is split into whole words. Every term has to appear in the same section for it to match
  (matching `[output.html.search] use-boolean-and = true`).
- Scores are weighted per field — title 4, breadcrumbs and body 1 — and results in the language of
  the current page are doubled. The two editions are paired, so without that doubling a Chinese page
  would show the English copy first for a query like `ReflectState`.
- The teaser is the 24 / 80 characters around the hit: the searcher's teaser counts **spaces**, and a
  Chinese section is a single "word", so an untrimmed body would print in full on every result line.

Known limits:

- No match highlighting inside the teaser (the body is escaped by the searcher before it is rendered,
  so an `<em>` cannot be injected); the `?highlight=` marking on the page itself is unaffected.
- No typo tolerance: it matches literally, unlike fzf's out-of-order matching.
- A `?search=` deep link has a millisecond-scale race on first load: that one run may still use
  mdBook's own index; a reload fixes it.
- To drop the behaviour, remove `cjk-search.js` from `additional-js`.

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
