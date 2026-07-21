#include <catch2/catch_test_macros.hpp>

#include <detail/stored_payload.hpp>
#include <detail/host_file.hpp>
#include <detail/host_file_path.hpp>
#include <detail/writer_publish.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <ostream>
#include <span>
#include <streambuf>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

constexpr libbsa::detail::host_file_context source_context{
    "Stored Payload test failed to open source", "Stored Payload test failed to inspect source",
    "Stored Payload test failed to read source", "Stored Payload test source changed",
    "Stored Payload test source"};

std::filesystem::path unique_test_root() {
    static std::atomic<std::uint64_t> next_id{0U};
    const auto root = std::filesystem::temp_directory_path() /
                      ("libbsa-stored-payload-test-" +
                       std::to_string(next_id.fetch_add(1U, std::memory_order_relaxed)));
    std::error_code fs_error;
    std::filesystem::remove_all(root, fs_error);
    fs_error.clear();
    REQUIRE(std::filesystem::create_directories(root, fs_error));
    REQUIRE_FALSE(fs_error);
    return root;
}

struct scoped_test_root {
    std::filesystem::path path;

    ~scoped_test_root() {
        std::error_code fs_error;
        std::filesystem::remove_all(path, fs_error);
    }
};

class collecting_stream_buffer final : public std::streambuf {
   public:
    [[nodiscard]] const std::vector<std::byte>& bytes() const noexcept { return bytes_; }
    [[nodiscard]] std::size_t max_write_size() const noexcept { return max_write_size_; }

   protected:
    std::streamsize xsputn(const char* source, std::streamsize count) override {
        const auto size = static_cast<std::size_t>(count);
        max_write_size_ = (std::max)(max_write_size_, size);
        const auto* first = reinterpret_cast<const std::byte*>(source);
        bytes_.insert(bytes_.end(), first, first + static_cast<std::ptrdiff_t>(size));
        return count;
    }

   private:
    std::vector<std::byte> bytes_;
    std::size_t max_write_size_{0U};
};

class failing_stream_buffer final : public std::streambuf {
   protected:
    std::streamsize xsputn(const char*, std::streamsize count) override {
        if (successful_writes_++ == 0U) {
            return count;
        }
        return 0;
    }

   private:
    std::size_t successful_writes_{0U};
};

std::vector<std::byte> bytes_from_text(std::string_view text) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const auto ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

void write_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    REQUIRE(output.good());
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary | std::ios::ate};
    REQUIRE(input.good());
    const auto size = input.tellg();
    REQUIRE(size >= 0);
    input.seekg(0, std::ios::beg);
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    input.read(reinterpret_cast<char*>(bytes.data()), size);
    REQUIRE(input.good());
    return bytes;
}

libbsa::detail::stored_payload make_snapshot_payload(
    const libbsa::detail::finalization_workspace& workspace,
    const std::filesystem::path& source_root, std::size_t identity,
    std::span<const std::byte> prefix, std::span<const std::byte> body) {
    const auto source_path = source_root / ("source-" + std::to_string(identity) + ".bin");
    write_binary_file(source_path, body);
    auto resolved = libbsa::detail::resolve_host_file_path(source_path.string());
    REQUIRE(resolved.has_value());
    auto opened = libbsa::detail::stable_host_file_session::open(resolved.value(), source_context);
    REQUIRE(opened.has_value());
    auto snapshotted = libbsa::detail::stored_payload::from_workspace_snapshot(
        std::vector<std::byte>{prefix.begin(), prefix.end()}, std::move(opened).value(), workspace,
        identity, 2U);
    REQUIRE(snapshotted.has_value());
    return std::move(snapshotted).value();
}

}  // namespace

