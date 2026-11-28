# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
# https://github.com/Pen-NineCat

"""Review one pull request with an OpenAI-compatible model and post the result as a GitHub review.

The review is the first approval layer: the process exits non-zero when a finding is `high` or
`medium`, so the workflow job can be a required status check. Everything that could hide a problem
fails closed: a model error, an answer that does not match the schema, and a review that could not
be posted all exit non-zero.

The model may read the checkout through three read-only tools before it answers. The diff and the
changed files are already in the prompt, so a tool call is only for what they cannot show — a
header and its contract, a caller, a build file, a test. `PXC_REVIEW_TOOLS=off` falls back to one
request without them.

The script is executed from the *base* commit of the pull request, see `.github/workflows/
agent-review.yml`, so a pull request cannot ship a modified reviewer.

Environment:
    GITHUB_TOKEN          token used to post the review, needs `pull-requests: write`
    GITHUB_REPOSITORY     owner/repo, set by Actions
    GITHUB_EVENT_PATH     event payload, set by Actions
    PXC_REVIEW_API_KEY    API key of the OpenAI-compatible endpoint (repository secret)
    PXC_REVIEW_API_BASE   endpoint base, e.g. https://api.deepseek.com/v1
    PXC_REVIEW_MODEL      model id, e.g. deepseek-chat
    PXC_REVIEW_LANGUAGE   language of the generated text, English by default
    PXC_CI_STATE          overrides the state derived from the `ci:*` labels, for a local run
    PXC_REVIEW_TOOLS      `off` asks in one request without repository tools, default on
    PR_NUMBER, BASE_SHA, HEAD_SHA   optional overrides, taken from the event when absent

Usage:
    python Tools/agent_review.py              # review, post, gate the job
    python Tools/agent_review.py --self-test  # run the parsing tests, no network
    python Tools/agent_review.py --dry-run    # build the prompt only, no API call, no review
    python Tools/agent_review.py --diff-file pr.diff --dry-run
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

DEFAULT_API_BASE = "https://api.deepseek.com/v1"
DEFAULT_MODEL = "deepseek-chat"
DEFAULT_LANGUAGE = "English"

# CI is opt-in: a maintainer adds a `ci:*` label (`ci:cpp` today) to start a matrix, so a review
# normally runs before any build exists. A model that assumes otherwise fills the gap with invented
# confidence ("this compiles", "the tests pass"), which is exactly what a review must not do. The
# state is read from the labels, so a new `ci:<matrix>` label needs no change here.
CI_CONTEXT = {
	"not-run": (
		"No CI has run for this pull request. CI is started by a maintainer adding a `ci:*` label (the\n"
		"C++ matrix uses `ci:cpp`), and no such label is on this pull request. You see only the diff:\n"
		"do not state that the code compiles, that tests pass or that formatting is clean, and when a\n"
		"finding depends on one of them, say the claim is unverified instead of assuming it holds."
	),
	"requested": (
		"CI was requested for this pull request with {labels}. Its state is unknown here: it may be\n"
		"running, may have failed, or may not cover this commit. Do not treat it as a pass, and do\n"
		"not restate its verdict as a finding."
	),
}

# Review comments are capped: a wall of inline notes is worse than no review at all.
MAX_INLINE_COMMENTS = 10
MAX_FINDINGS = 15

# Prompt budget, roughly four characters per token.
MAX_PROMPT_CHARS = 200_000
MAX_DIFF_CHARS = 120_000
MAX_FILE_TEXT_CHARS = 30_000
MAX_FILE_TEXT_TOTAL = 90_000
MAX_FILE_TEXT_BYTES = 200_000

# Tool loop. The diff and the changed files answer most questions, but not "what does this header
# promise" or "who else calls this", and the file that would answer it is not always C++. The model
# may read the checkout instead, inside a budget that keeps one review predictable.
MAX_TOOL_ROUNDS = 6
MAX_TOOL_CALLS = 4
MAX_TOOL_CONTEXT_CHARS = 60_000
MAX_TOOL_READ_LINES = 400
MAX_TOOL_MATCHES = 80
MAX_TOOL_ENTRIES = 200

TOOLS = [
	{
		"type": "function",
		"function": {
			"name": "read_file",
			"description": "Read one text file of this checkout, by line range.",
			"parameters": {
				"type": "object",
				"properties": {
					"path": {"type": "string", "description": "repository-relative path"},
					"start": {"type": "integer", "description": "1-based first line, default 1"},
					"end": {"type": "integer", "description": f"last line, default start+{MAX_TOOL_READ_LINES - 1}"},
				},
				"required": ["path"],
			},
		},
	},
	{
		"type": "function",
		"function": {
			"name": "grep",
			"description": "Search the checkout with an extended regular expression; answers path:line:text.",
			"parameters": {
				"type": "object",
				"properties": {
					"pattern": {"type": "string", "description": "extended regular expression"},
					"glob": {"type": "string", "description": "restrict to paths matching a git pathspec, e.g. *.hpp"},
				},
				"required": ["pattern"],
			},
		},
	},
	{
		"type": "function",
		"function": {
			"name": "list_dir",
			"description": "List the entries of one directory of this checkout.",
			"parameters": {
				"type": "object",
				"properties": {"path": {"type": "string", "description": "repository-relative directory"}},
				"required": ["path"],
			},
		},
	},
]

BLOCKING_SEVERITIES = ("high", "medium")
SEVERITY_ORDER = {"high": 0, "medium": 1, "low": 2}
UNKNOWN_SEVERITY = "medium"

RETRY_STATUSES = (429, 500, 502, 503, 504)
MAX_ATTEMPTS = 4

HUNK_RE = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,\d+)? @@")
FENCE_RE = re.compile(r"^```[A-Za-z0-9_+-]*\s*$", re.MULTILINE)
# The leading HTML comments of the conventions file: the same SPDX header every repository file
# carries. They are metadata for the repository, not guidance for the reviewer.
HEADER_COMMENT_RE = re.compile(r"\A(?:\s*<!--.*?-->\s*)+", re.DOTALL)

CONVENTIONS_PATH = Path(__file__).resolve().parent / "review_conventions.md"

GIT_ESCAPES = {
	"a": b"\a",
	"b": b"\b",
	"f": b"\f",
	"n": b"\n",
	"r": b"\r",
	"t": b"\t",
	"v": b"\v",
	"\\": b"\\",
	'"': b'"',
}


class ResponseFormatUnsupported(RuntimeError):
	"""The endpoint does not accept `response_format`; retry without it."""


class ToolsUnsupported(RuntimeError):
	"""The endpoint does not accept `tools`; fall back to one request without them."""


def ReadEvent() -> dict:
	eventPath = os.environ.get("GITHUB_EVENT_PATH", "")
	if not eventPath:
		raise RuntimeError("GITHUB_EVENT_PATH is not set, this script is meant to run in Actions")
	return json.loads(Path(eventPath).read_text(encoding="utf-8"))


def RunGit(*args: str) -> str:
	"""Run git and return stdout. A failing git command is a hard error: the diff is the input."""
	result = subprocess.run(
		["git", "-c", "core.quotePath=false", *args],
		check=True,
		capture_output=True,
		text=True,
		encoding="utf-8",
		errors="replace",
	)
	return result.stdout


def UnquoteGitPath(value: str) -> str:
	"""Decode the C-style quoting git uses for paths it considers unusual.

	`core.quotePath=false` keeps non-ASCII names literal, but git still quotes paths containing
	control characters or a leading quote, and the octal escapes have to be turned back into bytes
	before the path can be matched against the file system.
	"""
	if len(value) < 2 or not value.startswith('"') or not value.endswith('"'):
		return value

	body = value[1:-1]
	out = bytearray()
	index = 0
	while index < len(body):
		char = body[index]
		if char != "\\":
			out.extend(char.encode("utf-8"))
			index += 1
			continue
		index += 1
		if index >= len(body):
			break
		escape = body[index]
		if escape in GIT_ESCAPES:
			out.extend(GIT_ESCAPES[escape])
			index += 1
		elif escape.isdigit() and body[index : index + 3].isdigit():
			out.append(int(body[index : index + 3], 8))
			index += 3
		else:
			out.extend(escape.encode("utf-8"))
			index += 1
	return out.decode("utf-8", "replace")


def ParseDiff(diffText: str) -> tuple[list[str], dict[str, set[int]], dict[str, str]]:
	"""Split a unified diff into changed paths, commentable new-file lines and their hunks.

	The review API only accepts a `line` that is an added or context line of some hunk, so the set
	built here is the whitelist every model answer is checked against. Header detection is a state
	machine rather than a prefix test: an added line whose content is `++ x;` shows up as `+++ x;`
	and must not be mistaken for a `+++ b/path` header.
	"""
	order: list[str] = []
	commentable: dict[str, set[int]] = {}
	chunks: dict[str, str] = {}
	path = ""
	newLine = 0
	state = "idle"
	buffer: list[str] = []

	for line in diffText.splitlines():
		if line.startswith("diff --git "):
			if path:
				chunks[path] = "\n".join(buffer)
			buffer = [line]
			path = ""
			state = "header"
			continue

		if state == "idle":
			continue

		if state == "header":
			buffer.append(line)
			if line.startswith("--- "):
				continue
			if line.startswith("+++ "):
				target = UnquoteGitPath(line[4:].strip())
				if target != "/dev/null":
					path = target[2:] if target.startswith("b/") else target
					if path not in commentable:
						order.append(path)
						commentable[path] = set()
				state = "body"
				continue
			if line.startswith("Binary files ") or line.startswith("GIT binary patch"):
				state = "body"
			continue

		buffer.append(line)

		if line.startswith("@@ "):
			match = HUNK_RE.match(line)
			if match is None:
				continue
			newLine = int(match.group(1))
			continue

		if not path:
			continue

		if line.startswith("+"):
			commentable[path].add(newLine)
			newLine += 1
		elif line.startswith(" "):
			commentable[path].add(newLine)
			newLine += 1

	if path:
		chunks[path] = "\n".join(buffer)

	return order, commentable, chunks


def StripHeaderComments(text: str) -> str:
	"""Drop the HTML comments the file opens with so they never reach the prompt.

	Only the comments before the first non-comment line are removed: a comment further down is a
	note a reader needs, and slicing the top of the file is the whole job here.
	"""
	return HEADER_COMMENT_RE.sub("", text, count=1)


def ReadConventions() -> str:
	if not CONVENTIONS_PATH.exists():
		return ""
	return StripHeaderComments(CONVENTIONS_PATH.read_text(encoding="utf-8"))


def IsBinary(path: str) -> bool:
	"""Git only prints `Binary files ... differ` when the file has no `diff` representation.

	Some files (small ones, or ones that happen to be NUL-free) still arrive as a text hunk, so the
	content is inspected before it is pasted into the prompt.
	"""
	try:
		with open(path, "rb") as handle:
			sample = handle.read(8192)
	except OSError:
		return True
	return b"\0" in sample


def ReadFileText(path: str, budget: list[int]) -> str:
	"""Read one changed file for context, inside a shared character budget."""
	if budget[0] <= 0 or IsBinary(path):
		return ""
	try:
		size = os.path.getsize(path)
		if size > MAX_FILE_TEXT_BYTES:
			return ""
		text = Path(path).read_text(encoding="utf-8", errors="replace")
	except OSError:
		return ""
	if len(text) > MAX_FILE_TEXT_CHARS:
		text = text[:MAX_FILE_TEXT_CHARS] + "\n... (truncated)\n"
	budget[0] -= len(text)
	return text


def CiLabels(pr: dict) -> list[str]:
	"""The labels that request CI, in the order the pull request lists them.

	The prefix is what matters, not `ci:cpp`: a new `ci:<matrix>` label reaches the model without a
	change here, and the comment says which one was requested.
	"""
	return [
		str(label.get("name") or "")
		for label in (pr.get("labels") or [])
		if isinstance(label, dict) and str(label.get("name") or "").startswith("ci:")
	]


def CiContext(pr: dict) -> str:
	"""What the checks do and do not prove.

	The state comes from the pull request's labels, so the same prompt is built from a
	`pull_request` payload and from the API object a `workflow_run` has to fetch. `PXC_CI_STATE`
	overrides it for a local dry run. An unknown state reads as "not run", the safe side.
	"""
	labels = CiLabels(pr)
	state = os.environ.get("PXC_CI_STATE") or ("requested" if labels else "not-run")
	named = ", ".join(f"`{label}`" for label in labels) or "a `ci:*` label"
	return CI_CONTEXT.get(state, CI_CONTEXT["not-run"]).format(labels=named)


def BuildPrompt(pr: dict, files: list[str], chunks: dict[str, str], conventions: str, language: str) -> tuple[str, str]:
	title = str(pr.get("title") or "").strip()
	body = str(pr.get("body") or "").strip()
	if len(body) > 4000:
		body = body[:4000] + "\n... (truncated)"

	system = "\n".join(
		[
			"You are an AI reviewer for the Pxc repository. You read a pull request diff and",
			"return findings as one JSON object.",
			"",
			"Repository conventions:",
			conventions.strip() or "(none provided, rely on the rules below)",
			"",
			CiContext(pr),
			"",
			"Answer with ONE JSON object and nothing else, shaped exactly like:",
			'{"summary": "one paragraph", "findings": [{"path": "a/b.cpp", "line": 12,',
			' "severity": "high", "title": "short title", "detail": "what is wrong and why",',
			' "suggestion": "replacement code or null"}]}',
			"",
			"Rules:",
			'- "findings" must always be present and must be a list, possibly empty.',
			'- "path" has to be one of the changed files, "line" is a new-file line number that appears',
			"  in the diff, or 0 when the finding is about the whole file.",
			'- "severity" is one of "high", "medium", "low":',
			'  "high" for correctness, undefined behaviour, crash, security, data loss or a broken build;',
			'  "medium" for a convention violation, a missing test for new behaviour, a misleading error',
			'  or a leaked resource; "low" for readability and naming nits.',
			"- Do not report formatting that clang-format enforces, do not restate the diff, do not",
			"  praise, do not invent files that are not in the diff.",
			'- "suggestion" contains only replacement code for the commented lines, or null.',
			f"- At most {MAX_FINDINGS} findings, most severe first.",
			f"- Write the summary and every detail in {language}. Keep identifiers and paths verbatim.",
			"",
			"You may call `read_file`, `grep` and `list_dir` before you answer. The diff and the full text",
			"of every changed file are already above, so call a tool only for what they cannot show: a",
			"header or its contract, a caller, a build file, a test. A tool call is not an answer — when",
			"you have what you need, answer with the one JSON object.",
		]
	)

	parts = [f"# Pull request: {title}", "", body, "", "# Changed files", ""]
	total = 0
	budget = [MAX_FILE_TEXT_TOTAL]

	for path in files:
		chunk = chunks.get(path, "")
		if total + len(chunk) > MAX_DIFF_CHARS:
			parts.extend([f"## {path}", "(diff omitted, prompt budget reached)", ""])
			continue
		total += len(chunk)
		parts.extend([f"## {path}", "```diff", chunk, "```", ""])

	parts.extend(["# Full content of the changed files (context only)", ""])
	for path in files:
		text = ReadFileText(path, budget)
		if not text:
			continue
		parts.extend([f"## {path}", "```", text, "```", ""])

	user = "\n".join(parts)
	if len(user) > MAX_PROMPT_CHARS:
		user = user[:MAX_PROMPT_CHARS] + "\n... (truncated)\n"
	return system, user


def RetryDelay(error: urllib.error.HTTPError, attempt: int) -> float:
	header = error.headers.get("Retry-After") if error.headers else None
	if header and header.strip().isdigit():
		return min(float(header.strip()), 60.0)
	return float(2**attempt)


def SendChat(base: str, key: str, payload: dict) -> tuple[dict, dict]:
	body = json.dumps(payload).encode("utf-8")
	for attempt in range(1, MAX_ATTEMPTS + 1):
		request = urllib.request.Request(
			f"{base}/chat/completions",
			data=body,
			method="POST",
			headers={"Content-Type": "application/json", "Authorization": f"Bearer {key}"},
		)
		try:
			with urllib.request.urlopen(request, timeout=300) as response:
				data = json.loads(response.read().decode("utf-8"))
			try:
				message = data["choices"][0]["message"]
			except (KeyError, IndexError, TypeError) as error:
				raise RuntimeError(f"unexpected chat completion payload: {error}") from error
			return message, data.get("usage", {})
		except urllib.error.HTTPError as error:
			detail = error.read()[:400].decode("utf-8", "replace")
			if error.code in RETRY_STATUSES and attempt < MAX_ATTEMPTS:
				delay = RetryDelay(error, attempt)
				print(f"::warning::chat completion returned {error.code}, retrying in {delay:.0f}s")
				time.sleep(delay)
				continue
			if error.code in (400, 422) and "tools" in payload and "tool" in detail.lower():
				raise ToolsUnsupported(detail) from error
			if error.code in (400, 422) and "response_format" in payload:
				raise ResponseFormatUnsupported(detail) from error
			raise RuntimeError(f"chat completion returned {error.code}: {detail}") from error
		except urllib.error.URLError as error:
			if attempt < MAX_ATTEMPTS:
				delay = float(2**attempt)
				print(f"::warning::chat completion failed ({error.reason}), retrying in {delay:.0f}s")
				time.sleep(delay)
				continue
			raise RuntimeError(f"chat completion failed: {error.reason}") from error
	raise RuntimeError("chat completion failed after retries")


def CallChat(messages: list[dict], tools: list[dict] | None) -> tuple[dict, dict]:
	"""One chat completion. Without tools the answer is asked for as a JSON object."""
	key = os.environ.get("PXC_REVIEW_API_KEY", "")
	if not key:
		raise RuntimeError("PXC_REVIEW_API_KEY is not set")
	base = (os.environ.get("PXC_REVIEW_API_BASE") or DEFAULT_API_BASE).rstrip("/")
	model = os.environ.get("PXC_REVIEW_MODEL") or DEFAULT_MODEL

	payload = {
		"model": model,
		"messages": messages,
		"temperature": 0,
		"max_tokens": 8192,
	}
	if tools:
		payload["tools"] = tools
		payload["tool_choice"] = "auto"
	else:
		payload["response_format"] = {"type": "json_object"}
	try:
		return SendChat(base, key, payload)
	except ResponseFormatUnsupported:
		print("::warning::the endpoint rejected response_format, retrying without it")
		payload.pop("response_format")
		return SendChat(base, key, payload)


def CallModel(system: str, user: str) -> tuple[str, dict]:
	message, usage = CallChat(
		[{"role": "system", "content": system}, {"role": "user", "content": user}],
		None,
	)
	return str(message.get("content") or ""), usage


def ResolvePath(path: str) -> Path:
	"""Resolve a tool path inside the checkout; a path that leaves it is refused.

	The checkout is the pull request head, so a path here is untrusted input: the review must not
	be able to read outside the repository or into `.git`.
	"""
	root = Path.cwd().resolve()
	candidate = Path(path)
	if not candidate.is_absolute():
		candidate = root / candidate
	candidate = candidate.resolve()
	if candidate != root and root not in candidate.parents:
		raise ValueError(f"{path!r} is outside the repository")
	if candidate != root and ".git" in candidate.relative_to(root).parts:
		raise ValueError(f"{path!r} is not reviewable")
	return candidate


def RequireInt(value: object, default: int) -> int:
	try:
		return int(value)  # a model argument is untrusted; a bad integer falls back to the default
	except (TypeError, ValueError):
		return default


def ToolReadFile(path: str, arguments: dict) -> str:
	target = ResolvePath(path)
	if not target.is_file():
		raise ValueError(f"{path!r} is not a file")
	if target.stat().st_size > MAX_FILE_TEXT_BYTES:
		raise ValueError(f"{path!r} is larger than {MAX_FILE_TEXT_BYTES} bytes")
	lines = target.read_text(encoding="utf-8", errors="replace").splitlines()
	start = max(1, RequireInt(arguments.get("start"), 1))
	if start > len(lines):
		raise ValueError(f"{path!r} has only {len(lines)} lines")
	# `end` is clamped to the window: one call never returns more than MAX_TOOL_READ_LINES lines.
	end = min(RequireInt(arguments.get("end"), start + MAX_TOOL_READ_LINES - 1), start + MAX_TOOL_READ_LINES - 1, len(lines))
	if end < start:
		raise ValueError("`end` is before `start`")
	body = "\n".join(lines[start - 1 : end])
	if len(body) > MAX_FILE_TEXT_CHARS:
		body = body[:MAX_FILE_TEXT_CHARS] + "\n... (truncated)\n"
	relative = target.relative_to(Path.cwd().resolve()).as_posix()
	return f"{relative}:{start}-{end}\n{body}"


def ToolGrep(pattern: str, arguments: dict) -> str:
	if not pattern:
		raise ValueError("grep needs a `pattern`")
	glob = str(arguments.get("glob") or "")
	# `--untracked` keeps a local run and the self-test honest: a file added but not committed is
	# part of the change being reviewed, and a bare `git grep` would not see it.
	completed = subprocess.run(
		["git", "-c", "core.quotePath=false", "grep", "-n", "-I", "-E", "--no-color", "--untracked", "-e", pattern, "--", glob or "."],
		capture_output=True,
		text=True,
		encoding="utf-8",
		errors="replace",
	)
	# 1 is "no match"; anything else is a broken pattern or a missing tree and must be reported.
	if completed.returncode not in (0, 1):
		raise RuntimeError(completed.stderr.strip()[:400] or "git grep failed")
	lines = completed.stdout.splitlines()
	if not lines:
		return f"no match for {pattern!r}" + (f" in {glob!r}" if glob else "")
	body = "\n".join(lines[:MAX_TOOL_MATCHES])
	if len(lines) > MAX_TOOL_MATCHES:
		body += f"\n... ({len(lines) - MAX_TOOL_MATCHES} more match(es))"
	return body


def ToolListDir(path: str) -> str:
	target = ResolvePath(path)
	if not target.is_dir():
		raise ValueError(f"{path!r} is not a directory")
	entries = sorted(target.iterdir(), key=lambda entry: entry.name)
	rendered = [f"{entry.name}/" if entry.is_dir() else entry.name for entry in entries[:MAX_TOOL_ENTRIES]]
	if len(entries) > MAX_TOOL_ENTRIES:
		rendered.append(f"... ({len(entries) - MAX_TOOL_ENTRIES} more)")
	return "\n".join(rendered) or "(empty)"


def RunTool(name: str, arguments: dict, budget: list[int]) -> str:
	"""Run one tool call and answer text. Every failure becomes a message the model can act on."""
	if budget[0] <= 0:
		return "error: the context budget is exhausted, answer with what you have"
	try:
		if name == "read_file":
			text = ToolReadFile(str(arguments.get("path") or ""), arguments)
		elif name == "grep":
			text = ToolGrep(str(arguments.get("pattern") or ""), arguments)
		elif name == "list_dir":
			text = ToolListDir(str(arguments.get("path") or "."))
		else:
			return f"error: unknown tool {name!r}"
	except (OSError, ValueError, RuntimeError) as error:
		return f"error: {error}"
	if len(text) > budget[0]:
		text = text[: budget[0]] + "\n... (context budget reached)\n"
	budget[0] -= len(text)
	return text


def ReviewWithTools(system: str, user: str) -> tuple[str, dict]:
	"""Ask the model, letting it read the checkout before it answers.

	The diff and the full text of every changed file are already in the prompt, so a call is only
	for what they cannot show: a header, a caller, a build file, a test. The last round carries no
	tools, so the answer is always a completion and never another call.
	"""
	messages = [{"role": "system", "content": system}, {"role": "user", "content": user}]
	budget = [MAX_TOOL_CONTEXT_CHARS]
	usage: dict = {}
	for round in range(MAX_TOOL_ROUNDS + 1):
		last = round == MAX_TOOL_ROUNDS
		message, usage = CallChat(messages, None if last else TOOLS)
		calls = message.get("tool_calls") or []
		if not calls:
			return str(message.get("content") or ""), usage
		if last:
			raise RuntimeError("the model called a tool in the answering round")
		messages.append({"role": "assistant", "content": message.get("content") or "", "tool_calls": calls})
		for index, call in enumerate(calls):
			function = call.get("function") or {}
			if index >= MAX_TOOL_CALLS:
				# The API requires an answer for every call, even the ones over the cap.
				result = f"error: at most {MAX_TOOL_CALLS} tool calls per answer"
			else:
				result = UnpackToolCall(function, budget)
			messages.append({"role": "tool", "tool_call_id": call.get("id") or "", "content": result})
		print(f"tool round {round + 1}: {len(calls)} call(s), {budget[0]} context chars left")
	raise RuntimeError("the model kept calling tools after the last round")


def UnpackToolCall(function: dict, budget: list[int]) -> str:
	name = str(function.get("name") or "")
	try:
		arguments = json.loads(function.get("arguments") or "{}")
	except json.JSONDecodeError:
		return "error: the arguments are not a JSON object"
	if not isinstance(arguments, dict):
		return "error: the arguments are not a JSON object"
	return RunTool(name, arguments, budget)


def ParseAnswer(content: str) -> tuple[str, list[dict]]:
	text = FENCE_RE.sub("", content).strip()
	start = text.find("{")
	end = text.rfind("}")
	if start < 0 or end <= start:
		raise RuntimeError("the model answer contains no JSON object")
	data = json.loads(text[start : end + 1])
	if not isinstance(data, dict):
		raise RuntimeError("the model answer is not a JSON object")

	entries = data.get("findings")
	if not isinstance(entries, list):
		# `{}`, `{"summary": "..."}` and `{"findings": "none"}` all mean the answer is unusable;
		# treating them as "no findings" would turn a broken review into a green check.
		raise RuntimeError("the model answer has no `findings` list")

	findings: list[dict] = []
	for entry in entries:
		if not isinstance(entry, dict):
			continue
		path = UnquoteGitPath(str(entry.get("path") or "").strip())
		while path.startswith("./"):
			path = path[2:]
		severity = str(entry.get("severity") or "").strip().lower()
		detail = str(entry.get("detail") or "").strip()
		if severity not in SEVERITY_ORDER:
			# An unknown severity is not silently dropped: it becomes blocking and says so.
			detail = f"{detail}\n\nThe model reported the unknown severity `{severity or '(empty)'}`, treated as `medium`.".strip()
			severity = UNKNOWN_SEVERITY
		if not path:
			raise RuntimeError(f"the model returned a finding without a path: {entry!r}")
		line = entry.get("line")
		findings.append(
			{
				"path": path,
				"line": int(line) if isinstance(line, int) else 0,
				"severity": severity,
				"title": str(entry.get("title") or "").strip(),
				"detail": detail,
				"suggestion": entry.get("suggestion"),
			}
		)
	findings.sort(key=lambda item: SEVERITY_ORDER[item["severity"]])
	return str(data.get("summary") or "").strip(), findings[:MAX_FINDINGS]


def FormatComment(finding: dict) -> str:
	alert = {"high": "CAUTION", "medium": "WARNING"}.get(finding["severity"], "NOTE")
	label = finding["severity"].capitalize()
	lines = [f"> [!{alert}]", f"> **{label} — {finding['title'] or 'finding'}**", "", finding["detail"]]
	suggestion = finding["suggestion"]
	if isinstance(suggestion, str) and suggestion.strip():
		lines.extend(["", "```suggestion", suggestion.rstrip("\n"), "```"])
	return "\n".join(lines)


def NormalizeFindings(findings: list[dict], commentable: dict[str, set[int]]) -> tuple[list[dict], list[dict]]:
	"""Split findings into inline comments and the ones the review body has to carry.

	A model answer is untrusted, and GitHub validates a review request as a whole: one comment it
	cannot anchor fails every comment in it. A finding without a line of the 3-line diff — an
	unknown path, line 0, or a line only the 80-line view shows — therefore stays in the body.
	"""
	inline: list[dict] = []
	dropped: list[dict] = []
	seen: set[tuple[str, int, str]] = set()

	for finding in findings:
		path = finding["path"]
		if path not in commentable:
			dropped.append(finding)
			continue
		identity = (path, finding["line"], finding["title"])
		if identity in seen:
			continue
		seen.add(identity)

		if finding["line"] > 0 and finding["line"] in commentable[path] and len(inline) < MAX_INLINE_COMMENTS:
			inline.append({"path": path, "line": finding["line"], "side": "RIGHT", "body": FormatComment(finding)})
		else:
			dropped.append(finding)

	return inline, dropped


def FormatBody(summary: str, findings: list[dict], fileCount: int, dropped: list[dict], model: str) -> str:
	counts = {severity: sum(1 for finding in findings if finding["severity"] == severity) for severity in SEVERITY_ORDER}
	blocking = counts["high"] + counts["medium"]
	alert = "CAUTION" if blocking else "NOTE"
	verdict = f"{blocking} blocking, {counts['low']} non-blocking" if blocking else "No blocking issues"

	lines = [f"> [!{alert}]", f"> **Agent review — {verdict}**", ""]
	if summary:
		lines.extend([summary, ""])

	lines.extend(
		[
			"<details><summary>What was checked</summary>",
			"",
			f"- {fileCount} file(s) in the diff",
			"- conventions: `Tools/review_conventions.md`",
			f"- model: `{model}`",
			"",
			"</details>",
			"",
		]
	)

	if dropped:
		lines.extend(["<details><summary>Findings without a comment slot</summary>", ""])
		for finding in dropped:
			location = finding["path"] if finding["line"] <= 0 else f"{finding['path']}:{finding['line']}"
			lines.append(f"- `{finding['severity']}` `{location}` — {finding['title']}")
		lines.extend(["", "</details>"])

	return "\n".join(lines)


def SendReview(token: str, repo: str, prNumber: int, payload: dict) -> tuple[int, str]:
	request = urllib.request.Request(
		f"https://api.github.com/repos/{repo}/pulls/{prNumber}/reviews",
		data=json.dumps(payload).encode("utf-8"),
		method="POST",
		headers={
			"Accept": "application/vnd.github+json",
			"Authorization": f"Bearer {token}",
			"X-GitHub-Api-Version": "2022-11-28",
			"Content-Type": "application/json",
		},
	)
	try:
		with urllib.request.urlopen(request, timeout=60) as response:
			response.read()
	except urllib.error.HTTPError as error:
		return error.code, error.read()[:400].decode("utf-8", "replace")
	except urllib.error.URLError as error:
		return 0, str(error.reason)
	return 200, ""


def WriteStepSummary(body: str) -> None:
	"""Keep the review readable on the run page when it never reached the pull request."""
	path = os.environ.get("GITHUB_STEP_SUMMARY", "")
	if not path:
		return
	try:
		with open(path, "a", encoding="utf-8") as handle:
			handle.write(f"## Agent review (not posted)\n\n{body}\n")
	except OSError as error:
		print(f"::warning::the step summary was not written: {error}")


def PostReview(repo: str, prNumber: int, commitId: str, body: str, comments: list[dict]) -> None:
	"""Post the review, or fail: a review that did not land must not leave a green check.

	GitHub validates the request as a whole, so one comment it cannot anchor fails all of them.
	The body is worth more than the anchors, hence the body-only retry; when even that fails the
	body goes to the log and the step summary, because nothing else keeps it.
	"""
	token = os.environ.get("GITHUB_TOKEN", "")
	if not token:
		raise RuntimeError("GITHUB_TOKEN is not set, the review cannot be posted")

	payload = {"commit_id": commitId, "event": "COMMENT", "body": body, "comments": comments}
	status, detail = SendReview(token, repo, prNumber, payload)
	if status == 422 and comments:
		print("::warning::the review was rejected with its inline comments, retrying with the body alone")
		status, detail = SendReview(token, repo, prNumber, {**payload, "comments": []})
	if status not in (200, 201):
		print(body)
		WriteStepSummary(body)
		raise RuntimeError(f"posting the review failed: {status} {detail}")


def CollectDiff(baseSha: str, headSha: str) -> tuple[str, str]:
	"""Two diffs: one to read, one to anchor comments in.

	The prompt needs the neighbourhood, but a review comment's `line` is validated against
	GitHub's own patch, which carries three context lines. Deriving the whitelist from the wider
	diff would let a finding pick a line GitHub cannot anchor, and that fails the whole review.
	"""
	mergeBase = RunGit("merge-base", baseSha, headSha).strip()
	return (
		RunGit("diff", "--no-color", "--unified=80", mergeBase, headSha),
		RunGit("diff", "--no-color", "--unified=3", mergeBase, headSha),
	)


def ResolvePullRequest(event: dict, repo: str, prNumber: int) -> dict:
	pr = event.get("pull_request")
	if isinstance(pr, dict):
		return pr
	# A `workflow_run` payload carries no pull request object: the title, the body, both SHAs and
	# the labels come from the API instead. Without them the prompt would lose what a person wrote.
	if not repo or prNumber <= 0:
		raise RuntimeError("the event payload has no pull_request and PR_NUMBER is not set")
	return FetchPullRequest(repo, prNumber)


def FetchPullRequest(repo: str, prNumber: int) -> dict:
	token = os.environ.get("GITHUB_TOKEN", "")
	if not token:
		raise RuntimeError("GITHUB_TOKEN is not set, the pull request cannot be read")
	request = urllib.request.Request(
		f"https://api.github.com/repos/{repo}/pulls/{prNumber}",
		headers={
			"Accept": "application/vnd.github+json",
			"Authorization": f"Bearer {token}",
			"X-GitHub-Api-Version": "2022-11-28",
		},
	)
	try:
		with urllib.request.urlopen(request, timeout=60) as response:
			return json.loads(response.read().decode("utf-8"))
	except urllib.error.HTTPError as error:
		detail = error.read()[:400].decode("utf-8", "replace")
		raise RuntimeError(f"reading pull request #{prNumber} failed: {error.code} {detail}") from error
	except urllib.error.URLError as error:
		raise RuntimeError(f"reading pull request #{prNumber} failed: {error.reason}") from error


def SelfTest() -> int:
	def Parse(diff: str) -> tuple[list[str], dict[str, set[int]], dict[str, str]]:
		return ParseDiff(diff)

	# An added line whose content is "++ x;" must not be mistaken for a +++ header.
	order, commentable, chunks = Parse(
		"\n".join(
			[
				"diff --git a/a.cpp b/a.cpp",
				"--- a/a.cpp",
				"+++ b/a.cpp",
				"@@ -1,2 +1,3 @@",
				" int main()",
				" {",
				"+++ x;",
			]
		)
	)
	assert order == ["a.cpp"], order
	assert commentable["a.cpp"] == {1, 2, 3}, commentable
	assert "a.cpp" in chunks

	# A quoted non-ASCII path has to be decoded before it can be matched against the worktree.
	order, commentable, _ = Parse(
		"\n".join(
			[
				r'diff --git "a/docs/\344\270\255\346\226\207.md" "b/docs/\344\270\255\346\226\207.md"',
				r'--- "a/docs/\344\270\255\346\226\207.md"',
				r'+++ "b/docs/\344\270\255\346\226\207.md"',
				"@@ -1,0 +2 @@",
				"+more",
			]
		)
	)
	assert order == ["docs/中文.md"], order
	assert commentable["docs/中文.md"] == {2}, commentable

	# A binary diff has no +++ line and no commentable lines.
	order, commentable, _ = Parse("diff --git a/x.bin b/x.bin\nBinary files a/x.bin and b/x.bin differ")
	assert order == [], order

	# Deleted files stay out of the review.
	order, _, _ = Parse("diff --git a/gone.cpp b/gone.cpp\n--- a/gone.cpp\n+++ /dev/null\n@@ -1,0 +0,0 @@\n-x")
	assert order == [], order

	summary, findings = ParseAnswer(
		'{"summary": "s", "findings": [{"path": "a.cpp", "line": 3, "severity": "High", "title": "t", "detail": "d"}]}'
	)
	assert summary == "s" and findings[0]["severity"] == "high", findings

	_, findings = ParseAnswer(
		'{"summary": "s", "findings": [{"path": "a.cpp", "line": 1, "severity": "critical", "title": "t", "detail": "d"}]}'
	)
	assert findings[0]["severity"] == "medium", findings
	assert "unknown severity" in findings[0]["detail"], findings[0]

	for broken in ('{}', '{"summary": "s"}', '{"summary": "s", "findings": "none"}'):
		try:
			ParseAnswer(broken)
		except RuntimeError:
			pass
		else:
			raise AssertionError(f"ParseAnswer accepted {broken!r}")

	# Nothing becomes a file-level comment: `subject_type` belongs to the single comment endpoint,
	# and a comment without a line fails the whole review, so those findings go to the body.
	commentable = {"a.cpp": {1, 2}}
	mixed = [
		{"path": "a.cpp", "line": 99, "severity": "high", "title": "out of range", "detail": "d", "suggestion": None},
		{"path": "zzz.cpp", "line": 1, "severity": "low", "title": "unknown path", "detail": "d", "suggestion": None},
	]
	inline, dropped = NormalizeFindings(mixed, commentable)
	assert not inline, inline
	assert [item["path"] for item in dropped] == ["a.cpp", "zzz.cpp"], dropped

	# A line that only the 80-line view shows is not commentable: the whitelist comes from the
	# 3-line diff GitHub itself anchors comments in.
	wide = (
		"diff --git a/a.cpp b/a.cpp\n--- a/a.cpp\n+++ b/a.cpp\n@@ -1,6 +1,13 @@\n"
		" one\n two\n three\n four\n five\n six\n+seven\n eight\n nine\n ten\n eleven\n twelve\n thirteen\n"
	)
	narrow = (
		"diff --git a/a.cpp b/a.cpp\n--- a/a.cpp\n+++ b/a.cpp\n@@ -4,3 +4,4 @@\n"
		" four\n five\n six\n+seven\n eight\n nine\n ten\n"
	)
	_, wideCommentable, _ = Parse(wide)
	_, narrowCommentable, _ = Parse(narrow)
	assert narrowCommentable["a.cpp"] < wideCommentable["a.cpp"], (narrowCommentable, wideCommentable)
	inline, dropped = NormalizeFindings(
		[{"path": "a.cpp", "line": 12, "severity": "high", "title": "far context", "detail": "d", "suggestion": None}],
		narrowCommentable,
	)
	assert not inline and [item["line"] for item in dropped] == [12], (inline, dropped)

	fileComment = FormatComment({"severity": "high", "title": "t", "detail": "d", "suggestion": "x"})
	assert "```suggestion" in fileComment, fileComment

	assert UnquoteGitPath('"b/a b.md"') == "b/a b.md"
	assert UnquoteGitPath("b/a.cpp") == "b/a.cpp"

	# The conventions file opens with the repository SPDX header; the prompt must start at the
	# first real line, while a comment that sits inside the file stays.
	header = "<!-- SPDX-License-Identifier: Apache-2.0 -->\n<!-- Copyright(c) x -->\n\n# Title\n\n- rule"
	assert StripHeaderComments(header) == "# Title\n\n- rule", StripHeaderComments(header)
	assert StripHeaderComments("# Title\n\n- rule") == "# Title\n\n- rule"
	inner = "# Title\n\n<!-- keep me -->\n\n- rule"
	assert StripHeaderComments(inner) == inner, StripHeaderComments(inner)

	# A tool path may not leave the checkout, and a refusal is a message for the model rather
	# than an exception that aborts the review.
	for escape in ("../", "/etc/passwd", ".git/config"):
		try:
			ResolvePath(escape)
		except ValueError:
			pass
		else:
			raise AssertionError(f"ResolvePath accepted {escape!r}")
	assert RunTool("nope", {}, [1000]).startswith("error:")
	assert RunTool("read_file", {"path": "../x"}, [1000]).startswith("error:")

	if (Path.cwd() / "Tools" / "agent_review.py").is_file():
		head = RunTool("read_file", {"path": "Tools/agent_review.py", "start": 1, "end": 2}, [100_000])
		assert "SPDX" in head, head

	# Any `ci:*` label requests CI; a new matrix label needs no change here.
	assert CiLabels({"labels": [{"name": "ci:cpp"}]}) == ["ci:cpp"]
	assert CiLabels({"labels": [{"name": "ci:docs"}, {"name": "ci:cpp"}]}) == ["ci:docs", "ci:cpp"]
	assert CiLabels({"labels": [{"name": "bug"}]}) == []
	assert CiLabels({}) == []
	override = os.environ.pop("PXC_CI_STATE", None)
	assert "`ci:docs`" in CiContext({"labels": [{"name": "ci:docs"}]})
	if override is not None:
		os.environ["PXC_CI_STATE"] = override

	# A `workflow_run` payload has no pull request object; without a number to fetch, that is an
	# error rather than a review of nothing.
	payload = {"pull_request": {"title": "t"}}
	assert ResolvePullRequest(payload, "o/r", 1) is payload["pull_request"]
	try:
		ResolvePullRequest({}, "", 0)
	except RuntimeError:
		pass
	else:
		raise AssertionError("ResolvePullRequest accepted a payload without a pull request")

	print("self-test ok")
	return 0


def ParseArguments() -> argparse.Namespace:
	parser = argparse.ArgumentParser(description="Review a pull request with an OpenAI-compatible model.")
	parser.add_argument("--self-test", action="store_true", help="run the parsing tests and exit")
	parser.add_argument("--dry-run", action="store_true", help="build the prompt, do not call the model and do not post")
	parser.add_argument("--diff-file", help="read the diff from this file instead of git")
	return parser.parse_args()


def main() -> int:
	# A Windows console is not UTF-8 and a diff can contain non-ASCII text.
	sys.stdout.reconfigure(encoding="utf-8", errors="replace")
	sys.stderr.reconfigure(encoding="utf-8", errors="replace")

	arguments = ParseArguments()
	if arguments.self_test:
		return SelfTest()

	repo = os.environ.get("GITHUB_REPOSITORY", "")
	prNumber = int(os.environ.get("PR_NUMBER") or 0)
	baseSha = os.environ.get("BASE_SHA", "")
	headSha = os.environ.get("HEAD_SHA", "")
	pr: dict = {}

	if arguments.diff_file:
		diffText = Path(arguments.diff_file).read_text(encoding="utf-8", errors="replace")
		# A local diff has no other rendering: its own hunks are the only whitelist available.
		commentDiffText = diffText
	else:
		event = ReadEvent()
		pr = ResolvePullRequest(event, repo, prNumber)
		repo = repo or (event.get("repository") or {}).get("full_name", "")
		prNumber = prNumber or int(pr.get("number") or 0)
		baseSha = baseSha or str((pr.get("base") or {}).get("sha") or "")
		headSha = headSha or str((pr.get("head") or {}).get("sha") or "")

		# The review runs in the privileged `workflow_run` job, so a fork gets the secret and the
		# write token as well; a missing key is a configuration error and fails below instead of
		# being skipped.
		diffText, commentDiffText = CollectDiff(baseSha, headSha)

	if not diffText.strip():
		print("no changes in the pull request, nothing to review")
		return 0

	# `files` and `chunks` come from the readable diff, `commentable` from the one GitHub anchors
	# comments in; the two differ by more than 70 lines of context per side.
	files, _, chunks = ParseDiff(diffText)
	if not files:
		print("no reviewable files in the diff, nothing to review")
		return 0
	_, commentable, _ = ParseDiff(commentDiffText)
	print(f"{len(files)} file(s) in the diff")

	language = os.environ.get("PXC_REVIEW_LANGUAGE") or DEFAULT_LANGUAGE
	system, user = BuildPrompt(pr, files, chunks, ReadConventions(), language)
	print(f"prompt: {len(system) + len(user)} chars")

	if arguments.dry_run:
		print("::notice::dry run, no API call and no review posted")
		print(user[:800])
		return 0

	if not os.environ.get("PXC_REVIEW_API_KEY"):
		raise RuntimeError("PXC_REVIEW_API_KEY is not set, add it as a repository secret")

	toolsEnabled = (os.environ.get("PXC_REVIEW_TOOLS") or "on").strip().lower() not in ("off", "0", "false")
	print(f"repository tools: {'on' if toolsEnabled else 'off'}")
	if not toolsEnabled:
		content, usage = CallModel(system, user)
	else:
		try:
			content, usage = ReviewWithTools(system, user)
		except ToolsUnsupported:
			# An endpoint without function calling still gets a review: it sees the same prompt,
			# just without the option to read a file that is not in it.
			print("::warning::the endpoint rejected tools, retrying without them")
			content, usage = CallModel(system, user)
	print(f"usage: {json.dumps(usage)}")
	summary, findings = ParseAnswer(content)
	inline, dropped = NormalizeFindings(findings, commentable)

	model = os.environ.get("PXC_REVIEW_MODEL") or DEFAULT_MODEL
	body = FormatBody(summary, findings, len(files), dropped, model)
	blocking = [finding for finding in findings if finding["severity"] in BLOCKING_SEVERITIES]

	print(f"findings: {len(findings)} ({len(blocking)} blocking), comments: {len(inline)}")
	# Raises when the review cannot be posted: a review nobody can read must not leave a green
	# check.
	PostReview(repo, prNumber, headSha, body, inline)

	if blocking:
		print(f"::error::{len(blocking)} blocking finding(s), see the review")
		return 1
	return 0


if __name__ == "__main__":
	try:
		raise SystemExit(main())
	except SystemExit:
		raise
	except Exception as error:
		# Fail closed: a review that could not run must not look like a review that passed.
		print(f"::error::agent review failed: {error}")
		raise SystemExit(1)
