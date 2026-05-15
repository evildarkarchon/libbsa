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

TEST_CASE("writer_hotspot_policy requires BA2 GNRL staged dedupe identity before exact equality",
          "[unit][writer_hotspot_policy]") {
  const auto root = source_root();
  const auto prepare_header = read_text_file(root / "src/formats/ba2/ba2_gnrl_prepare.hpp");
  const auto prepare_source = read_text_file(root / "src/formats/ba2/ba2_gnrl_prepare.cpp");
  const auto layout_source = read_text_file(root / "src/formats/ba2/ba2_gnrl_layout.cpp");
  const auto assign_offsets_body = function_body(layout_source,
                                                 "result<void> ba2_gnrl_assign_payload_offsets(",
                                                 "namespace {");

  constexpr auto staged_identity_contract = std::to_array<std::string_view>({
      "final_stored_dedupe_hash",
      "payload_hash",
      "stored_payload",
  });
  require_all_tokens(prepare_header, staged_identity_contract);

  constexpr auto prepare_evidence = std::to_array<std::string_view>({
      "final_stored_dedupe_hash",
      "ba2_gnrl_final_stored_dedupe_hash",
      "payload_hash =",
      "hash_disk_payload",
  });
  require_all_tokens(prepare_source, prepare_evidence);

  constexpr auto layout_evidence = std::to_array<std::string_view>({
      "make_ba2_gnrl_final_stored_dedupe_key",
      "final_stored_dedupe_hash",
      "deduplicated_payloads.find",
      "deduplicated_payloads[",
      "ba2_gnrl_payloads_equal",
  });
  require_all_tokens(assign_offsets_body, layout_evidence);

  constexpr auto exact_equality_share_gate = std::to_array<std::string_view>({
      "auto equal = ba2_gnrl_payloads_equal(entry, entries[candidate.entry_index]);",
      "if (!equal)",
      "if (equal.value())",
      "entry.payload_offset = candidate.offset;",
      "entry.owns_payload_bytes = false;",
  });
  require_all_tokens(assign_offsets_body, exact_equality_share_gate);
}

TEST_CASE("writer_hotspot_policy requires BA2 GNRL disk-source change diagnostics",
          "[unit][writer_hotspot_policy]") {
  const auto layout_source = read_text_file(source_root() / "src/formats/ba2/ba2_gnrl_layout.cpp");

  constexpr auto disk_change_evidence = std::to_array<std::string_view>({
      "compare_disk_payload_to_bytes",
      "compare_disk_payloads",
      "BA2 GNRL disk source changed during dedupe preparation",
      "A file that grew after preparation can otherwise compare equal for the prepared prefix and corrupt offsets.",
      "error_code::io_error",
  });
  require_all_tokens(layout_source, disk_change_evidence);
}
