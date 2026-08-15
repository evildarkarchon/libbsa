// Proves that each libbsa_tests process owns a private temp root, and that the
// libbsa library under test resolves its own temp paths inside it.
//
// The second property is the load-bearing one. Issue #60 isolates test processes
// by repointing TMP and TEMP rather than by threading a path helper through the
// library, precisely so that production code -- which resolves
// std::filesystem::temp_directory_path() at a call site no test can reach --
// is covered too. Without an assertion on a snapshot directory created by a real
// BA2 DX10 write, a future STL or toolchain change could silently revert the
// library to the shared system temp root and nothing here would notice.
//
// The mechanism itself, and the constraints it imposes, are pinned separately by
// tests/unit/temp_directory_redirection_tests.cpp and documented on
// tests/support/private_temp_root.cpp.

#include <catch2/catch_test_macros.hpp>

#include "support/child_test_process.hpp"
#include "support/private_temp_root.hpp"

#include <libbsa/libbsa.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <system_error>

#include <nlohmann/json.hpp>

namespace {

constexpr std::string_view snapshot_directory_prefix = "libbsa-dx10-snapshot-";

// Spawning a second copy of this binary, and the parent/child reporting protocol
// it uses, are shared with the single-instance guard tests; see
// tests/support/child_test_process.hpp. Pulled in by name rather than qualified at
// every use, because these read as local vocabulary in the bodies below.
using libbsa::tests::private_root_report_variable;
using libbsa::tests::read_text_file;
using libbsa::tests::run_child_test_binary;
using libbsa::tests::scoped_environment_variable;
using libbsa::tests::scoped_handle;
using libbsa::tests::startup_sweep_report_variable;
using libbsa::tests::win32_environment_value;
using libbsa::tests::write_text_file;

std::filesystem::path generated_source_dir() {
    return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "source";
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
    std::ifstream stream{path};
    REQUIRE(stream.is_open());
    return nlohmann::json::parse(stream);
}

std::string utf8_string_from_path(const std::filesystem::path& path) {
    // Public writer APIs take UTF-8 host text, and the private temp root sits
    // under the user's temp directory, whose name can carry non-ASCII characters.
    // A narrow ACP conversion would corrupt those.
    const auto utf8 = path.u8string();
    return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
}

/// Resolves `path` to its canonical form. Canonicalisation failure is a hard test
/// failure rather than a sentinel return: returning a distinct path would let a
/// negative assertion pass for the wrong reason.
std::filesystem::path canonical_path(const std::filesystem::path& path) {
    std::error_code fs_error;
    auto canonical = std::filesystem::canonical(path, fs_error);
    INFO("canonicalising: " << path.string());
    REQUIRE_FALSE(fs_error);
    return canonical;
}

/// Lists BA2 DX10 snapshot directories directly under `root`, without querying
/// metadata for unrelated temp entries.
std::set<std::filesystem::path> snapshot_directories_under(const std::filesystem::path& root) {
    std::set<std::filesystem::path> paths;
    std::error_code fs_error;
    for (const auto& entry : std::filesystem::directory_iterator{root, fs_error}) {
        const auto name = entry.path().filename().string();
        if (name.rfind(snapshot_directory_prefix, 0U) != 0U) {
            continue;
        }

        std::error_code entry_error;
        if (entry.is_directory(entry_error)) {
            paths.insert(entry.path());
        }
    }
    REQUIRE_FALSE(fs_error);
    return paths;
}

/// Creates a directory the sweep cannot tell apart from a real private temp root.
///
/// Only the name prefix makes a directory a candidate, so the suffix is free; it
/// is spelled out per call so a failure names which forgery it was about.
std::filesystem::path make_forged_root(const std::filesystem::path& parent,
                                       std::wstring_view suffix) {
    auto root = parent / (std::wstring{libbsa::tests::private_root_name_prefix} + L"forged-" +
                          std::wstring{suffix});
    std::error_code fs_error;
    std::filesystem::remove_all(root, fs_error);
    REQUIRE(std::filesystem::create_directories(root));
    return root;
}

/// Claims a forged root exactly the way a real owner does: an exclusive,
/// share-nothing handle on the owner marker, held until the caller drops it.
///
/// Sharing nothing is the entire mechanism. While this handle is open no other
/// process can open the marker, which is what a sweeping process reads as "the
/// owner is alive"; closing it makes the same root collectable, which is what
/// happens for real when a process dies and Windows tears down its handle table.
scoped_handle claim_forged_root(const std::filesystem::path& root) {
    const auto marker = root / std::wstring{libbsa::tests::private_root_owner_marker_name};
    return scoped_handle{::CreateFileW(marker.native().c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                                       nullptr, CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN, nullptr)};
}

std::uint64_t filetime_ticks(const FILETIME& value) noexcept {
    return (static_cast<std::uint64_t>(value.dwHighDateTime) << 32) |
           static_cast<std::uint64_t>(value.dwLowDateTime);
}

std::uint64_t creation_time_ticks(const std::filesystem::path& path) {
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    REQUIRE(::GetFileAttributesExW(path.native().c_str(), GetFileExInfoStandard, &attributes) !=
            FALSE);
    return filetime_ticks(attributes.ftCreationTime);
}

/// Rewinds a directory's creation time by `ticks_back` 100ns ticks.
///
/// The sweep's grace period for a root carrying no owner marker is an hour, which
/// a test cannot wait out, so the clock is moved instead of the test. Opening a
/// directory with CreateFileW requires FILE_FLAG_BACKUP_SEMANTICS.
///
/// The rewind is measured from the private root's creation stamp rather than from
/// the wall clock, because that is the reference the sweep itself compares
/// against. Backdating against a different clock would make this test pass or fail
/// on skew rather than on the grace period.
void backdate_creation_time(const std::filesystem::path& directory, std::uint64_t ticks_back) {
    const auto reference_ticks = creation_time_ticks(libbsa::tests::private_temp_root());
    REQUIRE(reference_ticks > ticks_back);
    const auto backdated_ticks = reference_ticks - ticks_back;

    {
        const scoped_handle handle{::CreateFileW(
            directory.native().c_str(), FILE_WRITE_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS, nullptr)};
        REQUIRE(handle.valid());

        FILETIME backdated{};
        backdated.dwLowDateTime = static_cast<DWORD>(backdated_ticks & 0xFFFFFFFFULL);
        backdated.dwHighDateTime = static_cast<DWORD>(backdated_ticks >> 32);
        REQUIRE(::SetFileTime(handle.get(), &backdated, nullptr, nullptr) != FALSE);
    }

    // Read back after the handle has closed. NTFS writes an explicitly set
    // timestamp through to the parent's index entry on close, and the sweep reads
    // that entry through FindFirstFileEx; asserting here means a platform that
    // did not do so fails as a broken setup rather than as a sweep that ignored
    // the grace period.
    REQUIRE(creation_time_ticks(directory) == backdated_ticks);
}

/// A directory under the private root for one test case's forged roots.
///
/// Pointing the sweep at a sandbox rather than at the system temp root is what
/// keeps these tests from being an instance of the very hazard they cover: a
/// forged root planted in a directory shared with the machine would be visible
/// to, and destroyable by, every other process on it.
std::filesystem::path make_sweep_sandbox(std::wstring_view name) {
    const auto& private_root = libbsa::tests::private_temp_root();
    REQUIRE_FALSE(private_root.empty());
    auto sandbox = private_root / (std::wstring{L"sweep-sandbox-"} + std::wstring{name});
    std::error_code fs_error;
    std::filesystem::remove_all(sandbox, fs_error);
    REQUIRE(std::filesystem::create_directories(sandbox));
    return sandbox;
}

}  // namespace

