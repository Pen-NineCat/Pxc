<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright(c) 2026-Present Pen-NineCat(PenNineCat) -->
<!-- https://github.com/Pen-NineCat -->

# Review conventions

## File level

- Every repository file starts with the SPDX header on its first line, in the comment syntax of the
  language: `// SPDX-License-Identifier: Apache-2.0`.
- ※ New source files are registered in the matching `CMakeLists.txt`; otherwise they never compile.
- ※ Public headers document ownership and lifetime of the raw pointers and references they expose.
- Temporary files, probes and one-off scripts do not belong in `Code/`. Repository level scripts
  live under `Tools/`.

## Behaviour

- ※ New behaviour comes with a unit test (GTest, `Code/Tests/Unit/...`). Missing coverage for a new
  branch, a new diagnostic code or a new parser path is a finding.
- ※ Diagnostic codes are added to `DiagnosticCode.yaml`; the generated table is never edited by hand.
- Error paths never swallow failures silently, and messages state what the caller can do next.
- No unrelated reformatting or renaming mixed into a functional change; it hides the real diff.
