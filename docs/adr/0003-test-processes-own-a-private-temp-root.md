# ADR-0003: Test processes own a private temp root, installed through the process environment

- Status: Accepted
- Date: 2026-08-14

## Context

The BA2 DX10 snapshot-cleanup tests decide which `libbsa-dx10-snapshot-*` directory belongs to the
writer they just ran by scanning the temp root before the write and again after, and treating
everything new as writer-owned. That diff is the only attribution available, because the production
name deliberately carries no owner identity: `make_unique_snapshot_directory()`
(`src/formats/ba2/ba2_dx10_snapshot_builder.cpp`) composes `libbsa-dx10-snapshot-` plus a 128-bit
`BCryptGenRandom` suffix, and *"BA2 DX10 writer snapshot directory names are unpredictable"*
(`tests/unit/ba2_dx10_writer_tests.cpp`) pins that as a property worth keeping.

In a temp root shared with every other process on the machine, the diff is wrong the moment anything
else creates a snapshot directory inside the window. The cleanup helper then fails
`CHECK_FALSE(exists(path))` on a directory whose owner is still using it, and — worse — calls
`remove_all` on it. Nothing in that helper distinguishes a second test binary from a real libbsa tool
run by the user, so the test suite could destroy a live snapshot directory belonging to someone's
actual work. Issue #60 has the reproduction and the full account.

Fixing the helper alone does not fix the attribution. As long as the process can *see* another
process's snapshot directories, any scan-and-diff scheme is guessing.

## Decision

Every `libbsa_tests` process creates a uniquely named directory under the real system temp root
before any test body runs, points its own `TMP` and `TEMP` at that directory, and removes it when the
process exits. Both test code and the libbsa library under test therefore resolve
`std::filesystem::temp_directory_path()` to that private root.

The isolation lives in the **process environment**, not in a library seam.

That is the part of this decision that needed recording, because the obvious alternative — an
injectable snapshot root on the BA2 DX10 writer — looks cleaner and is not available here.
`make_unique_snapshot_directory()` resolves its root at a single call site in an anonymous namespace:
no option field, no environment override, nothing linkable from a test. The one existing seam,
`ba2_dx10_ensure_snapshot_directory`, takes an already-chosen directory rather than the root. Adding
an injectable root would therefore be production code that exists solely for test isolation, which
`AGENTS.md` forbids outright. Repointing the environment adds no seam to the library at all, and
covers the production call site precisely *because* it does not go through one.

The mechanism is `SetEnvironmentVariableW`, pinned by issue #61 and by
`tests/unit/temp_directory_redirection_tests.cpp`. MSVC's `temp_directory_path()` reaches
`GetTempPath2W` on every call and no layer in that chain holds a static, so resolution is per-call and
a mid-process repoint takes effect immediately. `GetTempPath2W` reads the Win32 process environment
block, which is a different store from the CRT copy that `std::getenv` serves; both are written, and
a repointed value must be read back with `GetEnvironmentVariableW`.

Catch2 supplies `main()` for this binary (`Catch2::Catch2WithMain`), so the hook is a
`CATCH_REGISTER_LISTENER` event listener rather than a custom `main`. It lives in
`tests/support/private_temp_root.cpp`, the repository's first shared test-support translation unit.
It cannot live in the force-included `tests/unit/test_source_root.hpp`, which reaches every
translation unit and would register the listener dozens of times.

The production snapshot name is unchanged. In particular no process identity is added to it:
unpredictability is a pinned property, and encoding process identity in a world-readable temp path
would leak information for no benefit. Ownership is a test-side question and stays on the test side.

## Consequences

- Library code is untouched. There is no new option, no new seam, and no test-only branch in
  `src/`, so nothing here has to be justified against the `AGENTS.md` rule about production code kept
  alive for test compatibility.
- The before/after snapshot-directory diff becomes a correct ownership proxy. A process cannot see
  another process's snapshot directories, so it cannot misattribute one or delete one.
- The private root also covers the quieter version of the same hazard: the fixed temp-derived paths
  that many test translation units use (`libbsa_ba2_dx10_writer_tests`,
  `libbsa-ba2-dx10-partial-overlap.ba2`, and so on) can no longer collide with another *process*.
  They can still collide with each other inside one process, which is why `ctest -j` remains out of
  scope and needs its own work.
- Setup and teardown run once per **test case**, not once per suite run: `catch_discover_tests`
  registers one CTest test per `TEST_CASE`, so each case is its own process. Setup is one
  `CreateDirectory`, one `CreateFile`, four environment writes, and one sweep; teardown is one
  `CloseHandle` plus one `remove_all`. Anything more expensive would be paid several hundred times
  per lane.
- The private root's location is exposed through `libbsa::tests::private_temp_root()`, and the
  original system root through `libbsa::tests::system_temp_root()`. A test that needs to talk about
  the shared root — to assert something did *not* land there — has to ask for it, because
  `temp_directory_path()` no longer names it.
- Any test that repoints the temp environment itself now nests under the private root rather than
  under the system root. `temp_directory_redirection_tests.cpp` already restores what it changes, so
  this is transparent, but a future test that leaks a repoint would leak it inside the private root
  and be cleaned up with it.
- A process running as **SYSTEM** cannot isolate itself this way: `GetTempPath2W` ignores `TMP` and
  `TEMP` entirely for SYSTEM on Windows 11 and later and returns `%SystemRoot%\SystemTemp`. The
  listener fails loudly there rather than silently sharing a root. Running the suite as SYSTEM is out
  of scope.