TEST_CASE("test process resolves its temp directory to a private root",
          "[unit][private_temp_root]") {
    const auto& private_root = libbsa::tests::private_temp_root();
    const auto& system_root = libbsa::tests::system_temp_root();

    // An empty path means the Catch2 listener never installed the root, which
    // would leave every temp-using test back on the shared system root.
    REQUIRE_FALSE(private_root.empty());
    REQUIRE_FALSE(system_root.empty());
    REQUIRE(std::filesystem::is_directory(private_root));

    CHECK(canonical_path(private_root) != canonical_path(system_root));
    CHECK(canonical_path(private_root.parent_path()) == canonical_path(system_root));

    // The point of repointing the environment rather than adding a path helper:
    // anything that asks the platform for a temp directory, in test code or in
    // libbsa, gets the private root.
    CHECK(canonical_path(std::filesystem::temp_directory_path()) == canonical_path(private_root));
}

TEST_CASE("private temp root reports its location when asked to",
          "[unit][private_temp_root][private_temp_root_report]") {
    const auto& private_root = libbsa::tests::private_temp_root();
    REQUIRE_FALSE(private_root.empty());
    REQUIRE(std::filesystem::is_directory(private_root));

    // Normally neither variable is set and this case is just another assertion
    // that the root exists. The out-of-process checks below re-run this binary
    // filtered to this case with one or both variables set, and the files written
    // here are what those checks read once this process has exited or, for the
    // sweep counts, as soon as it has.
    if (const auto report_path = win32_environment_value(private_root_report_variable)) {
        std::ofstream report{std::filesystem::path{*report_path},
                             std::ios::binary | std::ios::trunc};
        REQUIRE(report.is_open());
        const auto utf8 = private_root.u8string();
        report.write(reinterpret_cast<const char*>(utf8.data()),
                     static_cast<std::streamsize>(utf8.size()));
        REQUIRE(report.good());
    }

    if (const auto sweep_path = win32_environment_value(startup_sweep_report_variable)) {
        // The sweep already ran, in the listener, before this test body started.
        const auto& sweep = libbsa::tests::startup_sweep_report();
        const nlohmann::json counts{{"examined", sweep.examined},
                                    {"removed", sweep.removed},
                                    {"owner_live", sweep.owner_live},
                                    {"unclaimed_recent", sweep.unclaimed_recent},
                                    {"skipped", sweep.skipped},
                                    {"remove_failed", sweep.remove_failed}};
        std::ofstream report{std::filesystem::path{*sweep_path}, std::ios::binary | std::ios::trunc};
        REQUIRE(report.is_open());
        report << counts.dump();
        REQUIRE(report.good());
    }
}

