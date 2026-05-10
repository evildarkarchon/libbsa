#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::filesystem::path warning_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_compatibility_warning_tests";
  std::filesystem::create_directories(path);
  return path;
}

std::filesystem::path unique_output_path(std::string_view stem, std::string_view extension) {
  static std::atomic_uint64_t counter{0};
  auto path = warning_test_dir() / (std::string{stem} + "-" +
                                   std::to_string(counter.fetch_add(1, std::memory_order_relaxed)) +
                                   std::string{extension});
  std::filesystem::remove(path);
  return path;
}

std::vector<std::byte> bytes_from_text(std::string_view text) {
  std::vector<std::byte> bytes;
  bytes.reserve(text.size());
  for (const char ch : text) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
}

std::vector<std::byte> repeated_text_bytes(std::string_view text, std::size_t repetitions) {
  std::vector<std::byte> bytes;
  bytes.reserve(text.size() * repetitions);
  for (std::size_t index = 0; index < repetitions; ++index) {
    const auto chunk = bytes_from_text(text);
    bytes.insert(bytes.end(), chunk.begin(), chunk.end());
  }
  return bytes;
}

const libbsa::compatibility_warning& require_warning(const libbsa::validation_report& report,
                                                     libbsa::compatibility_warning_code code,
                                                     libbsa::compatibility_warning_severity severity,
                                                     bool has_archive_path) {
  for (const auto& warning : report.warnings) {
    if (warning.code == code) {
      CHECK(warning.severity == severity);
      CHECK(warning.archive_path.has_value() == has_archive_path);
      return warning;
    }
  }
  FAIL("missing expected compatibility warning");
}

libbsa::validation_report require_validated_report(const std::filesystem::path& archive,
                                                   libbsa::validation_options options = {}) {
  auto validated = libbsa::validate_archive(archive.string(), options);
  REQUIRE(validated.has_value());
  CHECK(validated.value().is_valid());
  CHECK(validated.value().errors.empty());
  return validated.value();
}

std::filesystem::path write_raw_ba2_archive() {
  const auto output = unique_output_path("target-family-mismatch", ".ba2");
  libbsa::ba2_gnrl_writer_options options;
  options.compression = libbsa::archive_compression_policy::all_raw;
  options.overwrite_existing = true;
  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
  REQUIRE(writer.add_bytes("Meshes/Mismatch/Probe.nif", bytes_from_text("ba2 target mismatch payload")).has_value());
  REQUIRE(writer.write_to(output.string()).has_value());
  return output;
}

std::filesystem::path write_embedded_name_bsa_archive() {
  const auto output = unique_output_path("embedded-name-risk", ".bsa");
  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::all_raw;
  options.embed_file_names = true;
  options.overwrite_existing = true;
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
  REQUIRE(writer.add_bytes("Meshes/Embedded/Model.nif", bytes_from_text("embedded name compatibility payload"))
              .has_value());
  REQUIRE(writer.write_to(output.string()).has_value());
  return output;
}

std::filesystem::path write_compressed_sound_bsa_archive() {
  const auto output = unique_output_path("compressed-sound-risk", ".bsa");
  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::all_compressed;
  options.overwrite_existing = true;
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
  REQUIRE(writer.add_bytes("sound/fx/alert.wav", repeated_text_bytes("alert sound payload ", 16U)).has_value());
  REQUIRE(writer.write_to(output.string()).has_value());
  return output;
}

} // namespace

TEST_CASE("compatibility_warning reports BA2 target family mismatch",
          "[unit][compat][compatibility_warning]") {
  libbsa::validation_options options;
  options.expected_type = libbsa::archive_type::bsa;

  const auto report = require_validated_report(write_raw_ba2_archive(), options);

  require_warning(report,
                  libbsa::compatibility_warning_code::target_family_mismatch,
                  libbsa::compatibility_warning_severity::risky,
                  false);
}

TEST_CASE("compatibility_warning reports BSA embedded name compatibility risk",
          "[unit][compat][compatibility_warning]") {
  const auto report = require_validated_report(write_embedded_name_bsa_archive());

  require_warning(report,
                  libbsa::compatibility_warning_code::bsa_embedded_name_compatibility_risk,
                  libbsa::compatibility_warning_severity::risky,
                  true);
}

TEST_CASE("compatibility_warning reports compressed sound payloads",
          "[unit][compat][compatibility_warning]") {
  const auto report = require_validated_report(write_compressed_sound_bsa_archive());

  require_warning(report,
                  libbsa::compatibility_warning_code::compressed_sound_payload,
                  libbsa::compatibility_warning_severity::advisory,
                  true);
}
