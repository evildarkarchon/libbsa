# Issue tracker: GitHub

Issues and specs for this repo live as GitHub issues in `evildarkarchon/libbsa`. Use the `gh` CLI for issue operations. Run every `gh` command outside the sandbox because authentication uses the user's Windows Credential Manager.

## Conventions

- **Create an issue:** `gh issue create --title "..." --body-file <file>`. Put multiline bodies in a temporary file.
- **Read an issue:** `gh issue view <number> --json number,title,body,state,labels,comments,url` for the issue, labels, and discussion; use `gh issue view <number> --comments` for a readable view.
- **List issues:** `gh issue list --state open --json number,title,body,labels` with appropriate `--label` and `--state` filters.
- **Comment on an issue:** `gh issue comment <number> --body-file <file>` for multiline comments.
- **Apply or remove labels:** `gh issue edit <number> --add-label "..."` or `--remove-label "..."`.
- **Close an issue:** `gh issue close <number> --comment "..."` when an outcome needs recording.

Run these commands inside this clone so `gh` resolves `evildarkarchon/libbsa` from `origin`. Verify the target before writing when a command could refer to another repository or issue. Use the label names in `docs/agents/triage-labels.md` for triage states.

## Pull requests as a triage surface

**PRs as a request surface: no.** _(Set to `yes` if this repo treats external PRs as feature requests; `/triage` reads this flag.)_

When set to `yes`, PRs run through the same labels and states as issues, using the `gh pr` equivalents:

- **Read a PR:** `gh pr view <number> --comments` and `gh pr diff <number>` for the diff.
- **List external PRs for triage:** `gh pr list --state open --json number,title,body,labels,author,authorAssociation,comments`, then keep `CONTRIBUTOR`, `FIRST_TIME_CONTRIBUTOR`, or `NONE` associations.
- **Comment, label, or close:** `gh pr comment`, `gh pr edit --add-label` or `--remove-label`, and `gh pr close`.

GitHub shares one number space across issues and PRs. Resolve an ambiguous `#<number>` with `gh pr view <number>` and then `gh issue view <number>` if needed.

## When a skill says "publish to the issue tracker"

Create a GitHub issue.

## When a skill says "fetch the relevant ticket"

Read the GitHub issue, including its labels and comments, with `gh issue view <number> --json number,title,body,state,labels,comments,url`.

## Wayfinding operations

Used by `/wayfinder`. The **map** is a single issue with **child** issues as tickets.

- **Map:** a single issue labelled `wayfinder:map`, holding the Notes / Decisions-so-far / Fog body. Create it with `gh issue create --label wayfinder:map`.
- **Child ticket:** an issue linked to the map as a GitHub sub-issue (`gh api` on the sub-issues endpoint). Where sub-issues aren't enabled, add the child to a task list in the map body and put `Part of #<map>` at the top of the child body. Labels: `wayfinder:<type>` (`research`/`prototype`/`grilling`/`task`). Once claimed, the ticket is assigned to the driving developer.
- **Blocking:** GitHub's native issue dependencies. Add an edge with `gh api --method POST repos/<owner>/<repo>/issues/<child>/dependencies/blocked_by -F issue_id=<blocker-db-id>`, where the blocker's numeric database ID comes from `gh api repos/<owner>/<repo>/issues/<n> --jq .id`. Where dependencies aren't available, use a `Blocked by: #<n>, #<n>` line at the top of the child body. A ticket is unblocked when every blocker is closed.
- **Frontier query:** list the map's open children, exclude assigned issues and issues with open blockers (`issue_dependencies_summary.blocked_by > 0`, or an open issue in the `Blocked by` line); first in map order wins.
- **Claim:** `gh issue edit <n> --add-assignee @me`, the session's first write.
- **Resolve:** comment with the answer, close the child, then append a context pointer (gist and link) to the map's Decisions-so-far.