TEST_CASE("the private temp root is removed when the process exits", "[unit][private_temp_root]") {
    // Teardown cannot be observed from inside the process it tears down, so this
    // runs a second copy of the test binary, has it report the root it created,
    // and checks that root after it has exited. Without this the removal path is
    // implemented but unproven, and a regression would leak one directory per test
    // case -- roughly five hundred per lane -- in complete silence.
    //
    // This is not the concurrency proof issue #60 also asks for: the child runs to
    // completion before anything is asserted, and nothing here runs two instances
    // at once.
    const auto& private_root = libbsa::tests::private_temp_root();
    REQUIRE_FALSE(private_root.empty());

    const auto report_path = private_root / "child-private-root.txt";
    const auto child_log_path = private_root / "child-output.txt";

    std::error_code fs_error;
    std::filesystem::remove(report_path, fs_error);

    {
        const scoped_environment_variable report_variable{private_root_report_variable,
                                                          report_path};

        const auto exit_code = run_child_test_binary(L"[private_temp_root_report]", child_log_path);
        INFO("child output: " << read_text_file(child_log_path));
        // A zero exit proves the child's own REQUIRE that its private root existed
        // while it was running, which is the half of the property this process
        // cannot see for itself.
        REQUIRE(exit_code == 0U);
    }

    REQUIRE(std::filesystem::exists(report_path));
    const std::filesystem::path child_root{
        std::u8string{reinterpret_cast<const char8_t*>(read_text_file(report_path).c_str())}};
    REQUIRE_FALSE(child_root.empty());

    // Guards against the report being some unrelated path: the child's root is
    // created under the temp root it inherited, which is this process's own.
    INFO("child private temp root: " << child_root.string());
    CHECK(child_root.filename().string().rfind("libbsa-tests-", 0U) == 0U);
    CHECK(canonical_path(child_root.parent_path()) == canonical_path(private_root));

    CHECK_FALSE(std::filesystem::exists(child_root));

    std::filesystem::remove(report_path, fs_error);
    std::filesystem::remove(child_log_path, fs_error);
}

