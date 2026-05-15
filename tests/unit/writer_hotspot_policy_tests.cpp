#include <catch2/catch_test_macros.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <span>
#include <string>
#include <string_view>

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
  REQUIRE(start != std::string_view::npos);

  const auto body_start = source.find('{', start);
  REQUIRE(body_start != std::string_view::npos);

  const auto end = source.find(next_signature, body_start);
  REQUIRE(end != std::string_view::npos);
  return std::string{source.substr(body_start, end - body_start)};
}

void require_all_tokens(std::string_view text, std::span<const std::string_view> tokens) {
  for (const auto token : tokens) {
    INFO("missing token: " << token);
    REQUIRE(text.find(token) != std::string_view::npos);
  }
}

void require_absent_tokens(std::string_view text, std::span<const std::string_view> tokens) {
  for (const auto token : tokens) {
    INFO("forbidden token: " << token);
    REQUIRE(text.find(token) == std::string_view::npos);
  }
}

} // namespace

TEST_CASE("writer_hotspot_policy requires TES4 dedupe candidate narrowing before exact equality",
          "[unit][writer_hotspot_policy]") {
  const auto source = read_text_file(source_root() / "src/formats/bsa/tes4_bsa_layout.cpp");
  const auto assign_offsets_body = function_body(source,
                                                 "result<tes4_layout_result> tes4_assign_offsets(",
                                                 "} // namespace libbsa::formats::bsa");

  constexpr auto narrowing_evidence = std::to_array<std::string_view>({
      "std::map<",
      "stored_size",
      "fingerprint",
      "make_tes4_dedupe_identity",
      "deduplicated_payloads.find",
      "deduplicated_payloads[",
      "tes4_stored_payloads_equal",
  });
  require_all_tokens(source, narrowing_evidence);

  constexpr auto assignment_path_evidence = std::to_array<std::string_view>({
      "deduplicated_payloads.find",
      "deduplicated_payloads[",
      "tes4_stored_payloads_equal",
  });
  require_all_tokens(assign_offsets_body, assignment_path_evidence);

  constexpr auto exact_equality_share_gate = std::to_array<std::string_view>({
      "auto duplicate = tes4_stored_payloads_equal(entry, *candidate.entry);",
      "if (!duplicate)",
      "if (duplicate.value())",
      "entry.payload_offset = candidate.assignment.offset;",
      "entry.stored_size = candidate.assignment.stored_size;",
      "entry.owns_payload_bytes = false;",
  });
  require_all_tokens(assign_offsets_body, exact_equality_share_gate);

  constexpr auto all_prior_scan_tokens = std::to_array<std::string_view>({
      "std::vector<assigned_payload> deduplicated_payloads;",
      "for (const auto& candidate : deduplicated_payloads)",
      "deduplicated_payloads.push_back(assigned_payload",
  });
  require_absent_tokens(assign_offsets_body, all_prior_scan_tokens);
}
