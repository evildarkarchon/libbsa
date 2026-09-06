# ADR-0004: The test binary refuses a second concurrent instance by default

- Status: Accepted
- Date: 2026-08-14

## Context

ADR-0003 gave every `libbsa_tests` process a private temp root, so two concurrent runs no longer
destroy each other's BA2 DX10 snapshot directories. That closes the defect in issue #60. It does not
close the thing that made the defect expensive.

The failure concurrency produced was a *moving* set of DX10 cleanup failures: a different pair each
run, every one of them passing on `--rerun-failed`, with all three lanes green when run sequentially.
That signature is indistinguishable from a real regression in whatever code was just touched, and
issue #60 records an afternoon spent diagnosing exactly that on an unrelated TES3 change. Isolation
removes the corruption but leaves nothing that *names* the situation, and there is a second situation
isolation does not address at all: a stray test process left running from an earlier session, holding
a temp root and contributing nothing but confusion.

## Decision

Every `libbsa_tests` process takes a machine-local named mutex at start-up. A second concurrent
instance exits non-zero with a diagnostic that names the condition and names the opt-out, without
running any test body.

**The guard buys legibility, not safety.** The private temp root is what makes concurrency safe.
Nothing in the suite depends on the guard being present, and it is deliberately possible to turn off:
setting `LIBBSA_TEST_ALLOW_CONCURRENT` to any non-empty value skips the guard entirely. Issue #66's
two-instance concurrency proof runs that way, and so should anyone who knowingly wants two runs.

### A named mutex rather than a lock file

The criterion that decides the mechanism is that a lock left behind by a crashed process must be
recovered rather than block every later run forever. A kernel object gets that for free, twice over:

- When the last handle to a named mutex closes, the object is destroyed. A process that was killed
  leaves *nothing* behind, and the next instance creates the object fresh.
- When a process dies holding the mutex while some other handle keeps the object alive, the next
  waiter is told so with `WAIT_ABANDONED` and is given ownership anyway.

Windows destroys a process's entire handle table when the process terminates, whatever terminated
it, so neither path can get stuck — the same reason ADR-0003's stale-root sweep keys liveness to an
open handle rather than to an age. A lock *file* would need that ADR's exclusive-open dance plus its
own age-based escape hatch for a file that outlived its owner. There is no reason to repeat that
machinery when the kernel already maintains the bit.

Ownership is taken with a zero-timeout wait rather than with `CreateMutexW(..., bInitialOwner =
TRUE)`. `ERROR_ALREADY_EXISTS` only reports that the object existed; the wait distinguishes
held-by-a-live-owner from abandoned-by-a-dead-one, which is the distinction the recovery criterion is
about, and which a test asserts on directly.

### `Local\` rather than `Global\`

Creating an object in the global kernel namespace requires `SeCreateGlobalPrivilege`, which a standard
user account does not hold. A `Global\` name would therefore fail outright for exactly the developer
this guard exists for. The residual gap is two logon sessions of the *same* user — an RDP session
alongside the console — which share a temp root but not a `Local\` namespace and so do not contend.
That is accepted: it is rare, and the private temp root already makes it safe.

### It shares ADR-0003's listener rather than registering its own

The guard runs from the same `testRunStarting` hook that installs the private temp root, first.
Registering a second `CATCH_REGISTER_LISTENER` would leave the relative order to
static-initialisation order across translation units, which is unspecified; a refused instance would
then sometimes create and sweep a temp root on its way to being told it should not have started. Both
orders are correct — the root's teardown backstop cleans up either way — but only one is predictable.

## Consequences

- The lock is held for the duration of a **single test case**, not a whole suite run, because
  `catch_discover_tests` registers one CTest test per `TEST_CASE`. A serial `ctest` therefore never
  contends, since CTest starts the next process only after the previous one exits, while two
  overlapping `ctest` runs contend within the first case or two. Cost is one `CreateMutexW` plus one
  already-signalled wait per case.
- **`ctest -j` and `CTEST_PARALLEL_LEVEL` now fail loudly rather than flakily.** Every test case is
  its own process, so a parallel run puts several instances in contention and all but one is refused.
  That is not a new restriction — ADR-0003 already records intra-run parallelism as out of scope,
  because the fixed temp-derived names shared across many test translation units still collide inside
  one run, and a per-process root does nothing for two tests racing on
  `libbsa-ba2-dx10-partial-overlap.ba2`. What changes is the failure's shape: a named refusal instead
  of a scattered set of temp-path races. Setting the opt-out makes a parallel run start, but it does
  not make it correct, and that work still needs its own ticket. No preset sets `jobs`.
- **CTest's test discovery is unaffected.** `CatchAddTests.cmake` invokes the binary with
  `--list-tests --reporter json`, and listing test cases is not a test run, so Catch2 fires no
  `testRunStarting` and the guard never executes. That is a property of Catch2 rather than of this
  repository, so it is asserted by a test rather than assumed.
- A refused instance exits **1**: the guard throws out of `testRunStarting`, and Catch2's session
  catches it, prints `what()`, and returns non-zero before reaching any test case. The message is
  therefore *not* also written to stderr by the guard, which would print the whole paragraph twice —
  a poor result for a change whose entire purpose is a legible failure. A test pins that Catch2 still
  prints it.
- Any test that spawns a second copy of the binary has to opt the child out, because the parent is
  itself a running instance. `tests/support/child_test_process.hpp` does this, and it sets or
  *deletes* the variable explicitly rather than letting the child inherit the parent's environment —
  otherwise a developer with the opt-out set in their shell would turn every "this child must be
  refused" assertion into a false pass.
- A machine where the mutex cannot be created at all gets a warning on stderr and the run proceeds.
  Blocking the suite would trade a legibility aid for an outage, and the isolation that actually makes
  concurrency safe is unaffected.
- No process identity is added to the production snapshot directory name. Ownership stays a test-side
  question, exactly as ADR-0003 decided.

## What this does not do

It does not make concurrency safe — ADR-0003 did that — and it does not prove the concurrency
guarantee. That needs a test that actually runs two instances, which is the `concurrent_test_instances`
CTest case from issue #66, driven by `tests/concurrency/concurrent-test-instances.cmake`. That case
uses this guard's opt-out to start its two instances, and then covers the guard itself by holding the
named mutex above and requiring an instance started *without* the opt-out to be refused.