TEST_CASE("Stored Payload owns final bytes with lazy cached identity", "[unit][stored_payload]") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<libbsa::detail::stored_payload>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<libbsa::detail::stored_payload>);
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<libbsa::detail::stored_payload>);
    STATIC_REQUIRE(std::is_nothrow_move_assignable_v<libbsa::detail::stored_payload>);
    STATIC_REQUIRE(std::is_nothrow_destructible_v<libbsa::detail::stored_payload>);

    const auto expected = bytes_from_text("owned final stored bytes");
    auto first = libbsa::detail::stored_payload::from_owned_bytes(expected);
    auto second = libbsa::detail::stored_payload::from_owned_bytes(expected);

    CHECK(first.size() == expected.size());

    const auto fingerprint = first.fingerprint();

    CHECK(first.fingerprint() == fingerprint);
    REQUIRE(first.exactly_equals(second).has_value());
    CHECK(first.exactly_equals(second).value());
}

TEST_CASE("Stored Payload moves leave one valid logical owner", "[unit][stored_payload]") {
    const auto expected = bytes_from_text("move-only stored bytes");
    auto source = libbsa::detail::stored_payload::from_owned_bytes(expected);
    auto moved = std::move(source);

    CHECK(moved.size() == expected.size());
    CHECK(source.size() == 0U);
    auto empty = libbsa::detail::stored_payload::from_owned_bytes({});
    REQUIRE(source.exactly_equals(empty).has_value());
    CHECK(source.exactly_equals(empty).value());

    auto assigned = libbsa::detail::stored_payload::from_owned_bytes(bytes_from_text("replaced"));
    assigned = std::move(moved);

    CHECK(assigned.size() == expected.size());
    CHECK(moved.size() == 0U);
    REQUIRE(moved.exactly_equals(empty).has_value());
    CHECK(moved.exactly_equals(empty).value());
}

TEST_CASE("Stored Payload snapshots one stable source observation with prefix identity",
          "[unit][stored_payload][snapshot]") {
    const scoped_test_root root{unique_test_root()};
    const auto output_path = root.path / "archive.ba2";
    auto reserved =
        libbsa::detail::finalization_workspace::reserve(output_path, "Stored Payload test");
    REQUIRE(reserved.has_value());
    auto workspace = std::move(reserved).value();

    const auto prefix = bytes_from_text("prefix:");
    const auto body = bytes_from_text("stable snapshot body");
    const auto source_path = root.path / "source.bin";
    write_binary_file(source_path, body);
    auto resolved = libbsa::detail::resolve_host_file_path(source_path.string());
    REQUIRE(resolved.has_value());
    auto opened = libbsa::detail::stable_host_file_session::open(resolved.value(), source_context);
    REQUIRE(opened.has_value());

    // Snapshot creation must rewind because format preparation may already have
    // probed a prefix through this same coherent source observation.
    REQUIRE(opened.value().read_prefix(4U).has_value());
    const auto snapshot_path = workspace.snapshot_path(7U);
    auto snapshotted = libbsa::detail::stored_payload::from_workspace_snapshot(
        prefix, std::move(opened).value(), workspace, 7U, 3U);

    REQUIRE(snapshotted.has_value());
    CHECK(snapshotted.value().size() == prefix.size() + body.size());
    CHECK(read_binary_file(snapshot_path) == body);

    auto logical_bytes = prefix;
    logical_bytes.insert(logical_bytes.end(), body.begin(), body.end());
    auto owned = libbsa::detail::stored_payload::from_owned_bytes(logical_bytes);
    CHECK(snapshotted.value().fingerprint() == owned.fingerprint());
}

