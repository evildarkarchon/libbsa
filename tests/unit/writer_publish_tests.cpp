#include <catch2/catch_test_macros.hpp>

#include <detail/writer_publish.hpp>

#include <libbsa/result.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
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

std::filesystem::path writer_publish_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_writer_publish_tests";
  std::filesystem::create_directories(path);
  return path;
}

std::filesystem::path output_path(std::string name) {
  static std::atomic_uint64_t counter{0};
  auto path = writer_publish_test_dir() / std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
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

void write_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  REQUIRE(output.good());
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
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

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream input{path};
  REQUIRE(input.good());
  return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
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
      // Cleanup must restore write access so the test-owned temp tree can be removed later.
      SetFileAttributesW(path_.c_str(), attributes & ~FILE_ATTRIBUTE_READONLY);
    }
  }

 private:
  std::filesystem::path path_;
};

bool is_reparse_point(const std::filesystem::path& path) {
  const auto attributes = GetFileAttributesW(path.c_str());
  return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

bool try_create_file_symlink(const std::filesystem::path& link_path, const std::filesystem::path& target_path) {
  std::error_code fs_error;
  std::filesystem::create_symlink(target_path, link_path, fs_error);
  if (!fs_error) {
    return true;
  }

  WARN("Skipping reparse-point publish test because this host cannot create file symlinks: " << fs_error.message());
  return false;
}
#endif

} // namespace

TEST_CASE("writer_publish reserves isolated temp directories and cleans them after success",
          "[unit][writer_publish][publish]") {
  const auto archive = output_path("reserve-success.bsa");
  const auto collision = archive.string() + ".tmp";
  const auto expected = bytes_from_text("new archive bytes");
  const auto sentinel = bytes_from_text("caller temp sibling");
  std::filesystem::path observed_temp_dir;
  write_binary_file(collision, sentinel);

  auto published = libbsa::detail::publish_writer_output(
      archive, false, "TES3 BSA writer", [&](const std::filesystem::path& temp_path) -> libbsa::result<void> {
        observed_temp_dir = temp_path.parent_path();
        CHECK(temp_path.filename() == archive.filename());
        write_binary_file(temp_path, expected);
        return {};
      });

  REQUIRE(published.has_value());
  CHECK(read_binary_file(archive) == expected);
  CHECK(read_binary_file(collision) == sentinel);
  CHECK_FALSE(std::filesystem::exists(observed_temp_dir));
}

TEST_CASE("writer_publish refuses an existing destination before writing when overwrite is disabled",
          "[unit][writer_publish][publish]") {
  const auto archive = output_path("overwrite-disabled-existing.bsa");
  const auto sentinel = bytes_from_text("old archive bytes");
  bool callback_called = false;
  write_binary_file(archive, sentinel);

  auto published = libbsa::detail::publish_writer_output(
      archive, false, "TES4 BSA writer", [&](const std::filesystem::path&) -> libbsa::result<void> {
        callback_called = true;
        return {};
      });

  REQUIRE_FALSE(published.has_value());
  CHECK(published.error().code == libbsa::error_code::io_error);
  CHECK(published.error().message.find("TES4 BSA writer") != std::string::npos);
  CHECK_FALSE(callback_called);
  CHECK(read_binary_file(archive) == sentinel);
}

