# Issue tracker: GitHub

Issues and PRDs for this repository live as GitHub Issues in `evildarkarchon/libbsa`.

Inside Codex, prefer the connected GitHub app for issue operations. In other environments, use an authenticated `gh` CLI from this repository clone; infer the repository from `git remote -v`.

## Conventions

- **Create an issue**: use the GitHub connector's issue-creation operation, or `gh issue create --title "..." --body-file <path>`.
- **Read an issue**: use the GitHub connector's issue-fetch operation, or `gh issue view <number> --comments`.
- **List issues**: use the GitHub connector's issue search, or `gh issue list --state open --json number,title,body,labels,comments` with appropriate `--label` and `--state` filters.
- **Comment on an issue**: use the GitHub connector's comment operation, or `gh issue comment <number> --body-file <path>`.
- **Apply or remove labels**: use the GitHub connector's label operations, or `gh issue edit <number> --add-label "..."` / `--remove-label "..."`.
- **Close an issue**: use the GitHub connector's issue-update operation, or `gh issue close <number> --comment "..."`.

Use `--body-file` for multiline CLI content so commands remain reliable in PowerShell.

## Pull requests as a triage surface

**PRs as a request surface: no.**

Do not include pull requests in the triage queue. GitHub shares one number space across issues and pull requests, so resolve an ambiguous `#42` by checking the object type before modifying it.

## When a skill says "publish to the issue tracker"

Create a GitHub issue in `evildarkarchon/libbsa`.

## When a skill says "fetch the relevant ticket"

Fetch the GitHub issue, including its body, labels, and comments.

## Wayfinding operations

Used by `/wayfinder`. The **map** is a single issue with **child** issues as tickets.

- **Map**: a single issue labelled `wayfinder:map`, holding the Notes, Decisions-so-far, and Fog sections.
- **Child ticket**: an issue linked to the map as a GitHub sub-issue. If sub-issues are unavailable, add the child to a task list in the map body and put `Part of #<map>` at the top of the child body. Use `wayfinder:<type>` labels (`research`, `prototype`, `grilling`, or `task`).
- **Blocking**: use GitHub's native issue dependencies. If dependencies are unavailable, add a `Blocked by: #<n>, #<n>` line at the top of the child body.
- **Frontier query**: inspect the map's open children in map order and select the first ticket with no open blocker and no assignee.
- **Claim**: assign the selected issue to the driving developer; this is the session's first write.
- **Resolve**: comment with the answer, close the child issue, and append its durable context pointer to the map's Decisions-so-far section.