// The sweep cases below. Together they cover the two halves the spec asks for --
// a root left behind by a dead run is removed, a root a live process owns never
// is -- plus the three properties that make the sweep safe to run at all: it
// looks only at private roots, it treats a root it cannot classify as one to
// leave alone, and it never turns a failure into a test-run failure.
//
// All but the last drive the sweep against a sandbox under the private root
// rather than against the system temp root, which is what keeps a test about
// cross-process destruction from being an instance of it.

TEST_CASE("the stale-root sweep removes a private root whose owner is gone",
          "[unit][private_temp_root][private_temp_root_sweep]") {
    const auto sandbox = make_sweep_sandbox(L"dead-owner");

    const auto dead_root = make_forged_root(sandbox, L"dead");
    {
        // Claimed and then released, which is the state a crashed, killed, or
        // force-terminated run leaves behind: the marker file is still there, and
        // Windows closed the handle when the process died.
        const auto marker = claim_forged_root(dead_root);
        REQUIRE(marker.valid());
    }
    write_text_file(dead_root / "leftover.bin", "residue");

    const auto report = libbsa::tests::sweep_stale_private_temp_roots(sandbox);
    CHECK(report.examined == 1U);
    CHECK(report.removed == 1U);
    CHECK(report.owner_live == 0U);
    CHECK(report.remove_failed == 0U);
    CHECK_FALSE(std::filesystem::exists(dead_root));
}

TEST_CASE("the stale-root sweep never removes a root whose owner still holds it",
          "[unit][private_temp_root][private_temp_root_sweep]") {
    const auto sandbox = make_sweep_sandbox(L"live-owner");

    const auto live_root = make_forged_root(sandbox, L"live");
    auto marker = claim_forged_root(live_root);
    REQUIRE(marker.valid());
    write_text_file(live_root / "in-use.bin", "working");

    const auto held = libbsa::tests::sweep_stale_private_temp_roots(sandbox);
    CHECK(held.examined == 1U);
    CHECK(held.owner_live == 1U);
    CHECK(held.removed == 0U);
    CHECK(std::filesystem::is_directory(live_root));
    CHECK(std::filesystem::exists(live_root / "in-use.bin"));

    // Releasing the handle is the only thing that changed, and the same root is
    // now collectable. That is what pins the decision to the handle rather than to
    // the directory's name, its age, or anything else about it -- and it is the
    // reason a long ASan lane run cannot age out from under the sweep.
    marker.reset();

    const auto released = libbsa::tests::sweep_stale_private_temp_roots(sandbox);
    CHECK(released.examined == 1U);
    CHECK(released.removed == 1U);
    CHECK(released.owner_live == 0U);
    CHECK_FALSE(std::filesystem::exists(live_root));
}

