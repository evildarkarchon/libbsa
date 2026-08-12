#include <catch2/catch_test_macros.hpp>

#include "formats/ba2/ba2_gnrl_layout.hpp"
#include "formats/ba2/ba2_gnrl_prepare.hpp"
#include "formats/ba2/ba2_gnrl_serialize.hpp"
#include "formats/ba2/ba2_profile.hpp"

#include <detail/writer_publish.hpp>
#include <detail/parallel_work.hpp>

#include <libbsa/result.hpp>
#include <libbsa/writer.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {

struct path_only_publish_callback {
    libbsa::result<void> operator()(const std::filesystem::path&) const { return {}; }
};

template <typename Callback>
concept valid_writer_publish_callback =
    requires(Callback&& callback, const std::filesystem::path& path) {
        libbsa::detail::publish_writer_output(path, false, "test writer",
                                              std::forward<Callback>(callback));
    };

std::filesystem::path writer_publish_test_dir() {
    auto path = std::filesystem::temp_directory_path() / "libbsa_writer_publish_tests";
    std::filesystem::create_directories(path);
    return path;
}

std::filesystem::path output_path(std::string name) {
    static std::atomic_uint64_t counter{0};
    auto path =
        writer_publish_test_dir() / std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
    std::error_code fs_error;
    std::filesystem::remove_all(path, fs_error);
    std::filesystem::create_directories(path);
    return path / std::move(name);
}

std::vector<std::byte> bytes_from_text(std::string_view text) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

libbsa::formats::ba2::ba2_gnrl_writer_entry gnrl_disk_entry(
    std::string_view archive_path, const std::filesystem::path& source_path) {
    auto entry = libbsa::formats::ba2::ba2_gnrl_make_writer_entry(archive_path,
                                                                  libbsa::ba2_gnrl_entry_options{});
    REQUIRE(entry.has_value());
    entry.value().host_path = source_path.string();
    return std::move(entry).value();
}

void write_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    REQUIRE(output.good());
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    REQUIRE(input.good());

    std::vector<std::byte> bytes;
    for (char ch = 0; input.get(ch);) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

#if defined(_WIN32)
class read_only_file_guard {
   public:
    explicit read_only_file_guard(std::filesystem::path path) : path_(std::move(path)) {}

    void make_read_only() const {
        const auto attributes = GetFileAttributesW(path_.c_str());
        REQUIRE(attributes != INVALID_FILE_ATTRIBUTES);
        REQUIRE(SetFileAttributesW(path_.c_str(), attributes | FILE_ATTRIBUTE_READONLY) != 0);
    }

    ~read_only_file_guard() {
        const auto attributes = GetFileAttributesW(path_.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES) {
            // Cleanup must restore write access so the test-owned temp tree can be
            // removed later.
            SetFileAttributesW(path_.c_str(), attributes & ~FILE_ATTRIBUTE_READONLY);
        }
    }

   private:
    std::filesystem::path path_;
};

bool is_reparse_point(const std::filesystem::path& path) {
    const auto attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

bool try_create_file_symlink(const std::filesystem::path& link_path,
                             const std::filesystem::path& target_path) {
    std::error_code fs_error;
    std::filesystem::create_symlink(target_path, link_path, fs_error);
    if (!fs_error) {
        return true;
    }

    WARN(
        "Skipping reparse-point publish test because this host cannot create "
        "file symlinks: "
        << fs_error.message());
    return false;
}
#endif

}  // namespace

TEST_CASE(
    "writer_publish reserves isolated finalization workspaces and cleans them "
    "after success",
    "[unit][writer_publish][publish]") {
    const auto archive = output_path("reserve-success.bsa");
    const auto collision = archive.string() + ".tmp";
    const auto occupied_workspace = archive.string() + ".libbsa-tmp-0";
    const auto occupied_sentinel = std::filesystem::path{occupied_workspace} / "caller-owned.bin";
    const auto expected = bytes_from_text("new archive bytes");
    const auto sentinel = bytes_from_text("caller temp sibling");
    std::filesystem::path observed_temp_dir;
    write_binary_file(collision, sentinel);
    REQUIRE(std::filesystem::create_directory(occupied_workspace));
    write_binary_file(occupied_sentinel, sentinel);

    auto published = libbsa::detail::publish_writer_output(
        archive, false, "TES3 BSA writer",
        [&](const libbsa::detail::finalization_workspace& workspace) -> libbsa::result<void> {
            const auto& temp_path = workspace.temporary_archive_path();
            observed_temp_dir = temp_path.parent_path();
            write_binary_file(temp_path, expected);
            return {};
        });

    REQUIRE(published.has_value());
    CHECK(read_binary_file(archive) == expected);
    CHECK(read_binary_file(collision) == sentinel);
    CHECK(read_binary_file(occupied_sentinel) == sentinel);
    CHECK(observed_temp_dir != occupied_workspace);
    CHECK_FALSE(std::filesystem::exists(observed_temp_dir));
}

