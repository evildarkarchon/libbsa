#pragma once

#include <filesystem>

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

}  // namespace libbsa::tests