TEST_CASE("Stored Payload snapshots zero-length bodies", "[unit][stored_payload][snapshot]") {
    const scoped_test_root root{unique_test_root()};
    auto reserved = libbsa::detail::finalization_workspace::reserve(root.path / "archive.bsa",
                                                                    "Stored Payload test");
    REQUIRE(reserved.has_value());
    auto workspace = std::move(reserved).value();

    const auto source_path = root.path / "empty.bin";
    write_binary_file(source_path, {});
    auto resolved = libbsa::detail::resolve_host_file_path(source_path.string());
    REQUIRE(resolved.has_value());
    auto opened = libbsa::detail::stable_host_file_session::open(resolved.value(), source_context);
    REQUIRE(opened.has_value());

    const auto prefix = bytes_from_text("prefix-only");
    const auto snapshot_path = workspace.snapshot_path(0U);
    auto snapshotted = libbsa::detail::stored_payload::from_workspace_snapshot(
        prefix, std::move(opened).value(), workspace, 0U);

    REQUIRE(snapshotted.has_value());
    CHECK(snapshotted.value().size() == prefix.size());
    CHECK(read_binary_file(snapshot_path).empty());

    auto fully_empty = make_snapshot_payload(workspace, root.path, 1U, {}, {});
    auto empty_owned = libbsa::detail::stored_payload::from_owned_bytes({});
    CHECK(fully_empty.size() == 0U);
    REQUIRE(fully_empty.exactly_equals(empty_owned).has_value());
    CHECK(fully_empty.exactly_equals(empty_owned).value());
}

TEST_CASE("Stored Payload exact equality spans every storage-adapter combination",
          "[unit][stored_payload][equality]") {
    const scoped_test_root root{unique_test_root()};
    auto reserved = libbsa::detail::finalization_workspace::reserve(root.path / "archive.ba2",
                                                                    "Stored Payload test");
    REQUIRE(reserved.has_value());
    auto workspace = std::move(reserved).value();

    const auto logical = bytes_from_text("prefix-body");
    auto owned_first = libbsa::detail::stored_payload::from_owned_bytes(logical);
    auto owned_second = libbsa::detail::stored_payload::from_owned_bytes(logical);
    auto snapshot_first = make_snapshot_payload(workspace, root.path, 10U, bytes_from_text("pre"),
                                                bytes_from_text("fix-body"));
    auto snapshot_second = make_snapshot_payload(
        workspace, root.path, 11U, bytes_from_text("prefix-"), bytes_from_text("body"));

    REQUIRE(owned_first.exactly_equals(owned_second).has_value());
    CHECK(owned_first.exactly_equals(owned_second).value());
    REQUIRE(owned_first.exactly_equals(snapshot_first).has_value());
    CHECK(owned_first.exactly_equals(snapshot_first).value());
    REQUIRE(snapshot_first.exactly_equals(owned_first).has_value());
    CHECK(snapshot_first.exactly_equals(owned_first).value());
    REQUIRE(snapshot_first.exactly_equals(snapshot_second).has_value());
    CHECK(snapshot_first.exactly_equals(snapshot_second).value());
    CHECK(snapshot_first.fingerprint() == snapshot_second.fingerprint());
}

TEST_CASE("Stored Payload exact equality rejects genuine fingerprint collisions",
          "[unit][stored_payload][equality][collision]") {
    const scoped_test_root root{unique_test_root()};
    auto reserved = libbsa::detail::finalization_workspace::reserve(root.path / "archive.bsa",
                                                                    "Stored Payload test");
    REQUIRE(reserved.has_value());
    auto workspace = std::move(reserved).value();

    const std::vector<std::byte> first_bytes{
        std::byte{0x1D}, std::byte{0x50}, std::byte{0xD0}, std::byte{0x37},
        std::byte{0x4E}, std::byte{0xC6}, std::byte{0xF8}, std::byte{0x00},
    };
    const std::vector<std::byte> second_bytes{
        std::byte{0xB1}, std::byte{0xFB}, std::byte{0x78}, std::byte{0x97},
        std::byte{0xB8}, std::byte{0x62}, std::byte{0x20}, std::byte{0x25},
    };
    constexpr std::uint64_t colliding_fingerprint = 0x4C1655569D1ACD7DULL;

    auto first_owned = libbsa::detail::stored_payload::from_owned_bytes(first_bytes);
    auto second_owned = libbsa::detail::stored_payload::from_owned_bytes(second_bytes);
    const auto first_span = std::span<const std::byte>{first_bytes};
    const auto second_span = std::span<const std::byte>{second_bytes};
    auto first_snapshot = make_snapshot_payload(workspace, root.path, 20U, first_span.first(3U),
                                                first_span.subspan(3U));
    auto second_snapshot = make_snapshot_payload(workspace, root.path, 21U, second_span.first(5U),
                                                 second_span.subspan(5U));

    // These are real FNV-1a64 collisions, so every adapter pairing must still
    // consult authoritative bytes rather than accepting the narrowing key.
    for (const auto* payload : {&first_owned, &second_owned, &first_snapshot, &second_snapshot}) {
        CHECK(payload->size() == first_bytes.size());
        CHECK(payload->fingerprint() == colliding_fingerprint);
    }

    REQUIRE(first_owned.exactly_equals(second_owned).has_value());
    CHECK_FALSE(first_owned.exactly_equals(second_owned).value());
    REQUIRE(first_owned.exactly_equals(second_snapshot).has_value());
    CHECK_FALSE(first_owned.exactly_equals(second_snapshot).value());
    REQUIRE(first_snapshot.exactly_equals(second_snapshot).has_value());
    CHECK_FALSE(first_snapshot.exactly_equals(second_snapshot).value());

    const auto shared_body = bytes_from_text("-shared-body");
    auto first_prefix =
        make_snapshot_payload(workspace, root.path, 22U, bytes_from_text("aa"), shared_body);
    auto second_prefix =
        make_snapshot_payload(workspace, root.path, 23U, bytes_from_text("bb"), shared_body);
    CHECK(first_prefix.size() == second_prefix.size());
    REQUIRE(first_prefix.exactly_equals(second_prefix).has_value());
    CHECK_FALSE(first_prefix.exactly_equals(second_prefix).value());
}

