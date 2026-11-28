# Agent instructions

This file is short on purpose. If a rule here and a script disagree, the script wins.

## Prepare

Read [Readme](README.md) and [CONTRIBUTING](CONTRIBUTING.md) first.

## Build, test, format

- The canonical commands live in `justfile`. Run them; don't re-derive them and don't call the underlying tool directly.
- Run the focused test for what you changed before you report done.
- Don't add or bump a dependency, or move the toolchain pin, without asking.

## Text you must not write

Never produce text that will be published under a contributor's name:

- PR titles and bodies, issue bodies, review replies, public chat messages
- `Docs/`, `CHANGELOG.md`, release notes, and so on.

This is not about quality. A description written by a model tells a maintainer nothing about whether a person read the change, and that evidence cannot be generated afterwards by anyone.

If asked for one of these: say what it needs to cover, in your reply, and stop there. Don't offer a draft, a template, or a "you can edit this" version. If the rule blocks the task, name the rule — don't rename the artifact to get around it.

You may read, summarize, and review any of the above, and you may say what a description should cover. You may write code comments.

## Ask before

- changing semantics: the evaluator, the type checker, or when a diagnostic fires
- changing an existing test's expectations, or deleting one
- touching anything under `vendor/`, `submodules/`, `src/tools/`
