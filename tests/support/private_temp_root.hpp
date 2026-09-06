#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>

/// Access to the private temp root that every libbsa_tests process owns.
///
/// A Catch2 event listener (`tests/support/private_temp_root.cpp`) creates a
/// uniquely named directory under the real system temp root before any test body
/// runs, points the process's TMP and TEMP at it, and removes it when the process
/// exits. Both test code and the libbsa library under test therefore resolve
/// `std::filesystem::temp_directory_path()` to that private root.
///
/// This is what makes the BA2 DX10 snapshot cleanup helpers' before/after
/// directory diff a correct ownership proxy: a process that cannot see another
/// process's snapshot directories cannot misattribute or delete one (issue #60).
namespace libbsa::tests {

/// The real system temp root, captured before this process repointed its own
/// temp environment.
///
/// Once the listener has run, `std::filesystem::temp_directory_path()` no longer
/// names this directory, so a test that needs to talk about the shared root --
/// for example to assert something did *not* land there -- must ask for it here.
///
/// The path carries no trailing separator: it is the canonical form of the
/// directory, not the `GetTempPath2W` string, which always appends a backslash.
///
/// @return The system temp root, or an empty path if the listener has not run.
const std::filesystem::path& system_temp_root();

/// The private temp root this process owns.
///
/// Lives directly under `system_temp_root()`, is created before the first test
/// body runs, and is removed when the process exits. Test code may create state
/// under it freely; anything left behind goes away with the root.
///
/// The path carries no trailing separator, so comparing it against a
/// `std::filesystem::temp_directory_path()` result requires canonicalising both
/// sides rather than comparing lexically.
///
/// @return The private temp root, or an empty path if the listener has not run.
const std::filesystem::path& private_temp_root();

/// The name prefix every private temp root carries.
///
/// This is the *only* thing that makes a directory a sweep candidate, so the
/// constant is shared rather than restated: a sweep that recognised one spelling
/// and a test that forged another would prove nothing about each other.
inline constexpr std::wstring_view private_root_name_prefix = L"libbsa-tests-";

/// The name of the file each private root's owner holds open for the root's
/// lifetime, and the only evidence the sweep accepts that an owner is still alive.
inline constexpr std::wstring_view private_root_owner_marker_name = L".libbsa-tests-owner";

/// How old a candidate root carrying *no* owner marker must be before the sweep
/// will remove it, in FILETIME ticks of 100ns. One hour.
///
/// Gates the secondary branch only. A root that has a marker is decided by the
/// marker and never by age, which is why this value can be generous without
/// weakening anything; see the sweep's implementation for the full rule.
///
/// Shared rather than restated in tests for the same reason the name constants
/// are: a test that backdated against its own idea of the grace period would
/// prove nothing about the sweep's.
inline constexpr std::uint64_t private_root_unclaimed_grace_ticks = 36'000'000'000ULL;

/// What one sweep of stale private temp roots did.
///
/// Every field counts directories whose name carried `private_root_name_prefix`;
/// nothing else is ever looked at. `examined` is the total, and the remaining
/// fields partition it.
struct stale_root_sweep_report {
    /// Candidate roots considered, excluding this process's own root.
    std::size_t examined{0};
    /// Roots whose owner was gone and which were removed cleanly. A removal that
    /// reported any error counts under `remove_failed` instead, even if it did
    /// delete part of the tree.
    std::size_t removed{0};
    /// Roots skipped because the marker could not be opened exclusively. Usually
    /// a live owner; an antivirus scanner or the search indexer holding the file
    /// produces the same refusal. The two are not distinguished because the
    /// decision is the same either way, and a genuinely dead root is collected by
    /// the next sweep.
    std::size_t owner_live{0};
    /// Roots skipped because they carry no marker but are too young for the
    /// grace period to have expired.
    std::size_t unclaimed_recent{0};
    /// Roots skipped because the marker could not be probed at all — a
    /// permission-denied leftover from another user, most likely.
    std::size_t skipped{0};
    /// Roots whose removal was attempted and failed, usually because something
    /// inside them is still locked. Counted, never fatal.
    std::size_t remove_failed{0};
};

/// Removes private temp roots directly under `search_root` that no live process
/// owns.
///
/// Called once at process start-up against the real system temp root; exposed
/// here so a test can drive it against a sandbox directory instead of against a
/// directory shared with the rest of the machine.
///
/// Never throws and never fails the caller. A root it cannot probe or cannot
/// delete is counted and skipped, because a locked leftover is litter and turning
/// it into a test-run failure would make the suite depend on the machine's
/// history. The rule that keeps it from deleting a *live* root is documented on
/// the implementation in `tests/support/private_temp_root.cpp`.
///
/// @param search_root Directory to scan. An empty or unreadable path yields an
///        all-zero report.
/// @return Counts of what happened, for tests and for diagnosis.
stale_root_sweep_report sweep_stale_private_temp_roots(
    const std::filesystem::path& search_root) noexcept;

/// What the sweep this process ran at start-up did.
///
/// Exposed because the sweep's decisions are otherwise unobservable from outside
/// the process that made them. A test that wants to prove another process's
/// start-up sweep *considered* a live root and *chose* to spare it needs the
/// counts; without them, a root that survived because something else blocked the
/// delete is indistinguishable from one the sweep deliberately skipped.
///
/// @return The start-up sweep's report, or an all-zero report if the listener has
///         not run.
const stale_root_sweep_report& startup_sweep_report();

}  // namespace libbsa::tests
