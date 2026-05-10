#include <catch2/catch_test_macros.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>

namespace {

std::filesystem::path source_root() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR};
}

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream stream{path};
  REQUIRE(stream.is_open());

  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

void require_all_tokens(std::string_view text, std::initializer_list<std::string_view> tokens) {
  for (const auto token : tokens) {
    INFO("Missing token: " << token);
    REQUIRE(text.find(token) != std::string_view::npos);
  }
}

} // namespace

TEST_CASE("thread_safety_policy public types have canonical documentation sections", "[unit][thread_safety_policy]") {
  const auto root = source_root();
  const auto archive_header = read_text_file(root / "include" / "libbsa" / "archive.hpp");
  const auto writer_header = read_text_file(root / "include" / "libbsa" / "writer.hpp");
  const auto validation_header = read_text_file(root / "include" / "libbsa" / "validation.hpp");
  const auto docs = read_text_file(root / "docs" / "thread-safety.md");

  constexpr std::array archive_types{
      "archive_reader",
      "payload_sink",
      "bulk_extract_sink_factory",
      "bulk_extract_options",
      "bulk_extract_entry_result",
  };
  for (const auto type : archive_types) {
    INFO("archive public type: " << type);
    REQUIRE(archive_header.find(type) != std::string_view::npos);
    REQUIRE(docs.find(std::string{"## "} + type) != std::string::npos);
  }

  constexpr std::array writer_types{
      "tes3_bsa_writer",
      "tes4_bsa_writer",
      "ba2_gnrl_writer",
      "ba2_dx10_writer",
      "write_execution_options",
  };
  for (const auto type : writer_types) {
    INFO("writer public type: " << type);
    REQUIRE(writer_header.find(type) != std::string_view::npos);
    REQUIRE(docs.find(std::string{"## "} + type) != std::string::npos);
  }

  constexpr std::array validation_types{
      "validation_report",
      "validate_archive",
  };
  for (const auto type : validation_types) {
    INFO("validation public type: " << type);
    REQUIRE(validation_header.find(type) != std::string_view::npos);
    REQUIRE(docs.find(std::string{"## "} + type) != std::string::npos);
  }

  REQUIRE(docs.find("## libbsa_benchmarks") != std::string::npos);
}

TEST_CASE("thread_safety_policy documents callback sink writer validation and benchmark rules",
          "[unit][thread_safety_policy]") {
  const auto docs = read_text_file(source_root() / "docs" / "thread-safety.md");

  require_all_tokens(docs,
                     {"distinct sink",
                      "bulk_extract_sink_factory::create",
                      "may be called concurrently",
                      "libbsa does not call sink factory or sink methods while holding internal locks",
                      "add_file",
                      "add_bytes",
                      "not concurrent with other mutation or `write_to`",
                      "`write_to` owns any worker scheduling",
                      "write_execution_options",
                      "worker_count > 1",
                      "no global mutable state",
                      "Independent validation calls may run concurrently",
                      "libbsa_benchmarks",
                      "report generation",
                      "not a synchronization primitive"});
}

TEST_CASE("thread_safety_policy public headers point to canonical guidance", "[unit][thread_safety_policy]") {
  const auto root = source_root();
  const auto archive_header = read_text_file(root / "include" / "libbsa" / "archive.hpp");
  const auto writer_header = read_text_file(root / "include" / "libbsa" / "writer.hpp");
  const auto validation_header = read_text_file(root / "include" / "libbsa" / "validation.hpp");

  require_all_tokens(archive_header,
                     {"Thread-safety",
                      "docs/thread-safety.md",
                      "D-23",
                      "distinct sinks"});
  require_all_tokens(writer_header,
                     {"Thread-safety",
                      "docs/thread-safety.md",
                      "D-23",
                      "mutation is not concurrent"});
  require_all_tokens(validation_header,
                     {"Thread-safety",
                      "docs/thread-safety.md",
                      "no global mutable state"});
}
