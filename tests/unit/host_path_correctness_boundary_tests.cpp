#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

constexpr std::string_view non_ascii_path_token = "libbsa-Ångström-日本語";
constexpr std::wstring_view non_ascii_path_token_wide = L"libbsa-Ångström-日本語";

std::filesystem::path project_root() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR};
}

std::filesystem::path suite_source_path() {
  return project_root() / "tests" / "unit" / "host_path_correctness_boundary_tests.cpp";
}

std::filesystem::path tests_cmake_path() {
  return project_root() / "tests" / "CMakeLists.txt";
}

std::filesystem::path generated_archive_dir() {
  return project_root() / "tests" / "fixtures" / "generated" / "archives";
}

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  REQUIRE(input.is_open());
  return std::string{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

bool contains_text(const std::string& text, std::string_view needle) {
  return text.find(needle) != std::string::npos;
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  REQUIRE(input.is_open());
  return nlohmann::json::parse(input);
}

std::vector<std::byte> bytes_from_hex(std::string_view hex) {
  REQUIRE(hex.size() % 2U == 0U);
  std::vector<std::byte> bytes;
  bytes.reserve(hex.size() / 2U);
  for (std::size_t index = 0; index < hex.size(); index += 2U) {
    bytes.push_back(static_cast<std::byte>(std::stoul(std::string{hex.substr(index, 2U)}, nullptr, 16)));
  }
  return bytes;
}

class temporary_directory_cleanup final {
 public:
  /// Owns best-effort cleanup for the copied archive root so failed assertions do not leave non-ASCII paths behind.
  explicit temporary_directory_cleanup(std::filesystem::path root) : root_{std::move(root)} {}

  ~temporary_directory_cleanup() {
    std::error_code ignored;
    std::filesystem::remove_all(root_, ignored);
  }

 private:
  std::filesystem::path root_;
};

class collecting_sink final : public libbsa::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    return bytes.size();
  }

  [[nodiscard]] const std::vector<std::byte>& bytes() const noexcept { return bytes_; }

 private:
  std::vector<std::byte> bytes_;
};

struct representative_archive_case {
  std::string_view archive_file;
  std::string_view manifest_file;
  libbsa::archive_type expected_type;
  libbsa::archive_variant expected_variant;
};

const auto& representative_archive_cases() {
  static const std::array cases{
      representative_archive_case{"tes4_v103.bsa", "tes4_v103_manifest.json", libbsa::archive_type::bsa,
                                  libbsa::archive_variant::tes4},
      representative_archive_case{"tes4_v104.bsa", "tes4_v104_manifest.json", libbsa::archive_type::bsa,
                                  libbsa::archive_variant::tes4},
      representative_archive_case{"tes4_v105.bsa", "tes4_v105_manifest.json", libbsa::archive_type::bsa,
                                  libbsa::archive_variant::tes4},
      representative_archive_case{"ba2_gnrl_fo4.ba2", "ba2_gnrl_fo4_manifest.json", libbsa::archive_type::ba2,
                                  libbsa::archive_variant::fallout4},
      representative_archive_case{"ba2_dx10_fo4.ba2", "ba2_dx10_fo4_manifest.json", libbsa::archive_type::ba2,
                                  libbsa::archive_variant::fallout4},
      representative_archive_case{"ba2_gnrl_sfv3.ba2", "ba2_gnrl_sfv3_manifest.json", libbsa::archive_type::ba2,
                                  libbsa::archive_variant::starfield}};
  return cases;
}

std::filesystem::path unique_non_ascii_root() {
  static std::atomic_uint64_t counter{0};
  return std::filesystem::temp_directory_path()
         / std::filesystem::path{std::wstring{non_ascii_path_token_wide} + L"-host-path-"
                                 + std::to_wstring(counter.fetch_add(1, std::memory_order_relaxed))};
}

std::filesystem::path copied_archive_path(const representative_archive_case& archive_case, const std::filesystem::path& root) {
  const auto source = generated_archive_dir() / std::string{archive_case.archive_file};
  const auto renamed = std::filesystem::path{source.stem().wstring() + L"-" + std::wstring{non_ascii_path_token_wide}
                                             + source.extension().wstring()};
  return root / renamed;
}

std::filesystem::path copy_archive_under_test(const representative_archive_case& archive_case, const std::filesystem::path& root) {
  std::filesystem::create_directories(root);
  const auto source = generated_archive_dir() / std::string{archive_case.archive_file};
  const auto copied = copied_archive_path(archive_case, root);
  std::filesystem::copy_file(source, copied, std::filesystem::copy_options::overwrite_existing);
  return copied;
}

const nlohmann::json& canonical_entry_from_manifest(const nlohmann::json& manifest) {
  const auto& entries = manifest.at("entries");
  REQUIRE_FALSE(entries.empty());
  return entries.front();
}

bool canonical_extraction_matches_manifest(const representative_archive_case&) {
  return false;
}

} // namespace

TEST_CASE("host_path_correctness_boundary suite registration is wired into libbsa_tests",
          "[unit][host_path_correctness_boundary]") {
  const auto tests_cmake = read_text_file(tests_cmake_path());

  REQUIRE(contains_text(tests_cmake, "unit/host_path_correctness_boundary_tests.cpp"));
}

TEST_CASE("host_path_correctness_boundary suite source carries the locked non-ASCII token and smoke selector",
          "[unit][host_path_correctness_boundary][host_path_correctness_boundary_smoke]") {
  REQUIRE(non_ascii_path_token == "libbsa-Ångström-日本語");
}

TEST_CASE("host_path_correctness_boundary smoke setup uses the locked non-ASCII directory and filename without copying manifests",
          "[unit][fixture][host_path_correctness_boundary][host_path_correctness_boundary_smoke]") {
  const auto& archive_case = representative_archive_cases().front();
  const auto manifest_path = generated_archive_dir() / std::string{archive_case.manifest_file};
  const auto manifest = read_json_file(manifest_path);
  const auto& canonical_entry = canonical_entry_from_manifest(manifest);
  const auto root = unique_non_ascii_root();
  temporary_directory_cleanup cleanup{root};
  const auto expected_copied_archive = copied_archive_path(archive_case, root);
  const auto copied_archive = copy_archive_under_test(archive_case, root);

  REQUIRE(copied_archive.parent_path() == root);
  REQUIRE(copied_archive == expected_copied_archive);
  REQUIRE(std::filesystem::exists(copied_archive));
  REQUIRE(manifest_path.native().find(root.native()) == std::filesystem::path::string_type::npos);
  REQUIRE(canonical_entry.at("path").get<std::string>().empty() == false);
  REQUIRE_FALSE(bytes_from_hex(canonical_entry.at("expected").at("bytes_hex").get<std::string>()).empty());
}

TEST_CASE("host_path_correctness_boundary representative archives open validate and extract from non-ASCII paths",
          "[unit][fixture][host_path_correctness_boundary]") {
  REQUIRE(representative_archive_cases().size() == 6U);

  for (const auto& archive_case : representative_archive_cases()) {
    INFO(archive_case.archive_file);
    REQUIRE(canonical_extraction_matches_manifest(archive_case));
  }
}
