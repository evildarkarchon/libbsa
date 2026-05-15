#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace
{

  std::filesystem::path source_root()
  {
    return std::filesystem::path{LIBBSA_SOURCE_DIR};
  }

  std::filesystem::path generated_archive_dir()
  {
    return source_root() / "tests" / "fixtures" / "generated" / "archives";
  }

  std::filesystem::path compatibility_matrix_path()
  {
    return source_root() / "tests" / "fixtures" / "generated" / "compatibility_matrix.json";
  }

  nlohmann::json read_json_file(const std::filesystem::path &path)
  {
    std::ifstream stream{path};
    REQUIRE(stream.is_open());
    return nlohmann::json::parse(stream);
  }

  std::string read_text_file(const std::filesystem::path &path)
  {
    std::ifstream stream{path};
    REQUIRE(stream.is_open());
    return std::string{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
  }

  bool manifest_has_case_id(const nlohmann::json &manifest, std::string_view case_id)
  {
    const auto &cases = manifest.at("cases");
    return std::any_of(cases.begin(), cases.end(), [case_id](const nlohmann::json &test_case)
                       { return test_case.at("id").get<std::string>() == case_id; });
  }

  void require_required_matrix_keys(const nlohmann::json &row)
  {
    for (const auto key : {"id", "family", "category", "evidence_type", "phase", "expected_error"})
    {
      INFO("matrix row is missing key: " << key);
      REQUIRE(row.contains(key));
    }
  }

} // namespace

TEST_CASE("compatibility_matrix schema spans malformed families and risk categories",
          "[unit][fixture][malformed][compatibility_matrix]")
{
  REQUIRE(std::filesystem::is_regular_file(compatibility_matrix_path()));
  const auto matrix = read_json_file(compatibility_matrix_path());

  REQUIRE(matrix.at("matrix_kind").get<std::string>() == "phase11_malformed_hardening");
  REQUIRE(matrix.at("rows").is_array());

  const std::set<std::string> required_families{"tes3_bsa", "tes4_bsa", "ba2_gnrl", "ba2_dx10"};
  const std::set<std::string> required_categories{"truncated_structure",
                                                  "duplicate_canonical_path",
                                                  "invalid_payload_span",
                                                  "unsupported_route",
                                                  "decompression_failure",
                                                  "oversized_arithmetic",
                                                  "dds_chunk_layout"};
  const std::set<std::string> expected_errors{"format_error", "unsupported"};

  std::set<std::string> observed_families;
  std::set<std::string> observed_categories;

  for (const auto &row : matrix.at("rows"))
  {
    require_required_matrix_keys(row);

    const auto family = row.at("family").get<std::string>();
    const auto category = row.at("category").get<std::string>();
    const auto evidence_type = row.at("evidence_type").get<std::string>();
    const auto expected_error = row.at("expected_error").get<std::string>();

    INFO("compatibility_matrix row: " << row.at("id").get<std::string>());
    CHECK(required_families.contains(family));
    CHECK(required_categories.contains(category));
    CHECK((evidence_type == "manifest" || evidence_type == "test"));
    CHECK(expected_errors.contains(expected_error));

    observed_families.insert(family);
    observed_categories.insert(category);
  }

  for (const auto &family : required_families)
  {
    INFO("missing compatibility_matrix family: " << family);
    CHECK(observed_families.contains(family));
  }
  for (const auto &category : required_categories)
  {
    INFO("missing compatibility_matrix category: " << category);
    CHECK(observed_categories.contains(category));
  }
}

TEST_CASE("compatibility_matrix evidence references resolve to manifests or test tokens",
          "[unit][fixture][malformed][compatibility_matrix]")
{
  REQUIRE(std::filesystem::is_regular_file(compatibility_matrix_path()));
  const auto matrix = read_json_file(compatibility_matrix_path());

  bool observed_manifest_evidence = false;
  bool observed_test_evidence = false;

  for (const auto &row : matrix.at("rows"))
  {
    require_required_matrix_keys(row);
    const auto row_id = row.at("id").get<std::string>();
    const auto evidence_type = row.at("evidence_type").get<std::string>();
    INFO("compatibility_matrix evidence row: " << row_id);

    if (evidence_type == "manifest")
    {
      observed_manifest_evidence = true;
      for (const auto key : {"archive", "manifest", "case_id"})
      {
        INFO("manifest-backed row is missing key: " << key);
        REQUIRE(row.contains(key));
      }
      CHECK_FALSE(row.contains("test_file"));
      CHECK_FALSE(row.contains("test_name"));

      const auto archive = generated_archive_dir() / row.at("archive").get<std::string>();
      const auto manifest_path = generated_archive_dir() / row.at("manifest").get<std::string>();
      REQUIRE(std::filesystem::is_regular_file(archive));
      REQUIRE(std::filesystem::is_regular_file(manifest_path));

      const auto manifest = read_json_file(manifest_path);
      CHECK(manifest_has_case_id(manifest, row.at("case_id").get<std::string>()));
      continue;
    }

    REQUIRE(evidence_type == "test");
    observed_test_evidence = true;
    for (const auto key : {"test_file", "test_name"})
    {
      INFO("test-backed row is missing key: " << key);
      REQUIRE(row.contains(key));
    }
    CHECK_FALSE(row.contains("archive"));
    CHECK_FALSE(row.contains("manifest"));
    CHECK_FALSE(row.contains("case_id"));

    const auto test_file = source_root() / row.at("test_file").get<std::string>();
    REQUIRE(std::filesystem::is_regular_file(test_file));
    const auto test_text = read_text_file(test_file);
    CHECK(test_text.find(row.at("test_name").get<std::string>()) != std::string::npos);
  }

  CHECK(observed_manifest_evidence);
  CHECK(observed_test_evidence);
}