TEST_CASE(
    "writer_publish finalization workspace gives indexed workers deterministic "
    "collision-free snapshot paths",
    "[unit][writer_publish][publish][workspace]") {
    constexpr std::size_t snapshot_count = 16U;
    const auto archive = output_path("snapshot-0.bin");
    const auto expected = bytes_from_text("archive bytes");
    std::array<std::filesystem::path, snapshot_count> snapshot_paths;
    std::filesystem::path observed_workspace_dir;
    std::filesystem::path observed_archive_path;
    std::filesystem::path repeated_zero_path;

    auto published = libbsa::detail::publish_writer_output(
        archive, false, "TES4 BSA writer",
        [&](const libbsa::detail::finalization_workspace& workspace) -> libbsa::result<void> {
            observed_workspace_dir = workspace.temporary_archive_path().parent_path();
            observed_archive_path = workspace.temporary_archive_path();
            repeated_zero_path = workspace.snapshot_path(0U);
            auto reserved = libbsa::detail::run_indexed_work(
                snapshot_paths.size(), 4U, [&](std::size_t index) -> libbsa::result<void> {
                    snapshot_paths[index] = workspace.snapshot_path(index);
                    std::ofstream snapshot{snapshot_paths[index],
                                           std::ios::binary | std::ios::trunc};
                    if (!snapshot.good()) {
                        return libbsa::error{libbsa::error_code::io_error,
                                             "test failed to create workspace snapshot"};
                    }
                    snapshot.put(static_cast<char>(index));
                    if (!snapshot.good()) {
                        return libbsa::error{libbsa::error_code::io_error,
                                             "test failed to write workspace snapshot"};
                    }
                    return {};
                });
            if (!reserved) {
                return reserved.error();
            }

            write_binary_file(workspace.temporary_archive_path(), expected);
            return {};
        });

    REQUIRE(published.has_value());
    CHECK(read_binary_file(archive) == expected);
    std::set<std::filesystem::path> distinct_paths{snapshot_paths.begin(), snapshot_paths.end()};
    CHECK(distinct_paths.size() == snapshot_paths.size());
    for (std::size_t index = 0; index < snapshot_paths.size(); ++index) {
        CAPTURE(index);
        CHECK(snapshot_paths[index] ==
              observed_workspace_dir / ("snapshot-" + std::to_string(index) + ".bin"));
        CHECK(snapshot_paths[index].parent_path() == observed_workspace_dir);
        CHECK(snapshot_paths[index] != observed_archive_path);
    }
    CHECK(repeated_zero_path == snapshot_paths[0]);
    CHECK_FALSE(std::filesystem::exists(observed_workspace_dir));
}

TEST_CASE("Finalization Workspace is a move-only cleanup owner",
          "[unit][writer_publish][publish][workspace]") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<libbsa::detail::finalization_workspace>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<libbsa::detail::finalization_workspace>);
    STATIC_REQUIRE(std::is_move_constructible_v<libbsa::detail::finalization_workspace>);
    STATIC_REQUIRE(std::is_nothrow_destructible_v<libbsa::detail::finalization_workspace>);
    STATIC_REQUIRE_FALSE(valid_writer_publish_callback<path_only_publish_callback>);
}