TEST_CASE("Stored Payload exact equality includes prefixes with zero-length bodies",
          "[unit][stored_payload][equality][snapshot]") {
    const scoped_test_root root{unique_test_root()};
    auto reserved = libbsa::detail::finalization_workspace::reserve(root.path / "archive.bsa",
                                                                    "Stored Payload test");
    REQUIRE(reserved.has_value());
    auto workspace = std::move(reserved).value();

    const auto prefix = bytes_from_text("prefix-only");
    auto owned = libbsa::detail::stored_payload::from_owned_bytes(prefix);
    auto first = make_snapshot_payload(workspace, root.path, 30U, prefix, {});
    auto second = make_snapshot_payload(workspace, root.path, 31U, prefix, {});

    REQUIRE(owned.exactly_equals(first).has_value());
    CHECK(owned.exactly_equals(first).value());
    REQUIRE(first.exactly_equals(second).has_value());
    CHECK(first.exactly_equals(second).value());
}

TEST_CASE("Stored Payload bounded emission reproduces its exact logical bytes",
          "[unit][stored_payload][emission]") {
    const scoped_test_root root{unique_test_root()};
    auto reserved = libbsa::detail::finalization_workspace::reserve(root.path / "archive.bsa",
                                                                    "Stored Payload test");
    REQUIRE(reserved.has_value());
    auto workspace = std::move(reserved).value();

    const auto prefix = bytes_from_text("prefix:");
    const auto body = bytes_from_text("snapshot-body");
    auto snapshot = make_snapshot_payload(workspace, root.path, 40U, prefix, body);
    auto expected = prefix;
    expected.insert(expected.end(), body.begin(), body.end());
    auto owned = libbsa::detail::stored_payload::from_owned_bytes(expected);

    for (const auto* payload : {&owned, &snapshot}) {
        collecting_stream_buffer buffer;
        std::ostream output{&buffer};
        auto emitted = payload->emit(output, 3U);

        REQUIRE(emitted.has_value());
        CHECK(buffer.bytes() == expected);
        CHECK(buffer.max_write_size() <= 3U);
    }
}