TEST_CASE("writer_publish no-overwrite publication preserves a raced destination and cleans temp output",
          "[unit][writer_publish][publish]") {
  const auto archive = output_path("raced-destination.ba2");
  const auto expected = bytes_from_text("completed archive");
  const auto raced = bytes_from_text("raced destination");
  std::filesystem::path observed_temp_dir;

  auto published = libbsa::detail::publish_writer_output(
      archive, false, "BA2 GNRL writer", [&](const std::filesystem::path& temp_path) -> libbsa::result<void> {
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

TEST_CASE("writer_publish overwrites existing regular archives through the atomic replacement path",
          "[unit][writer_publish][publish][overwrite]") {
  const auto archive = output_path("overwrite-existing.ba2");
  const auto original = bytes_from_text("original archive");
  const auto replacement = bytes_from_text("replacement archive");
  write_binary_file(archive, original);

  auto published = libbsa::detail::publish_writer_output(
      archive, true, "BA2 DX10 writer", [&](const std::filesystem::path& temp_path) -> libbsa::result<void> {
        write_binary_file(temp_path, replacement);
        return {};
      });

  REQUIRE(published.has_value());
  CHECK(read_binary_file(archive) == replacement);
}

TEST_CASE("writer_publish preserves read-only overwrite targets when replacement is denied",
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
      archive, true, "BA2 GNRL writer", [&](const std::filesystem::path& temp_path) -> libbsa::result<void> {
        observed_temp_dir = temp_path.parent_path();
        write_binary_file(temp_path, replacement);
        return {};
      });

  REQUIRE_FALSE(published.has_value());
  CHECK(published.error().code == libbsa::error_code::io_error);
  CHECK(published.error().message.find("BA2 GNRL writer") != std::string::npos);
  CHECK(published.error().message.find("failed to publish output host path") != std::string::npos);
  CHECK(read_binary_file(archive) == original);
  CHECK_FALSE(std::filesystem::exists(observed_temp_dir));
#else
  SUCCEED("read-only overwrite preservation is covered by Windows writer publish tests");
#endif
}

TEST_CASE("writer_publish rejects existing reparse-point overwrite targets before writing",
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
      archive, true, "TES4 BSA writer", [&](const std::filesystem::path&) -> libbsa::result<void> {
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
  SUCCEED("reparse-point overwrite refusal is covered by Windows writer publish tests");
#endif
}

TEST_CASE("writer_publish rejects reparse-point overwrite targets introduced before final publication",
          "[unit][writer_publish][publish][overwrite]") {
#if defined(_WIN32)
  const auto archive = output_path("raced-reparse-output.ba2");
  const auto target = archive.parent_path() / "raced-reparse-target.bin";
  const auto target_bytes = bytes_from_text("raced target bytes remain caller owned");
  const auto replacement = bytes_from_text("replacement archive bytes");
  std::filesystem::path observed_temp_dir;
  write_binary_file(target, target_bytes);

  auto published = libbsa::detail::publish_writer_output(
      archive, true, "TES3 BSA writer", [&](const std::filesystem::path& temp_path) -> libbsa::result<void> {
        observed_temp_dir = temp_path.parent_path();
        write_binary_file(temp_path, replacement);
        if (!try_create_file_symlink(archive, target)) {
          return libbsa::error{libbsa::error_code::io_error, "test host cannot create raced reparse point"};
        }
        return {};
      });

  if (!published.has_value() && published.error().message == "test host cannot create raced reparse point") {
    WARN("Skipping raced reparse-point publish test because this host cannot create file symlinks");
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
  SUCCEED("raced reparse-point overwrite refusal is covered by Windows writer publish tests");
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
      directory, true, "TES4 BSA writer", [&](const std::filesystem::path&) -> libbsa::result<void> {
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

TEST_CASE("writer_publish reports publish failures with the writer diagnostic prefix and cleans temp output",
          "[unit][writer_publish][publish][overwrite]") {
  const auto archive = output_path("missing-temp-publish-failure.ba2");
  const auto original = bytes_from_text("original archive");
  std::filesystem::path observed_temp_dir;
  write_binary_file(archive, original);

  auto published = libbsa::detail::publish_writer_output(
      archive, true, "BA2 DX10 writer", [&](const std::filesystem::path& temp_path) -> libbsa::result<void> {
        observed_temp_dir = temp_path.parent_path();
        return {};
      });

  REQUIRE_FALSE(published.has_value());
  CHECK(published.error().code == libbsa::error_code::io_error);
  CHECK(published.error().message.find("BA2 DX10 writer") != std::string::npos);
  CHECK(published.error().message.find("failed to publish output host path") != std::string::npos);
  CHECK(read_binary_file(archive) == original);
  CHECK_FALSE(std::filesystem::exists(observed_temp_dir));
}

TEST_CASE("writer_publish cleans temporary output after callback errors",
          "[unit][writer_publish][publish]") {
  const auto archive = output_path("callback-error.ba2");
  std::filesystem::path observed_temp_dir;

  auto published = libbsa::detail::publish_writer_output(
      archive, false, "BA2 GNRL writer", [&](const std::filesystem::path& temp_path) -> libbsa::result<void> {
        observed_temp_dir = temp_path.parent_path();
        write_binary_file(temp_path, bytes_from_text("partial archive"));
        return libbsa::error{libbsa::error_code::io_error, "format writer failed"};
      });

  REQUIRE_FALSE(published.has_value());
  CHECK(published.error().message == "format writer failed");
  CHECK_FALSE(std::filesystem::exists(archive));
  CHECK_FALSE(std::filesystem::exists(observed_temp_dir));
}

TEST_CASE("writer_publish delegation boundary keeps all writer families on the shared helper",
          "[unit][writer_publish][publish][static_boundary]") {
  const auto root = std::filesystem::path{LIBBSA_SOURCE_DIR};
  const std::array sources{
      std::pair{"src/formats/bsa/tes3_bsa_writer.cpp", "TES3 BSA writer"},
      std::pair{"src/formats/bsa/tes4_bsa_writer.cpp", "TES4 BSA writer"},
      std::pair{"src/formats/ba2/ba2_gnrl_writer.cpp", "BA2 GNRL writer"},
      std::pair{"src/formats/ba2/ba2_dx10_writer.cpp", "BA2 DX10 writer"},
  };

  for (const auto& [relative_source, prefix] : sources) {
    INFO("writer publish delegation boundary source: " << relative_source);
    const auto text = read_text_file(root / relative_source);
    CHECK(text.find("detail::publish_writer_output(") != std::string::npos);
    CHECK(text.find(prefix) != std::string::npos);
    CHECK(text.find("make_unique_publish_directory") == std::string::npos);
    CHECK(text.find("cleanup_publish_directory") == std::string::npos);
    CHECK(text.find("reserve_backup_path") == std::string::npos);
    CHECK(text.find("restore_backup_after_publish_failure") == std::string::npos);
    CHECK(text.find("detail::publish_file_without_replace") == std::string::npos);
    CHECK(text.find("detail::replace_file_atomically") == std::string::npos);
  }
}
