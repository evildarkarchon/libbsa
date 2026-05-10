#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

struct validation_archive_case {
  std::string_view file_name;
  libbsa::archive_type expected_type;
  libbsa::archive_variant expected_variant;
};

std::filesystem::path generated_archive_dir() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives";
}

std::filesystem::path generated_source_dir() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "source";
}

std::filesystem::path compatibility_matrix_path() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "compatibility_matrix.json";
}

std::filesystem::path validation_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_validation_api_tests";
  std::filesystem::create_directories(path);
  return path;
}

std::filesystem::path unique_output_path(std::string_view stem, std::string_view extension) {
  static std::atomic_uint64_t counter{0};
  auto path = validation_test_dir() / (std::string{stem} + "-" +
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

nlohmann::json read_json_file(const std::filesystem::path& path) {
  std::ifstream stream{path};
  REQUIRE(stream.is_open());
  return nlohmann::json::parse(stream);
}

libbsa::error_code error_code_from_matrix(std::string_view value) {
  if (value == "format_error") {
    return libbsa::error_code::format_error;
  }
  if (value == "unsupported") {
    return libbsa::error_code::unsupported;
  }
  FAIL("unknown compatibility matrix expected_error: " << value);
  return libbsa::error_code::format_error;
}

void require_valid_archive(std::string_view host_path,
                           libbsa::archive_type expected_type,
                           libbsa::archive_variant expected_variant,
                           libbsa::validation_options options = {}) {
  auto validated = libbsa::validate_archive(host_path, options);
  REQUIRE(validated.has_value());

  const auto& report = validated.value();
  CHECK(report.valid);
  CHECK(report.is_valid());
  CHECK(report.errors.empty());
  REQUIRE(report.metadata.has_value());
  CHECK(report.metadata->type == expected_type);
  CHECK(report.metadata->variant == expected_variant);
}

void require_validation_setup_error(std::string_view host_path, libbsa::error_code expected_code) {
  auto validated = libbsa::validate_archive(host_path);

  REQUIRE_FALSE(validated.has_value());
  CHECK(validated.error().code == expected_code);
}

void require_malformed_open_report(std::string_view file_name, libbsa::error_code expected_code) {
  const auto archive_path = generated_archive_dir() / std::string{file_name};

  auto opened = libbsa::archive_reader::open(archive_path.string());
  REQUIRE_FALSE(opened.has_value());
  CHECK(opened.error().code == expected_code);

  auto validated = libbsa::validate_archive(archive_path.string());
  REQUIRE(validated.has_value());
  const auto& report = validated.value();
  CHECK_FALSE(report.valid);
  CHECK_FALSE(report.is_valid());
  CHECK_FALSE(report.metadata.has_value());
  REQUIRE(report.errors.size() == 1U);
  CHECK(report.errors.front().code == expected_code);
}

bool report_has_error_code(const libbsa::validation_report& report, libbsa::error_code expected_code) {
  return std::any_of(report.errors.begin(), report.errors.end(), [expected_code](const libbsa::validation_diagnostic& error) {
    return error.code == expected_code;
  });
}

void require_matrix_open_report(const nlohmann::json& row) {
  const auto archive_path = generated_archive_dir() / row.at("archive").get<std::string>();
  const auto expected_code = error_code_from_matrix(row.at("expected_error").get<std::string>());

  auto opened = libbsa::archive_reader::open(archive_path.string());
  REQUIRE_FALSE(opened.has_value());
  CHECK(opened.error().code == expected_code);

  auto validated = libbsa::validate_archive(archive_path.string());
  REQUIRE(validated.has_value());
  const auto& report = validated.value();
  CHECK_FALSE(report.valid);
  CHECK_FALSE(report.is_valid());
  CHECK_FALSE(report.metadata.has_value());
  CHECK(report_has_error_code(report, expected_code));
}

void require_matrix_extraction_report(const nlohmann::json& row) {
  const auto archive_path = generated_archive_dir() / row.at("archive").get<std::string>();
  const auto expected_code = error_code_from_matrix(row.at("expected_error").get<std::string>());
  libbsa::validation_options options;
  options.validate_entry_extractability = true;

  auto opened = libbsa::archive_reader::open(archive_path.string());
  REQUIRE(opened.has_value());

  auto validated = libbsa::validate_archive(archive_path.string(), options);
  REQUIRE(validated.has_value());
  const auto& report = validated.value();
  CHECK_FALSE(report.valid);
  CHECK_FALSE(report.is_valid());
  CHECK(report.metadata.has_value());
  CHECK(report_has_error_code(report, expected_code));
}

std::filesystem::path write_tes3_archive() {
  const auto output = unique_output_path("validation-api-tes3-writer", ".bsa");
  libbsa::tes3_bsa_writer_options options;
  options.overwrite_existing = true;
  libbsa::tes3_bsa_writer writer{options};
  REQUIRE(writer.add_bytes("Meshes/Validation/TES3.NIF", bytes_from_text("tes3 validation payload")).has_value());
  REQUIRE(writer.write_to(output.string()).has_value());
  return output;
}

std::filesystem::path write_tes4_archive() {
  const auto output = unique_output_path("validation-api-tes4-writer", ".bsa");
  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::all_raw;
  options.overwrite_existing = true;
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
  REQUIRE(writer.add_bytes("Meshes/Validation/TES4.NIF", bytes_from_text("tes4 validation payload")).has_value());
  REQUIRE(writer.write_to(output.string()).has_value());
  return output;
}

std::filesystem::path write_ba2_gnrl_archive() {
  const auto output = unique_output_path("validation-api-ba2-gnrl-writer", ".ba2");
  libbsa::ba2_gnrl_writer_options options;
  options.compression = libbsa::archive_compression_policy::all_raw;
  options.overwrite_existing = true;
  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
  REQUIRE(writer.add_bytes("Meshes/Validation/Gnrl.nif", bytes_from_text("ba2 gnrl validation payload")).has_value());
  REQUIRE(writer.write_to(output.string()).has_value());
  return output;
}

std::filesystem::path write_ba2_dx10_archive() {
  const auto output = unique_output_path("validation-api-ba2-dx10-writer", ".ba2");
  libbsa::ba2_dx10_writer_options options;
  options.overwrite_existing = true;
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4, options};
  const auto dds_source = generated_source_dir() / "ba2_dx10_bc1_unorm.dds";
  REQUIRE(writer.add_file("textures/validation/bc1_unorm.dds", dds_source.string()).has_value());
  REQUIRE(writer.write_to(output.string()).has_value());
  return output;
}

} // namespace

