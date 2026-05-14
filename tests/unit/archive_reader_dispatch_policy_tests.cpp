#include <catch2/catch_test_macros.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::filesystem::path source_root() { return std::filesystem::path{LIBBSA_SOURCE_DIR}; }

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream input{path};
  REQUIRE(input.is_open());

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

std::string function_body(std::string_view source, std::string_view signature, std::string_view next_signature) {
  const auto start = source.find(signature);
  REQUIRE(start != std::string::npos);

  const auto body_start = source.find('{', start);
  REQUIRE(body_start != std::string::npos);

  const auto end = source.find(next_signature, body_start);
  REQUIRE(end != std::string::npos);
  return std::string{source.substr(body_start, end - body_start)};
}

void require_absent_tokens(std::string_view body, std::span<const std::string_view> forbidden_tokens) {
  for (const auto token : forbidden_tokens) {
    INFO("forbidden token: " << token);
    REQUIRE(body.find(token) == std::string_view::npos);
  }
}

} // namespace

TEST_CASE("archive_reader_dispatch_policy forbids repeated family dispatch in public reader methods",
          "[unit][archive_reader_dispatch_policy]") {
  const auto archive_text = read_text_file(source_root() / "src/archive.cpp");

  const auto entries_body = function_body(archive_text,
                                          "result<std::vector<entry_metadata>> archive_reader::entries() const",
                                          "result<std::optional<entry_metadata>> archive_reader::find(std::string_view path) const");
  const auto find_body = function_body(archive_text,
                                       "result<std::optional<entry_metadata>> archive_reader::find(std::string_view path) const",
                                       "result<bool> archive_reader::contains(std::string_view path) const");
  const auto contains_body = function_body(archive_text,
                                           "result<bool> archive_reader::contains(std::string_view path) const",
                                           "result<void> archive_reader::extract(std::string_view path, payload_sink& sink) const");
  const auto extract_body = function_body(archive_text,
                                          "result<void> archive_reader::extract(std::string_view path, payload_sink& sink) const",
                                          "result<std::vector<std::byte>> archive_reader::extract_bytes(std::string_view path) const");
  const auto extract_bytes_body = function_body(archive_text,
                                                "result<std::vector<std::byte>> archive_reader::extract_bytes(std::string_view path) const",
                                                "result<std::vector<bulk_extract_entry_result>> archive_reader::extract_entries(");
  const auto extract_entries_body = function_body(archive_text,
                                                  "result<std::vector<bulk_extract_entry_result>> archive_reader::extract_entries(",
                                                  "} // namespace libbsa");

  constexpr auto forbidden_dispatch_tokens = std::to_array<std::string_view>({
      "metadata.variant",
      "metadata.type",
      "is_ba2_dx10",
      "backend_identity",
      "reader_backend_identity::",
      "tes3_bsa_entries",
      "tes4_bsa_entries",
      "ba2_gnrl_entries",
      "ba2_dx10_entries",
      "find_tes3_bsa_entry",
      "find_tes4_bsa_entry",
      "find_ba2_gnrl_entry",
      "find_ba2_dx10_entry",
      "contains_tes3_bsa_entry",
      "contains_tes4_bsa_entry",
      "contains_ba2_gnrl_entry",
      "contains_ba2_dx10_entry",
      "extract_tes3_bsa_payload",
      "extract_tes4_bsa_payload_from_file",
      "extract_ba2_gnrl_payload",
      "extract_ba2_dx10_payload",
  });

  require_absent_tokens(entries_body, forbidden_dispatch_tokens);
  require_absent_tokens(find_body, forbidden_dispatch_tokens);
  require_absent_tokens(contains_body, forbidden_dispatch_tokens);
  require_absent_tokens(extract_body, forbidden_dispatch_tokens);
  require_absent_tokens(extract_bytes_body, forbidden_dispatch_tokens);
  require_absent_tokens(extract_entries_body, forbidden_dispatch_tokens);
}