- The root must fit in `MAX_PATH`, because the STL passes a fixed 261 wide-character buffer to
  `GetTempPath2W` and reports an overlong result as `not_a_directory` rather than as a length error.
  The listener checks the length itself so the failure names its real cause, and the root name is
  kept short (`libbsa-tests-<pid>-<4 hex>`) because every path the suite builds nests under it.
- Removal at exit is best-effort, and stays that way: a hard kill leaves a root behind. What keeps
  that from accumulating is the start-up sweep in the amendment below, not a stronger teardown.
- Every root costs one open handle for the life of its process, and every start-up costs one
  directory-search syscall pair plus one `CreateFile` per leftover found. The usual leftover count is
  zero, which is what makes this affordable at one sweep per test case.

## Scope: the test binary only

Only `libbsa_tests` carries the listener. The fixture-generator tools
(`generate_ba2_dx10_fixtures_tool` and its siblings) drive the same BA2 DX10 writer and therefore
still create snapshot directories in the shared system temp root. That is deliberate: they are
`add_custom_target` tools run on demand to regenerate committed fixtures, not part of the CTest graph,
so they are neither a source of the interference issue #60 reports nor a victim of it. Nothing in this
decision stops a later ticket from giving them the same treatment if that changes.

## What this does not do

It does not make the BA2 DX10 cleanup helpers *structurally* incapable of deleting outside their own
root — under a private root that is true by construction, but the rule should be stated rather than
inferred (issue #64). It does not detect a genuinely concurrent second instance (issue #65), and it
does not, by itself, prove the concurrency guarantee: that needs a test that actually runs two
instances (issue #66).

## Amendment (2026-08-14): liveness is an open handle, never an age

A best-effort teardown accumulates. A run that crashed, was killed, or was force-terminated leaves its
root behind, so start-up sweeps the system temp root for roots no live process owns.

Sweeping is a destructive scan over directories the process does not own — structurally the same
operation that caused the defect above — so the rule that decides what may be deleted is the decision
worth recording, not the sweep itself.

**A root may be removed only when this process can open its owner marker with exclusive access.** Every
root's owner creates `.libbsa-tests-owner` inside the root at install time and holds that handle with
`dwShareMode == 0` until uninstall. A second opener of a share-nothing handle is refused with
`ERROR_SHARING_VIOLATION`, so an open that succeeds proves no process holds the marker.

The rule is a handle rather than an age because Windows destroys a process's entire handle table when
the process terminates, whatever terminated it — a clean exit, an unhandled exception,
`TerminateProcess`, or the ASan lane aborting. The kernel maintains the liveness bit for us, and it
cannot get stuck in either direction: there is no path where a dead process still holds a handle, and
none where a live one has silently lost it.

An age threshold has neither property, which is why it was rejected as the primary rule. A long ASan
lane run can hold a root open for longer than any cutoff worth picking, and deleting it would
reintroduce exactly the cross-process destruction this ADR exists to remove.

One secondary branch is gated on age *in addition to* the rule above, never instead of it: a candidate
root carrying **no marker at all** is removed once its creation time is older than an hour. A live
owner is only ever in that state for the few microseconds between `create_directory` and the marker's
`CreateFileW` — install fails loudly if the marker cannot be created, and nothing else deletes it — so
an hour clears that race by some seven orders of magnitude. What the branch is for is a root whose
*contents* were partly deleted: a failed `remove_all` can take the marker and then stop on a locked
file, and without this branch that root would never be collectible again.

That margin argument only holds if both stamps come from the same clock, which is the second thing
worth recording. The branch compares the candidate against **this process's own root's creation
stamp**, not against `GetSystemTimeAsFileTime()`. The wall clock belongs to the process; the creation
stamp belongs to whichever filesystem hosts the temp root. Point `TEMP` at a network share whose
server clock lags the client by more than the grace period — an ordinary enterprise configuration —
and every freshly created root reads as aged out against the local clock, turning the microsecond
race above into a live deletion. Two stamps from one clock have no skew to exploit, and the reference
is always slightly behind the true present, which only makes the answer more conservative.

The accepted cost is that a volume recording no creation times at all disables the branch entirely,
since the reference reads as zero along with the candidates, and a marker-less root there is never
collected. "Cannot tell" has to mean "leave it alone": the worst outcome of that answer is litter,
while the opposite answer could delete a live root. The case is narrow — it needs a filesystem with no
creation timestamps *and* a root that lost its marker, while the ordinary crashed-run leftover still
has its marker and is collected by the primary rule.

Scope and failure handling follow from the same caution. `FindFirstFileEx` is given the
`libbsa-tests-*` pattern, so the filesystem does the filtering and no arbitrary temp entry ever reaches
the loop, let alone `remove_all`; reparse points are skipped so a junction cannot redirect a delete out
of the temp root. Failures are counted and skipped, never propagated — a leftover another user owns, or
one holding a file open, is litter, and failing the run over it would make the suite depend on the
machine's history rather than on the code.

The sweep's counts are exposed through `libbsa::tests::startup_sweep_report()` because its decisions are
otherwise unobservable from outside the process that made them. That is not a convenience: a live root
survives a sweep that *tried* to delete it, since the owner's marker handle also blocks `remove_all`, so
"the directory is still there" would pass against a sweep with no liveness rule at all. Only the counts
distinguish considered-and-spared from attempted-and-refused, and the out-of-process test asserts on
them.