TEST_CASE("validation_api reports result-level setup errors", "[unit][validation_api]") {
  require_validation_setup_error("", libbsa::error_code::invalid_argument);

  const auto missing = unique_output_path("validation-api-missing", ".bsa");
  require_validation_setup_error(missing.string(), libbsa::error_code::io_error);
}

TEST_CASE("validation_api accepts generated fixture archives", "[unit][fixture][validation_api]") {
  libbsa::validation_options options;
  options.validate_entry_extractability = true;

  constexpr std::array cases{
      validation_archive_case{"tes3_success.bsa", libbsa::archive_type::bsa, libbsa::archive_variant::tes3},
      validation_archive_case{"tes4_v103.bsa", libbsa::archive_type::bsa, libbsa::archive_variant::tes4},
      validation_archive_case{"ba2_gnrl_fo4.ba2", libbsa::archive_type::ba2, libbsa::archive_variant::fallout4},
      validation_archive_case{"ba2_dx10_fo4.ba2", libbsa::archive_type::ba2, libbsa::archive_variant::fallout4},
  };

  for (const auto& test_case : cases) {
    INFO("generated fixture: " << test_case.file_name);
    require_valid_archive((generated_archive_dir() / std::string{test_case.file_name}).string(),
                          test_case.expected_type,
                          test_case.expected_variant,
                          options);
  }
}

TEST_CASE("validation_api accepts writer-produced archives", "[unit][roundtrip][validation_api]") {
  libbsa::validation_options options;
  options.validate_entry_extractability = true;

  require_valid_archive(write_tes3_archive().string(), libbsa::archive_type::bsa, libbsa::archive_variant::tes3, options);
  require_valid_archive(write_tes4_archive().string(), libbsa::archive_type::bsa, libbsa::archive_variant::tes4, options);
  require_valid_archive(write_ba2_gnrl_archive().string(),
                        libbsa::archive_type::ba2,
                        libbsa::archive_variant::fallout4,
                        options);
  require_valid_archive(write_ba2_dx10_archive().string(),
                        libbsa::archive_type::ba2,
                        libbsa::archive_variant::fallout4,
                        options);
}

TEST_CASE("validation_api reports malformed archives without lenient readers", "[unit][fixture][malformed][validation_api]") {
  require_malformed_open_report("tes3_truncated_header.bsa", libbsa::error_code::format_error);
  require_malformed_open_report("malformed_unsupported_version.bsa", libbsa::error_code::unsupported);
  require_malformed_open_report("ba2_duplicate_canonical_path.ba2", libbsa::error_code::format_error);
  require_malformed_open_report("ba2_dx10_truncated_header.ba2", libbsa::error_code::format_error);
}

TEST_CASE("validation_api reports compatibility_matrix malformed rows as validation errors",
          "[unit][fixture][malformed][validation_api][compatibility_matrix]") {
  REQUIRE(std::filesystem::is_regular_file(compatibility_matrix_path()));
  const auto matrix = read_json_file(compatibility_matrix_path());
  bool observed_open_phase = false;
  bool observed_extraction_phase = false;

  for (const auto& row : matrix.at("rows")) {
    if (row.at("evidence_type").get<std::string>() != "manifest") {
      continue;
    }

    INFO("compatibility_matrix validation row: " << row.at("id").get<std::string>());
    const auto phase = row.at("phase").get<std::string>();
    if (phase == "open") {
      observed_open_phase = true;
      require_matrix_open_report(row);
      continue;
    }
    if (phase == "extraction") {
      observed_extraction_phase = true;
      require_matrix_extraction_report(row);
      continue;
    }
    FAIL("unknown compatibility matrix validation phase: " << phase);
  }

  CHECK(observed_open_phase);
  CHECK(observed_extraction_phase);
}
