#include <catch2/catch_test_macros.hpp>

#include <detail/host_file_path.hpp>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace
{

  constexpr std::wstring_view non_ascii_host_path_token = L"libbsa-\u00C5ngstr\u00F6m-\u65E5\u672C\u8A9E";

  std::filesystem::path project_root()
  {
    return std::filesystem::path{LIBBSA_SOURCE_DIR};
  }

  std::filesystem::path host_file_path_source_path()
  {
    return project_root() / "src" / "detail" / "host_file_path.cpp";
  }

  std::filesystem::path host_file_path_header_path()
  {
    return project_root() / "src" / "detail" / "host_file_path.hpp";
  }

  std::string read_text_file(const std::filesystem::path &path)
  {
    std::ifstream input{path, std::ios::binary};
    REQUIRE(input.is_open());
    return std::string{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
  }

  std::filesystem::path unique_non_ascii_host_path()
  {
    static std::atomic_uint64_t counter{0};
    return std::filesystem::temp_directory_path() / std::filesystem::path{std::wstring{non_ascii_host_path_token} + L"-" + std::to_wstring(counter.fetch_add(1, std::memory_order_relaxed))} / L"archive.bsa";
  }

  std::string utf8_string_from_path(const std::filesystem::path &path)
  {
    const auto utf8 = path.u8string();
    return {reinterpret_cast<const char *>(utf8.data()), utf8.size()};
  }

  std::string malformed_utf8_host_path()
  {
    std::string path{"bad-"};
    path.push_back(static_cast<char>(0xC3));
    path.push_back(static_cast<char>(0x28));
    path += ".bsa";
    return path;
  }

} // namespace

TEST_CASE("host_file_path decodes UTF-8 public host text into the native path", "[unit][host_file_path]")
{
  const auto native_path = unique_non_ascii_host_path();
  const auto utf8_host_path = utf8_string_from_path(native_path);

  auto resolved = libbsa::detail::resolve_host_file_path(utf8_host_path);

  REQUIRE(resolved.has_value());
  CHECK(resolved.value().resolved == native_path);
}

TEST_CASE("host_file_path source uses strict UTF-8 conversion instead of narrow path construction",
          "[unit][host_file_path]")
{
  const auto source = read_text_file(host_file_path_source_path());

  CHECK(source.find("MultiByteToWideChar") != std::string::npos);
  CHECK(source.find("MB_ERR_INVALID_CHARS") != std::string::npos);
  CHECK(source.find("std::filesystem::path{std::string{host_path}}") == std::string::npos);
}

TEST_CASE("host_file_path contract does not preserve caller UTF-8 text as dead diagnostics state",
          "[unit][host_file_path]")
{
  const auto removed_member = std::string{"original_"} + "utf8";
  const auto header = read_text_file(host_file_path_header_path());
  const auto source = read_text_file(host_file_path_source_path());

  CHECK(header.find(removed_member) == std::string::npos);
  CHECK(source.find(removed_member) == std::string::npos);
}

TEST_CASE("host_file_path rejects malformed UTF-8 before filesystem I/O", "[unit][host_file_path]")
{
  auto resolved = libbsa::detail::resolve_host_file_path(malformed_utf8_host_path());

  REQUIRE_FALSE(resolved.has_value());
  CHECK(resolved.error().code == libbsa::error_code::invalid_argument);
}
