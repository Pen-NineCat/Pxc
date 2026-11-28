# SPDX-License-Identifier: Apache-2.0
# Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
# https://github.com/Pen-NineCat

"""Move one pull request's item on the "Pxc Code Reviews" project to a given status.

The review flow has exactly two automatic transitions: a push resets the item to R1 (waiting for
the agent review), and a finished agent review moves it to R2 (waiting for a maintainer). Every
other status is set by a person or by a Projects built-in automation, so this script never
overwrites a status it did not expect: a maintainer who moved the item to Ready does not have it
dragged back to R2 by a review that finished after that decision.

`GITHUB_TOKEN` carries repository scope only and cannot write a project, so the workflows supply a
token with the `project` scope in `PXC_PROJECT_TOKEN`. Without it the transition is skipped with a
warning instead of reddening a job on every push.

Environment:
    PXC_PROJECT_TOKEN    token with the `project` scope (repository secret)
    PXC_PROJECT_OWNER    login of the project owner, e.g. Pen-NineCat
    PXC_PROJECT_NUMBER   project number, the 5 in github.com/users/Pen-NineCat/projects/5
    PXC_PR_NUMBER        pull request number
    PXC_STATUS_TO        status option to set, e.g. R1
    PXC_STATUS_FROM      status the item must currently have, empty to overwrite any status
    GITHUB_REPOSITORY    owner/repo, set by Actions

Usage:
    python Tools/review_state.py
    python Tools/review_state.py --dry-run
    python Tools/review_state.py --self-test
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import urllib.error
import urllib.request

API = "https://api.github.com"
STATUS_FIELD = "Status"

# A user-owned project answers `user`, an organization-owned one `organization`; both aliases run
# in one request so the caller does not have to know which kind the login is.
PROJECT_QUERY = """
query($login: String!, $number: Int!) {
  user(login: $login) { projectV2(number: $number) { ...Project } }
  organization(login: $login) { projectV2(number: $number) { ...Project } }
}
fragment Project on ProjectV2 {
  id
  field(name: "Status") { ... on ProjectV2SingleSelectField { id options { id name } } }
}
"""

# `fieldValueByName` and not `fieldValues(first: 100)`: the item may sit in several projects, and
# only the status of the one this flow owns is interesting.
ITEM_QUERY = """
query($owner: String!, $name: String!, $number: Int!) {
  repository(owner: $owner, name: $name) {
    pullRequest(number: $number) {
      state
      projectItems(first: 20) {
        nodes {
          id
          project { number }
          fieldValueByName(name: "Status") { ... on ProjectV2ItemFieldSingleSelectValue { name } }
        }
      }
    }
  }
}
"""

SET_STATUS_MUTATION = """
mutation($project: ID!, $item: ID!, $field: ID!, $option: String!) {
  updateProjectV2ItemFieldValue(
    input: { projectId: $project, itemId: $item, fieldId: $field, value: { singleSelectOptionId: $option } }
  ) {
    projectV2Item { id }
  }
}
"""


def Post(query: str, variables: dict, token: str) -> dict:
	"""Run one GraphQL request and return its `data`. Errors are hard: a silent no-op hides drift."""
	request = urllib.request.Request(
		f"{API}/graphql",
		data=json.dumps({"query": query, "variables": variables}).encode("utf-8"),
		method="POST",
		headers={
			"Accept": "application/vnd.github+json",
			"Authorization": f"Bearer {token}",
			"Content-Type": "application/json",
		},
	)
	try:
		with urllib.request.urlopen(request, timeout=30) as response:
			payload = json.loads(response.read().decode("utf-8"))
	except urllib.error.HTTPError as error:
		detail = error.read()[:400].decode("utf-8", "replace")
		raise RuntimeError(f"the API answered {error.code}: {detail}") from error
	except urllib.error.URLError as error:
		raise RuntimeError(f"the API is unreachable: {error.reason}") from error
	errors = payload.get("errors")
	if errors:
		raise RuntimeError("; ".join(str(item.get("message")) for item in errors))
	return payload.get("data") or {}


def Decide(current: str, source: str, target: str) -> str:
	"""Return `set`, `done` or `skip` for one item.

	`done` and `skip` both mean "write nothing": the first because the item already has the
	target status, the second because a person moved it somewhere this flow does not own.
	"""
	if current == target:
		return "done"
	if source and current != source:
		return "skip"
	return "set"


def MatchOption(options: list[dict], name: str) -> str:
	"""Return the option id whose name is exactly `name`, or an empty string."""
	for option in options:
		if str(option.get("name") or "") == name:
			return str(option.get("id") or "")
	return ""


def FindItem(token: str, owner: str, repo: str, pullNumber: int, projectNumber: int) -> dict | None:
	"""Return the item of this pull request inside `projectNumber`, with its current status."""
	data = Post(ITEM_QUERY, {"owner": owner, "name": repo, "number": pullNumber}, token)
	pullRequest = ((data.get("repository") or {}).get("pullRequest")) or {}
	for node in ((pullRequest.get("projectItems") or {}).get("nodes")) or []:
		if int((node.get("project") or {}).get("number") or 0) != projectNumber:
			continue
		value = node.get("fieldValueByName") or {}
		return {
			"id": str(node.get("id") or ""),
			"status": str(value.get("name") or ""),
			"state": str(pullRequest.get("state") or ""),
		}
	return None


def FindProject(token: str, login: str, number: int) -> dict:
	"""Return the project's id, its `Status` field id and that field's options."""
	data = Post(PROJECT_QUERY, {"login": login, "number": number}, token)
	project = ((data.get("user") or {}).get("projectV2")) or ((data.get("organization") or {}).get("projectV2"))
	if not project:
		raise RuntimeError(f"project #{number} of {login} is not visible to the token")
	field = project.get("field") or {}
	if not field:
		raise RuntimeError(f"project #{number} has no field named {STATUS_FIELD}")
	return {
		"id": str(project.get("id") or ""),
		"field": str(field.get("id") or ""),
		"options": list(field.get("options") or []),
	}


def SetStatus(token: str, projectId: str, itemId: str, fieldId: str, optionId: str) -> None:
	data = Post(
		SET_STATUS_MUTATION,
		{"project": projectId, "item": itemId, "field": fieldId, "option": optionId},
		token,
	)
	if not (data.get("updateProjectV2ItemFieldValue") or {}).get("projectV2Item"):
		raise RuntimeError("the status mutation returned no item")


def SelfTest() -> int:
	assert Decide("R1", "R1", "R2") == "set"
	assert Decide("R2", "R1", "R2") == "done"
	assert Decide("Ready", "R1", "R2") == "skip"
	assert Decide("Ready", "", "R1") == "set"
	assert Decide("", "R1", "R2") == "skip"
	assert MatchOption([{"id": "a", "name": "R1"}], "R1") == "a"
	assert MatchOption([{"id": "a", "name": "R1"}], "R2") == ""
	assert MatchOption([], "R1") == ""
	print("self-test ok")
	return 0


def ParseArguments() -> argparse.Namespace:
	parser = argparse.ArgumentParser(description="Move a pull request's project item to a status.")
	parser.add_argument("--self-test", action="store_true", help="run the pure tests and exit")
	parser.add_argument("--dry-run", action="store_true", help="print the transition, do not write")
	return parser.parse_args()


def main() -> int:
	arguments = ParseArguments()
	if arguments.self_test:
		return SelfTest()

	token = os.environ.get("PXC_PROJECT_TOKEN", "")
	if not token:
		print("::warning::PXC_PROJECT_TOKEN is not set, the project status is not updated")
		return 0

	owner, _, repo = os.environ.get("GITHUB_REPOSITORY", "").partition("/")
	pullNumber = int(os.environ.get("PXC_PR_NUMBER") or 0)
	projectOwner = os.environ.get("PXC_PROJECT_OWNER", "")
	projectNumber = int(os.environ.get("PXC_PROJECT_NUMBER") or 0)
	target = os.environ.get("PXC_STATUS_TO", "")
	source = os.environ.get("PXC_STATUS_FROM", "")

	if not owner or not repo:
		raise RuntimeError("GITHUB_REPOSITORY is not set, this script is meant to run in Actions")
	if pullNumber <= 0:
		raise RuntimeError("PXC_PR_NUMBER is not a pull request number")
	if not projectOwner or projectNumber <= 0:
		raise RuntimeError("PXC_PROJECT_OWNER and PXC_PROJECT_NUMBER name the project")
	if not target:
		raise RuntimeError("PXC_STATUS_TO is the status to set")

	item = FindItem(token, owner, repo, pullNumber, projectNumber)
	if item is None:
		print(f"::warning::#{pullNumber} is not on project #{projectNumber}, no status to move")
		return 0
	if item["state"] != "OPEN":
		print(f"#{pullNumber} is {item['state']}, its status is left alone")
		return 0

	project = FindProject(token, projectOwner, projectNumber)
	option = MatchOption(project["options"], target)
	if not option:
		names = ", ".join(str(entry.get("name")) for entry in project["options"])
		raise RuntimeError(f"project #{projectNumber} has no {STATUS_FIELD} option {target!r}; it has: {names}")

	decision = Decide(item["status"], source, target)
	if decision == "done":
		print(f"#{pullNumber} is already {target}, nothing to do")
		return 0
	if decision == "skip":
		print(f"#{pullNumber} is {item['status'] or '(no status)'}, not {source}; leaving it to a person")
		return 0

	if arguments.dry_run:
		print(f"::notice::dry run: #{pullNumber} {item['status'] or '(no status)'} -> {target}")
		return 0

	SetStatus(token, project["id"], item["id"], project["field"], option)
	print(f"#{pullNumber} {item['status'] or '(no status)'} -> {target}")
	return 0


if __name__ == "__main__":
	try:
		raise SystemExit(main())
	except SystemExit:
		raise
	except Exception as error:
		# A transition that could not run must be visible: the item would silently keep a status
		# nobody is waiting on.
		print(f"::error::the project status was not updated: {error}")
		raise SystemExit(1)