TEST_CASE("the stale-root sweep only considers directories named like private roots",
          "[unit][private_temp_root][private_temp_root_sweep]") {
    const auto sandbox = make_sweep_sandbox(L"naming");

    const auto dead_root = make_forged_root(sandbox, L"sweepable");
    {
        const auto marker = claim_forged_root(dead_root);
        REQUIRE(marker.valid());
    }

    // A BA2 DX10 snapshot directory is the nearest neighbour in the temp root and
    // exactly what issue #60 saw destroyed, so it is the one to prove untouched.
    const auto snapshot_lookalike = sandbox / "libbsa-dx10-snapshot-forged";
    REQUIRE(std::filesystem::create_directory(snapshot_lookalike));
    write_text_file(snapshot_lookalike / "staged.dds", "someone else's work");

    const auto unrelated = sandbox / "unrelated-directory";
    REQUIRE(std::filesystem::create_directory(unrelated));

    // Carries the prefix but is not a directory, so it is not a root and must not
    // be deleted as one.
    const auto prefixed_file =
        sandbox / (std::wstring{libbsa::tests::private_root_name_prefix} + L"not-a-directory");
    write_text_file(prefixed_file, "file, not root");

    const auto report = libbsa::tests::sweep_stale_private_temp_roots(sandbox);
    CHECK(report.examined == 1U);
    CHECK(report.removed == 1U);

    CHECK_FALSE(std::filesystem::exists(dead_root));
    CHECK(std::filesystem::is_directory(snapshot_lookalike));
    CHECK(std::filesystem::exists(snapshot_lookalike / "staged.dds"));
    CHECK(std::filesystem::is_directory(unrelated));
    CHECK(std::filesystem::is_regular_file(prefixed_file));
}

TEST_CASE("the stale-root sweep leaves an unclaimed root alone until it has aged out",
          "[unit][private_temp_root][private_temp_root_sweep]") {
    const auto sandbox = make_sweep_sandbox(L"unclaimed");

    // No owner marker at all. A live owner is only ever in this state for the few
    // microseconds between creating its root and claiming it, so the grace period
    // is what separates that race from a root whose marker a partly-failed
    // remove_all took with it.
    const auto unclaimed_root = make_forged_root(sandbox, L"unclaimed");
    write_text_file(unclaimed_root / "residue.bin", "residue");

    const auto fresh = libbsa::tests::sweep_stale_private_temp_roots(sandbox);
    CHECK(fresh.examined == 1U);
    CHECK(fresh.unclaimed_recent == 1U);
    CHECK(fresh.removed == 0U);
    CHECK(std::filesystem::is_directory(unclaimed_root));

    backdate_creation_time(unclaimed_root, 2U * libbsa::tests::private_root_unclaimed_grace_ticks);

    const auto aged = libbsa::tests::sweep_stale_private_temp_roots(sandbox);
    CHECK(aged.examined == 1U);
    CHECK(aged.unclaimed_recent == 0U);
    CHECK(aged.removed == 1U);
    CHECK_FALSE(std::filesystem::exists(unclaimed_root));

    // The grace period is shared with the sweep so this test cannot backdate
    // against a different number than the sweep compares against, which leaves the
    // *magnitude* unpinned. Pin it here: the argument that an hour dwarfs the
    // microsecond window it has to clear stops holding if someone quietly reduces
    // it to seconds, and a shared constant alone would not notice.
    static_assert(libbsa::tests::private_root_unclaimed_grace_ticks == 36'000'000'000ULL,
                  "the unclaimed-root grace period is meant to be one hour");
}

TEST_CASE("the stale-root sweep skips a root whose marker it cannot probe",
          "[unit][private_temp_root][private_temp_root_sweep]") {
    const auto sandbox = make_sweep_sandbox(L"unprobeable");

    // On a real machine this branch fires on a leftover owned by another user,
    // where opening the marker returns ERROR_ACCESS_DENIED. That is awkward to
    // stage without a second account, so the same error is produced the cheap way:
    // CreateFileW with OPEN_EXISTING refuses a directory unless it is passed
    // FILE_FLAG_BACKUP_SEMANTICS, which the sweep deliberately does not pass.
    const auto unprobeable_root = make_forged_root(sandbox, L"unprobeable");
    REQUIRE(std::filesystem::create_directory(
        unprobeable_root / std::wstring{libbsa::tests::private_root_owner_marker_name}));

    const auto report = libbsa::tests::sweep_stale_private_temp_roots(sandbox);
    CHECK(report.examined == 1U);
    CHECK(report.skipped == 1U);
    CHECK(report.removed == 0U);
    CHECK(report.owner_live == 0U);
    CHECK(report.unclaimed_recent == 0U);

    // A marker it cannot classify is not evidence of a dead owner, so the root
    // stays. Skipped, not failed: the sweep returned a report rather than raising.
    CHECK(std::filesystem::is_directory(unprobeable_root));
}