TEST_CASE(
    "writer_publish refuses an existing destination before writing when "
    "overwrite is disabled",
    "[unit][writer_publish][publish]") {
    const auto archive = output_path("overwrite-disabled-existing.bsa");
    const auto sentinel = bytes_from_text("old archive bytes");
    bool callback_called = false;
    write_binary_file(archive, sentinel);

    auto published = libbsa::detail::publish_writer_output(
        archive, false, "TES4 BSA writer",
        [&](const libbsa::detail::finalization_workspace&) -> libbsa::result<void> {
            callback_called = true;
            return {};
        });

    REQUIRE_FALSE(published.has_value());
    CHECK(published.error().code == libbsa::error_code::io_error);
    CHECK(published.error().message.find("TES4 BSA writer") != std::string::npos);
    CHECK_FALSE(callback_called);
    CHECK(read_binary_file(archive) == sentinel);
}

TEST_CASE(
    "writer_publish no-overwrite publication preserves a raced "
    "destination and cleans temp output",
    "[unit][writer_publish][publish]") {
    const auto archive = output_path("raced-destination.ba2");
    const auto expected = bytes_from_text("completed archive");
    const auto raced = bytes_from_text("raced destination");
    std::filesystem::path observed_temp_dir;

    auto published = libbsa::detail::publish_writer_output(
        archive, false, "BA2 GNRL writer",
        [&](const libbsa::detail::finalization_workspace& workspace) -> libbsa::result<void> {
            const auto& temp_path = workspace.temporary_archive_path();
            observed_temp_dir = temp_path.parent_path();
            write_binary_file(temp_path, expected);
            write_binary_file(archive, raced);
            return {};
        });

    REQUIRE_FALSE(published.has_value());
    CHECK(published.error().code == libbsa::error_code::io_error);
    CHECK(published.error().message.find("BA2 GNRL writer") != std::string::npos);
    CHECK(published.error().message.find("without overwrite") != std::string::npos);
    CHECK(read_binary_file(archive) == raced);
    CHECK_FALSE(std::filesystem::exists(observed_temp_dir));
}

TEST_CASE(
    "writer_publish overwrites existing regular archives through the "
    "atomic replacement path",
    "[unit][writer_publish][publish][overwrite]") {
    const auto archive = output_path("overwrite-existing.ba2");
    const auto original = bytes_from_text("original archive");
    const auto replacement = bytes_from_text("replacement archive");
    write_binary_file(archive, original);

    auto published = libbsa::detail::publish_writer_output(
        archive, true, "BA2 DX10 writer",
        [&](const libbsa::detail::finalization_workspace& workspace) -> libbsa::result<void> {
            write_binary_file(workspace.temporary_archive_path(), replacement);
            return {};
        });

    REQUIRE(published.has_value());
    CHECK(read_binary_file(archive) == replacement);
}

TEST_CASE(
    "writer_publish preserves read-only overwrite targets when "
    "replacement is denied",
    "[unit][writer_publish][publish][overwrite]") {
#if defined(_WIN32)
    const auto archive = output_path("read-only-existing.ba2");
    const auto original = bytes_from_text("read-only original archive");
    const auto replacement = bytes_from_text("replacement archive");
    std::filesystem::path observed_temp_dir;
    write_binary_file(archive, original);
    read_only_file_guard read_only{archive};
    read_only.make_read_only();

    auto published = libbsa::detail::publish_writer_output(
        archive, true, "BA2 GNRL writer",
        [&](const libbsa::detail::finalization_workspace& workspace) -> libbsa::result<void> {
            const auto& temp_path = workspace.temporary_archive_path();
            observed_temp_dir = temp_path.parent_path();
            write_binary_file(temp_path, replacement);
            return {};
        });

    REQUIRE_FALSE(published.has_value());
    CHECK(published.error().code == libbsa::error_code::io_error);
    CHECK(published.error().message.find("BA2 GNRL writer") != std::string::npos);
    CHECK(published.error().message.find("failed to publish output host path") !=
          std::string::npos);
    CHECK(read_binary_file(archive) == original);
    CHECK_FALSE(std::filesystem::exists(observed_temp_dir));
#else
    SUCCEED(
        "read-only overwrite preservation is covered by Windows writer "
        "publish tests");
#endif
}