TEST_CASE("Stored Payload snapshots outlive original-source mutation and deletion",
          "[unit][stored_payload][snapshot][lifetime]") {
    const scoped_test_root root{unique_test_root()};
    auto reserved = libbsa::detail::finalization_workspace::reserve(root.path / "archive.ba2",
                                                                    "Stored Payload test");
    REQUIRE(reserved.has_value());
    auto workspace = std::move(reserved).value();

    const auto prefix = bytes_from_text("prefix:");
    const auto body = bytes_from_text("original stable body");
    const auto source_path = root.path / "source-50.bin";
    auto snapshot = make_snapshot_payload(workspace, root.path, 50U, prefix, body);
    write_binary_file(source_path, bytes_from_text("mutated original source"));
    std::error_code removal_error;
    REQUIRE(std::filesystem::remove(source_path, removal_error));
    REQUIRE_FALSE(removal_error);

    auto expected = prefix;
    expected.insert(expected.end(), body.begin(), body.end());
    auto owned = libbsa::detail::stored_payload::from_owned_bytes(expected);
    REQUIRE(snapshot.exactly_equals(owned, 4U).has_value());
    CHECK(snapshot.exactly_equals(owned, 4U).value());

    collecting_stream_buffer buffer;
    std::ostream output{&buffer};
    REQUIRE(snapshot.emit(output, 4U).has_value());
    CHECK(buffer.bytes() == expected);
}

TEST_CASE("Stored Payload destruction leaves snapshot cleanup to its workspace",
          "[unit][stored_payload][snapshot][lifetime]") {
    const scoped_test_root root{unique_test_root()};
    std::filesystem::path snapshot_path;
    std::filesystem::path workspace_path;
    {
        auto reserved = libbsa::detail::finalization_workspace::reserve(root.path / "archive.bsa",
                                                                        "Stored Payload test");
        REQUIRE(reserved.has_value());
        auto workspace = std::move(reserved).value();
        snapshot_path = workspace.snapshot_path(60U);
        workspace_path = workspace.temporary_archive_path().parent_path();
        {
            auto snapshot = make_snapshot_payload(
                workspace, root.path, 60U, bytes_from_text("prefix"), bytes_from_text("body"));
            REQUIRE(std::filesystem::exists(snapshot_path));
        }
        CHECK(std::filesystem::exists(snapshot_path));
    }
    CHECK_FALSE(std::filesystem::exists(snapshot_path));
    CHECK_FALSE(std::filesystem::exists(workspace_path));

    std::optional<libbsa::detail::stored_payload> surviving_payload;
    std::filesystem::path surviving_workspace_path;
    const auto surviving_bytes = bytes_from_text("prefixbody");
    {
        auto reserved = libbsa::detail::finalization_workspace::reserve(
            root.path / "second-archive.bsa", "Stored Payload test");
        REQUIRE(reserved.has_value());
        auto workspace = std::move(reserved).value();
        surviving_workspace_path = workspace.temporary_archive_path().parent_path();
        surviving_payload.emplace(make_snapshot_payload(
            workspace, root.path, 61U, bytes_from_text("prefix"), bytes_from_text("body")));
        REQUIRE(std::filesystem::exists(workspace.snapshot_path(61U)));
    }

    // Keeping the payload alive must not keep a workspace lease or its files alive.
    REQUIRE(surviving_payload.has_value());
    CHECK_FALSE(std::filesystem::exists(surviving_workspace_path));
    auto owned = libbsa::detail::stored_payload::from_owned_bytes(surviving_bytes);
    auto compared = surviving_payload->exactly_equals(owned);
    REQUIRE_FALSE(compared.has_value());
    CHECK(compared.error().code == libbsa::error_code::io_error);
}