TEST_CASE("the stale-root sweep reports a leftover it cannot delete instead of failing",
          "[unit][private_temp_root][private_temp_root_sweep]") {
    const auto sandbox = make_sweep_sandbox(L"locked");

    const auto dead_root = make_forged_root(sandbox, L"locked");
    {
        const auto marker = claim_forged_root(dead_root);
        REQUIRE(marker.valid());
    }

    // Share nothing, so nothing can delete this file while the handle is open --
    // the shape of a leftover another process still has a grip on.
    const auto locked_path = dead_root / "locked.bin";
    scoped_handle locked{::CreateFileW(locked_path.native().c_str(), GENERIC_WRITE, 0, nullptr,
                                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)};
    REQUIRE(locked.valid());

    // Returns normally rather than throwing or aborting: the whole point is that a
    // leftover the machine happens to be holding is litter, not a test failure.
    const auto report = libbsa::tests::sweep_stale_private_temp_roots(sandbox);
    CHECK(report.examined == 1U);
    CHECK(report.removed == 0U);
    CHECK(report.remove_failed == 1U);
    CHECK(std::filesystem::exists(locked_path));

    // What survives the failed removal is deliberately not asserted: remove_all
    // does not specify the order it deletes in, so whether the owner marker is
    // still there afterwards is not a property to pin. The grace-period branch
    // above is what eventually collects a root left in that state.
    locked.reset();
    std::error_code fs_error;
    std::filesystem::remove_all(dead_root, fs_error);
}

TEST_CASE("a second process's start-up sweep removes a dead root and spares a live one",
          "[unit][private_temp_root][private_temp_root_sweep]") {
    // The end-to-end proof, and the only one of these that exercises the sweep
    // where it actually runs: at another process's start-up, against a temp root
    // it did not create.
    //
    // The child inherits this process's TMP and TEMP, so the "system temp root" it
    // sweeps *is* this process's private root. That is what makes the test both
    // genuinely cross-process and free of litter in the real system temp root,
    // where a failing run would otherwise leave forged directories on the machine.
    const auto& private_root = libbsa::tests::private_temp_root();
    REQUIRE_FALSE(private_root.empty());

    const auto dead_root = make_forged_root(private_root, L"child-sweep-dead");
    {
        const auto marker = claim_forged_root(dead_root);
        REQUIRE(marker.valid());
    }

    const auto live_root = make_forged_root(private_root, L"child-sweep-live");
    auto live_marker = claim_forged_root(live_root);
    REQUIRE(live_marker.valid());
    write_text_file(live_root / "in-use.bin", "this process is still working");

    const auto child_log_path = private_root / "sweep-child-output.txt";
    const auto sweep_report_path = private_root / "child-sweep-report.json";

    DWORD exit_code = 0;
    {
        const scoped_environment_variable sweep_variable{startup_sweep_report_variable,
                                                         sweep_report_path};
        exit_code = run_child_test_binary(L"[private_temp_root_report]", child_log_path);
    }
    INFO("child output: " << read_text_file(child_log_path));
    REQUIRE(exit_code == 0U);

    CHECK_FALSE(std::filesystem::exists(dead_root));

    // The half that matters, and it has to be asserted on the sweep's decision
    // rather than on the directory's survival. The live root survives a sweep that
    // *tried* to delete it too, because this process's marker handle also blocks
    // remove_all -- so "the directory is still there" alone would pass against a
    // sweep with no liveness rule at all. The counts are what distinguish
    // "considered it and spared it" from "attempted it and was refused".
    REQUIRE(std::filesystem::exists(sweep_report_path));
    const auto sweep = nlohmann::json::parse(read_text_file(sweep_report_path));
    INFO("child start-up sweep: " << sweep.dump());
    CHECK(sweep.at("examined").get<std::size_t>() == 2U);
    CHECK(sweep.at("owner_live").get<std::size_t>() == 1U);
    CHECK(sweep.at("removed").get<std::size_t>() == 1U);
    CHECK(sweep.at("remove_failed").get<std::size_t>() == 0U);

    CHECK(std::filesystem::is_directory(live_root));
    CHECK(std::filesystem::exists(live_root / "in-use.bin"));
    CHECK(std::filesystem::exists(live_root /
                                  std::wstring{libbsa::tests::private_root_owner_marker_name}));

    live_marker.reset();
    std::error_code fs_error;
    std::filesystem::remove_all(live_root, fs_error);
    std::filesystem::remove(sweep_report_path, fs_error);
    std::filesystem::remove(child_log_path, fs_error);
}