TEST_CASE(
    "writer_publish rejects existing reparse-point overwrite targets "
    "before writing",
    "[unit][writer_publish][publish][overwrite]") {
#if defined(_WIN32)
    const auto archive = output_path("existing-reparse-output.ba2");
    const auto target = archive.parent_path() / "existing-reparse-target.bin";
    const auto target_bytes = bytes_from_text("target bytes remain caller owned");
    bool callback_called = false;
    write_binary_file(target, target_bytes);
    if (!try_create_file_symlink(archive, target)) {
        return;
    }
    REQUIRE(is_reparse_point(archive));

    auto published = libbsa::detail::publish_writer_output(
        archive, true, "TES4 BSA writer",
        [&](const libbsa::detail::finalization_workspace&) -> libbsa::result<void> {
            callback_called = true;
            return {};
        });

    REQUIRE_FALSE(published.has_value());
    CHECK(published.error().code == libbsa::error_code::io_error);
    CHECK(published.error().message.find("TES4 BSA writer") != std::string::npos);
    CHECK(published.error().message.find("reparse") != std::string::npos);
    CHECK_FALSE(callback_called);
    CHECK(is_reparse_point(archive));
    CHECK(read_binary_file(target) == target_bytes);
#else
    SUCCEED(
        "reparse-point overwrite refusal is covered by Windows writer "
        "publish tests");
#endif
}

TEST_CASE(
    "writer_publish rejects reparse-point overwrite targets introduced "
    "before final publication",
    "[unit][writer_publish][publish][overwrite]") {
#if defined(_WIN32)
    const auto archive = output_path("raced-reparse-output.ba2");
    const auto target = archive.parent_path() / "raced-reparse-target.bin";
    const auto target_bytes = bytes_from_text("raced target bytes remain caller owned");
    const auto replacement = bytes_from_text("replacement archive bytes");
    std::filesystem::path observed_temp_dir;
    write_binary_file(target, target_bytes);

    auto published = libbsa::detail::publish_writer_output(
        archive, true, "TES3 BSA writer",
        [&](const libbsa::detail::finalization_workspace& workspace) -> libbsa::result<void> {
            const auto& temp_path = workspace.temporary_archive_path();
            observed_temp_dir = temp_path.parent_path();
            write_binary_file(temp_path, replacement);
            if (!try_create_file_symlink(archive, target)) {
                return libbsa::error{libbsa::error_code::io_error,
                                     "test host cannot create raced reparse point"};
            }
            return {};
        });

    if (!published.has_value() &&
        published.error().message == "test host cannot create raced reparse point") {
        WARN(
            "Skipping raced reparse-point publish test because this host cannot "
            "create file symlinks");
        return;
    }

    REQUIRE_FALSE(published.has_value());
    CHECK(published.error().code == libbsa::error_code::io_error);
    CHECK(published.error().message.find("TES3 BSA writer") != std::string::npos);
    CHECK(published.error().message.find("reparse") != std::string::npos);
    CHECK(is_reparse_point(archive));
    CHECK(read_binary_file(target) == target_bytes);
    CHECK_FALSE(std::filesystem::exists(observed_temp_dir));
#else
    SUCCEED(
        "raced reparse-point overwrite refusal is covered by Windows writer "
        "publish tests");
#endif
}

TEST_CASE("writer_publish rejects non-regular overwrite targets before writing",
          "[unit][writer_publish][publish][overwrite]") {
    const auto directory = output_path("non-regular-output.bsa");
    std::error_code fs_error;
    std::filesystem::remove_all(directory, fs_error);
    REQUIRE(std::filesystem::create_directory(directory));
    bool callback_called = false;

    auto published = libbsa::detail::publish_writer_output(
        directory, true, "TES4 BSA writer",
        [&](const libbsa::detail::finalization_workspace&) -> libbsa::result<void> {
            callback_called = true;
            return {};
        });

    REQUIRE_FALSE(published.has_value());
    CHECK(published.error().code == libbsa::error_code::io_error);
    CHECK(published.error().message.find("TES4 BSA writer") != std::string::npos);
    CHECK(published.error().message.find("non-regular") != std::string::npos);
    CHECK_FALSE(callback_called);
    CHECK(std::filesystem::is_directory(directory));
}

