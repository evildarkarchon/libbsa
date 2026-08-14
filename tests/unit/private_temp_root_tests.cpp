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

#include "support/private_temp_root.hpp"

#include <libbsa/libbsa.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

#include <nlohmann/json.hpp>

namespace {

constexpr std::string_view snapshot_directory_prefix = "libbsa-dx10-snapshot-";

/// Set by the out-of-process teardown check on the child it spawns. The child
/// writes its own private root to the named file so the parent can assert the
/// directory is gone once the child has exited.
constexpr const wchar_t* private_root_report_variable = L"LIBBSA_TEST_PRIVATE_ROOT_REPORT";

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

/// Reads a variable from the Win32 process environment block. The report
/// variable is handed to the child through that block, and the CRT's own copy of
/// a variable set by the parent after start-up is not guaranteed to carry it.
std::optional<std::wstring> win32_environment_value(const wchar_t* name) {
    const auto required = ::GetEnvironmentVariableW(name, nullptr, 0);
    if (required == 0) {
        return std::nullopt;
    }

    std::wstring value(required, L'\0');
    const auto written = ::GetEnvironmentVariableW(name, value.data(), required);
    REQUIRE(written < required);
    value.resize(written);
    return value;
}

/// Closes a Win32 handle on destruction so a failing REQUIRE cannot leak one.
class scoped_handle {
   public:
    explicit scoped_handle(HANDLE handle) noexcept : handle_{handle} {}

    scoped_handle(const scoped_handle&) = delete;
    scoped_handle& operator=(const scoped_handle&) = delete;
    scoped_handle(scoped_handle&&) = delete;
    scoped_handle& operator=(scoped_handle&&) = delete;

    ~scoped_handle() {
        if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(handle_);
        }
    }

    HANDLE get() const noexcept { return handle_; }
    bool valid() const noexcept { return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE; }

   private:
    HANDLE handle_;
};

/// Sets the report variable in the Win32 environment block for as long as it is
/// alive, so a spawned child inherits it and no later test case sees it.
class scoped_report_variable {
   public:
    explicit scoped_report_variable(const std::filesystem::path& report_path) {
        REQUIRE(::SetEnvironmentVariableW(private_root_report_variable,
                                          report_path.native().c_str()) != FALSE);
    }

    scoped_report_variable(const scoped_report_variable&) = delete;
    scoped_report_variable& operator=(const scoped_report_variable&) = delete;
    scoped_report_variable(scoped_report_variable&&) = delete;
    scoped_report_variable& operator=(scoped_report_variable&&) = delete;

    // A null value deletes the variable rather than leaving an empty string that
    // the reporting case would treat as a path.
    ~scoped_report_variable() { ::SetEnvironmentVariableW(private_root_report_variable, nullptr); }
};

std::filesystem::path own_executable_path() {
    // Deliberately larger than MAX_PATH: a build tree can sit deeper than that,
    // and GetModuleFileNameW signals truncation only by filling the buffer.
    std::wstring buffer(4096, L'\0');
    const auto written =
        ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    REQUIRE(written > 0);
    REQUIRE(static_cast<std::size_t>(written) < buffer.size());
    buffer.resize(written);
    return std::filesystem::path{buffer};
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream stream{path, std::ios::binary};
    REQUIRE(stream.is_open());
    std::ostringstream text;
    text << stream.rdbuf();
    return text.str();
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

    // Normally there is nothing to report and this case is just another assertion
    // that the root exists. The out-of-process teardown check below re-runs this
    // binary filtered to this case with the report variable set, and the file it
    // writes here is what that check looks for after this process has exited.
    const auto report_path = win32_environment_value(private_root_report_variable);
    if (!report_path) {
        return;
    }

    std::ofstream report{std::filesystem::path{*report_path}, std::ios::binary | std::ios::trunc};
    REQUIRE(report.is_open());
    const auto utf8 = private_root.u8string();
    report.write(reinterpret_cast<const char*>(utf8.data()),
                 static_cast<std::streamsize>(utf8.size()));
    REQUIRE(report.good());
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
        const scoped_report_variable report_variable{report_path};

        // The child's output is captured to a file rather than inherited so a
        // passing run stays quiet and a failing one still has the child's Catch2
        // report to explain itself.
        SECURITY_ATTRIBUTES inheritable{};
        inheritable.nLength = sizeof(inheritable);
        inheritable.bInheritHandle = TRUE;
        const scoped_handle child_log{::CreateFileW(
            child_log_path.native().c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
            &inheritable, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)};
        REQUIRE(child_log.valid());

        // CreateProcessW may write to its command-line argument, so it cannot be a
        // string literal or a const buffer.
        std::wstring command_line =
            L"\"" + own_executable_path().native() + L"\" \"[private_temp_root_report]\"";

        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
        startup.hStdOutput = child_log.get();
        startup.hStdError = child_log.get();

        PROCESS_INFORMATION process{};
        const auto started = ::CreateProcessW(nullptr, command_line.data(), nullptr, nullptr, TRUE,
                                              0, nullptr, nullptr, &startup, &process);
        REQUIRE(started != FALSE);

        const scoped_handle child_process{process.hProcess};
        const scoped_handle child_thread{process.hThread};

        // Generous, because the ASan lane is slow to start a process. A hang here
        // should fail the test rather than wedge the suite.
        REQUIRE(::WaitForSingleObject(child_process.get(), 120000) == WAIT_OBJECT_0);

        DWORD exit_code = 0;
        REQUIRE(::GetExitCodeProcess(child_process.get(), &exit_code) != FALSE);
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