TEST_CASE("BA2 DX10 snapshot directories land in the private temp root",
          "[unit][private_temp_root][ba2_dx10_writer]") {
    const auto& private_root = libbsa::tests::private_temp_root();
    const auto& system_root = libbsa::tests::system_temp_root();
    REQUIRE_FALSE(private_root.empty());
    REQUIRE_FALSE(system_root.empty());

    const auto before = snapshot_directories_under(private_root);

    const auto manifest =
        read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
    const auto& source_case = manifest.at("valid_cases").at(0);
    const auto source_path =
        utf8_string_from_path(generated_source_dir() / source_case.at("file").get<std::string>());

    // A real BA2 DX10 write, not a call into the snapshot seam: add_file is what
    // drives make_unique_snapshot_directory() through the public writer, so the
    // temp root being asserted on is the one production code resolved for itself.
    libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};
    REQUIRE(writer.add_file("textures/private-root-proof.dds", source_path).has_value());

    std::set<std::filesystem::path> created;
    for (const auto& path : snapshot_directories_under(private_root)) {
        if (!before.contains(path)) {
            created.insert(path);
        }
    }
    REQUIRE(created.size() == 1U);

    // This is the load-bearing assertion, and it is what catches the regression
    // the spec names: if a future STL or toolchain change stopped honouring the
    // repoint, the writer would create its snapshot directory in the shared system
    // root, `created` would be empty, and the REQUIRE above would already fail.
    const auto& snapshot_dir = *created.begin();
    INFO("BA2 DX10 snapshot directory: " << snapshot_dir.string());
    CHECK(canonical_path(snapshot_dir.parent_path()) == canonical_path(private_root));

    // The negative half of the criterion. It is deliberately a property of where
    // the directory landed rather than a scan of the system temp root for
    // directories that failed to appear there. Such a scan is the only
    // independently discriminating form, and it would false-fail whenever any
    // other process on the machine happened to be packing a DX10 archive inside
    // the window -- which is the cross-process flakiness this change exists to
    // remove, and reintroducing it here to prove its own absence would be absurd.
    //
    // What this form does catch is the private root silently collapsing back onto
    // the system root, which would satisfy every positive assertion above.
    CHECK(canonical_path(snapshot_dir.parent_path()) != canonical_path(system_root));

    // Finish the write so the writer's ordinary cleanup runs; a leaked snapshot
    // directory would otherwise be swept only by the private root's teardown, and
    // that would hide a cleanup regression rather than surface it.
    const auto output = private_root / "libbsa-private-temp-root-proof.ba2";
    std::error_code fs_error;
    std::filesystem::remove(output, fs_error);
    REQUIRE(writer.write_to(utf8_string_from_path(output)).has_value());
    CHECK_FALSE(std::filesystem::exists(snapshot_dir));

    std::filesystem::remove(output, fs_error);
}