TEST_CASE(
    "writer_publish reports publish failures with the writer diagnostic "
    "prefix and cleans temp output",
    "[unit][writer_publish][publish][overwrite]") {
    const auto archive = output_path("missing-temp-publish-failure.ba2");
    const auto original = bytes_from_text("original archive");
    std::filesystem::path observed_temp_dir;
    write_binary_file(archive, original);

    auto published = libbsa::detail::publish_writer_output(
        archive, true, "BA2 DX10 writer",
        [&](const libbsa::detail::finalization_workspace& workspace) -> libbsa::result<void> {
            observed_temp_dir = workspace.temporary_archive_path().parent_path();
            write_binary_file(workspace.snapshot_path(11U), bytes_from_text("prepared snapshot"));
            return {};
        });

    REQUIRE_FALSE(published.has_value());
    CHECK(published.error().code == libbsa::error_code::io_error);
    CHECK(published.error().message.find("BA2 DX10 writer") != std::string::npos);
    CHECK(published.error().message.find("failed to publish output host path") !=
          std::string::npos);
    CHECK(read_binary_file(archive) == original);
    CHECK_FALSE(std::filesystem::exists(observed_temp_dir));
}

TEST_CASE("writer_publish cleanup failure never replaces the callback error",
          "[unit][writer_publish][publish][workspace][cleanup]") {
#if defined(_WIN32)
    const auto archive = output_path("held-snapshot-cleanup-error.ba2");
    std::filesystem::path observed_workspace_dir;
    HANDLE held_snapshot = INVALID_HANDLE_VALUE;

    auto published = libbsa::detail::publish_writer_output(
        archive, false, "BA2 GNRL writer",
        [&](const libbsa::detail::finalization_workspace& workspace) -> libbsa::result<void> {
            observed_workspace_dir = workspace.temporary_archive_path().parent_path();
            const auto snapshot = workspace.snapshot_path(3U);
            write_binary_file(snapshot, bytes_from_text("held snapshot"));
            held_snapshot =
                CreateFileW(snapshot.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            REQUIRE(held_snapshot != INVALID_HANDLE_VALUE);
            return libbsa::error{libbsa::error_code::format_error, "primary finalization failure"};
        });

    REQUIRE_FALSE(published.has_value());
    CHECK(published.error().code == libbsa::error_code::format_error);
    CHECK(published.error().message == "primary finalization failure");
    CHECK(std::filesystem::exists(observed_workspace_dir));

    REQUIRE(CloseHandle(held_snapshot) != 0);
    std::error_code fs_error;
    std::filesystem::remove_all(observed_workspace_dir, fs_error);
    REQUIRE_FALSE(fs_error);
#else
    SUCCEED("cleanup-error precedence is covered by Windows writer publish tests");
#endif
}

TEST_CASE("writer_publish cleans the workspace after every finalization stage failure",
          "[unit][writer_publish][publish][workspace]") {
    libbsa::ba2_gnrl_writer_options options;
    options.compression = libbsa::archive_compression_policy::all_raw;
    auto gnrl_profile = libbsa::formats::ba2::make_ba2_profile_for_gnrl_writer(
        libbsa::ba2_gnrl_target::fallout4, options);
    REQUIRE(gnrl_profile.has_value());

    SECTION("preparation error after an earlier disk snapshot") {
        const auto source = output_path("preparation-source.bin");
        const auto missing_source = source.parent_path() / "missing-source.bin";
        const auto archive = output_path("preparation-failure.ba2");
        write_binary_file(source, bytes_from_text("snapshotted before preparation failure"));
        std::vector<libbsa::formats::ba2::ba2_gnrl_writer_entry> entries;
        entries.push_back(gnrl_disk_entry("A/Valid.bin", source));
        entries.push_back(gnrl_disk_entry("B/Missing.bin", missing_source));

        std::filesystem::path observed_temp_dir;
        std::optional<libbsa::error> stage_error;
        auto published = libbsa::detail::publish_writer_output(
            archive, false, "BA2 GNRL writer",
            [&](const libbsa::detail::finalization_workspace& workspace) -> libbsa::result<void> {
                observed_temp_dir = workspace.temporary_archive_path().parent_path();
                auto prepared = libbsa::formats::ba2::ba2_gnrl_prepare_entries(
                    gnrl_profile.value(), options, entries, 1U, workspace);
                REQUIRE_FALSE(prepared.has_value());
                CHECK(std::filesystem::exists(workspace.snapshot_path(0U)));
                stage_error = prepared.error();
                return prepared.error();
            });

        REQUIRE_FALSE(published.has_value());
        REQUIRE(stage_error.has_value());
        CHECK(published.error().code == stage_error->code);
        CHECK(published.error().message == stage_error->message);
        CHECK_FALSE(std::filesystem::exists(archive));
        CHECK_FALSE(std::filesystem::exists(observed_temp_dir));
    }

    SECTION("layout error after successful preparation") {
        const auto source = output_path("layout-source.bin");
        const auto archive = output_path("layout-failure.ba2");
        write_binary_file(source, bytes_from_text("snapshotted before layout failure"));
        std::vector<libbsa::formats::ba2::ba2_gnrl_writer_entry> entries;
        entries.push_back(gnrl_disk_entry("A/Layout.bin", source));
        auto dx10_profile = libbsa::formats::ba2::make_ba2_profile_for_dx10_writer(
            libbsa::ba2_dx10_target::fallout4, libbsa::ba2_dx10_writer_options{});
        REQUIRE(dx10_profile.has_value());

        std::filesystem::path observed_temp_dir;
        std::optional<libbsa::error> stage_error;
        auto published = libbsa::detail::publish_writer_output(
            archive, false, "BA2 GNRL writer",
            [&](const libbsa::detail::finalization_workspace& workspace) -> libbsa::result<void> {
                observed_temp_dir = workspace.temporary_archive_path().parent_path();
                auto prepared = libbsa::formats::ba2::ba2_gnrl_prepare_entries(
                    gnrl_profile.value(), options, entries, 1U, workspace);
                REQUIRE(prepared.has_value());
                REQUIRE(std::filesystem::exists(workspace.snapshot_path(0U)));

                auto plan = libbsa::formats::ba2::ba2_gnrl_plan_placements(
                    std::move(prepared).value(), dx10_profile.value(), false);
                REQUIRE_FALSE(plan.has_value());
                stage_error = plan.error();
                return plan.error();
            });

        REQUIRE_FALSE(published.has_value());
        REQUIRE(stage_error.has_value());
        CHECK(published.error().code == stage_error->code);
        CHECK(published.error().message == stage_error->message);
        CHECK_FALSE(std::filesystem::exists(archive));
        CHECK_FALSE(std::filesystem::exists(observed_temp_dir));
    }

    SECTION("serialization error after the planned snapshot becomes unavailable") {
        const auto source = output_path("serialization-source.bin");
        const auto archive = output_path("serialization-failure.ba2");
        write_binary_file(source, bytes_from_text("snapshotted before serialization failure"));
        std::vector<libbsa::formats::ba2::ba2_gnrl_writer_entry> entries;
        entries.push_back(gnrl_disk_entry("A/Serialization.bin", source));

        std::filesystem::path observed_temp_dir;
        std::optional<libbsa::error> stage_error;
        auto published = libbsa::detail::publish_writer_output(
            archive, false, "BA2 GNRL writer",
            [&](const libbsa::detail::finalization_workspace& workspace) -> libbsa::result<void> {
                observed_temp_dir = workspace.temporary_archive_path().parent_path();
                auto prepared = libbsa::formats::ba2::ba2_gnrl_prepare_entries(
                    gnrl_profile.value(), options, entries, 1U, workspace);
                REQUIRE(prepared.has_value());
                auto plan = libbsa::formats::ba2::ba2_gnrl_plan_placements(
                    std::move(prepared).value(), gnrl_profile.value(), false);
                REQUIRE(plan.has_value());

                std::error_code removal_error;
                REQUIRE(std::filesystem::remove(workspace.snapshot_path(0U), removal_error));
                REQUIRE_FALSE(removal_error);
                auto serialized = libbsa::formats::ba2::ba2_gnrl_write_archive_bytes(
                    gnrl_profile.value(), options, plan.value(),
                    workspace.temporary_archive_path());
                REQUIRE_FALSE(serialized.has_value());
                REQUIRE(std::filesystem::exists(workspace.temporary_archive_path()));
                stage_error = serialized.error();
                return serialized.error();
            });

        REQUIRE_FALSE(published.has_value());
        REQUIRE(stage_error.has_value());
        CHECK(published.error().code == stage_error->code);
        CHECK(published.error().message == stage_error->message);
        CHECK_FALSE(std::filesystem::exists(archive));
        CHECK_FALSE(std::filesystem::exists(observed_temp_dir));
    }
}