TEST_CASE("Stored Payload reports snapshot comparison and emission failures",
          "[unit][stored_payload][failure]") {
    const scoped_test_root root{unique_test_root()};
    auto reserved = libbsa::detail::finalization_workspace::reserve(root.path / "archive.ba2",
                                                                    "Stored Payload test");
    REQUIRE(reserved.has_value());
    auto workspace = std::move(reserved).value();

    const auto prefix = bytes_from_text("prefix:");
    const auto body = bytes_from_text("body");
    const auto snapshot_path = workspace.snapshot_path(70U);
    auto snapshot = make_snapshot_payload(workspace, root.path, 70U, prefix, body);
    const auto cached_fingerprint = snapshot.fingerprint();
    std::error_code removal_error;
    REQUIRE(std::filesystem::remove(snapshot_path, removal_error));
    REQUIRE_FALSE(removal_error);

    // Fingerprint retrieval is cache-only after the mandatory snapshot copy.
    CHECK(snapshot.fingerprint() == cached_fingerprint);
    auto expected = prefix;
    expected.insert(expected.end(), body.begin(), body.end());
    auto owned = libbsa::detail::stored_payload::from_owned_bytes(expected);

    auto compared = snapshot.exactly_equals(owned);
    REQUIRE_FALSE(compared.has_value());
    CHECK(compared.error().code == libbsa::error_code::io_error);

    collecting_stream_buffer buffer;
    std::ostream output{&buffer};
    auto emitted = snapshot.emit(output);
    REQUIRE_FALSE(emitted.has_value());
    CHECK(emitted.error().code == libbsa::error_code::io_error);
}

TEST_CASE("Stored Payload propagates output failures without reporting success",
          "[unit][stored_payload][emission][failure]") {
    auto payload = libbsa::detail::stored_payload::from_owned_bytes(
        bytes_from_text("more than one bounded output chunk"));
    failing_stream_buffer buffer;
    std::ostream output{&buffer};

    auto emitted = payload.emit(output, 4U);

    REQUIRE_FALSE(emitted.has_value());
    CHECK(emitted.error().code == libbsa::error_code::io_error);
}

TEST_CASE("Stored Payload reports snapshot and comparison allocation limits",
          "[unit][stored_payload][failure][allocation]") {
    const scoped_test_root root{unique_test_root()};
    auto reserved = libbsa::detail::finalization_workspace::reserve(root.path / "archive.bsa",
                                                                    "Stored Payload test");
    REQUIRE(reserved.has_value());
    auto workspace = std::move(reserved).value();

    const auto body = bytes_from_text("body");
    const auto source_path = root.path / "allocation-source.bin";
    write_binary_file(source_path, body);
    auto resolved = libbsa::detail::resolve_host_file_path(source_path.string());
    REQUIRE(resolved.has_value());
    auto opened = libbsa::detail::stable_host_file_session::open(resolved.value(), source_context);
    REQUIRE(opened.has_value());

    const auto impossible_chunk = (std::numeric_limits<std::size_t>::max)();
    auto snapshotted = libbsa::detail::stored_payload::from_workspace_snapshot(
        {}, std::move(opened).value(), workspace, 80U, impossible_chunk);
    REQUIRE_FALSE(snapshotted.has_value());
    CHECK(snapshotted.error().code == libbsa::error_code::format_error);

    auto valid_snapshot = make_snapshot_payload(workspace, root.path, 81U, {}, body);
    auto owned = libbsa::detail::stored_payload::from_owned_bytes(body);
    auto compared = valid_snapshot.exactly_equals(owned, impossible_chunk);
    REQUIRE_FALSE(compared.has_value());
    CHECK(compared.error().code == libbsa::error_code::format_error);
}

TEST_CASE("Stored Payload reports snapshot creation failures",
          "[unit][stored_payload][snapshot][failure]") {
    const scoped_test_root root{unique_test_root()};
    auto reserved = libbsa::detail::finalization_workspace::reserve(root.path / "archive.ba2",
                                                                    "Stored Payload test");
    REQUIRE(reserved.has_value());
    auto workspace = std::move(reserved).value();

    const auto source_path = root.path / "source.bin";
    write_binary_file(source_path, bytes_from_text("body"));
    auto resolved = libbsa::detail::resolve_host_file_path(source_path.string());
    REQUIRE(resolved.has_value());
    auto opened = libbsa::detail::stable_host_file_session::open(resolved.value(), source_context);
    REQUIRE(opened.has_value());
    REQUIRE(std::filesystem::create_directory(workspace.snapshot_path(90U)));

    auto snapshotted = libbsa::detail::stored_payload::from_workspace_snapshot(
        {}, std::move(opened).value(), workspace, 90U);

    REQUIRE_FALSE(snapshotted.has_value());
    CHECK(snapshotted.error().code == libbsa::error_code::io_error);
}
