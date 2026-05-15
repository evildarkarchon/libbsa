#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace
{

  std::filesystem::path generated_archive_dir()
  {
    return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives";
  }

  std::filesystem::path generated_archive_path(std::string_view filename)
  {
    return generated_archive_dir() / std::string{filename};
  }

  nlohmann::json read_json_file(const std::filesystem::path &path)
  {
    std::ifstream stream{path};
    REQUIRE(stream.is_open());
    return nlohmann::json::parse(stream);
  }

  std::vector<std::byte> bytes_from_hex(std::string_view hex)
  {
    REQUIRE(hex.size() % 2U == 0U);
    std::vector<std::byte> bytes;
    bytes.reserve(hex.size() / 2U);
    for (std::size_t offset = 0; offset < hex.size(); offset += 2U)
    {
      const auto pair = std::string{hex.substr(offset, 2U)};
      bytes.push_back(static_cast<std::byte>(std::stoul(pair, nullptr, 16)));
    }
    return bytes;
  }

  struct dispatch_fixture
  {
    std::string name;
    std::string archive_filename;
    std::string manifest_filename;
    std::string expected_extract_path;
    std::string expected_missing_path;
    std::string invalid_archive_path;
  };

  std::vector<dispatch_fixture> dispatch_fixtures()
  {
    return {{"tes3_bsa", "tes3_success.bsa", "tes3_success_manifest.json", "meshes/tiny/probe.nif", "valid/missing/path.txt",
             "folder//file.txt"},
            {"tes4_bsa", "tes4_v103.bsa", "tes4_v103_manifest.json", "meshes/tiny/rawmesh.nif", "valid/missing/path.txt",
             "folder//file.txt"},
            {"ba2_gnrl_fo4", "ba2_gnrl_fo4.ba2", "ba2_gnrl_fo4_manifest.json", "meshes/mixedcase/probe.nif",
             "valid/missing/path.txt", "folder//file.txt"},
            {"ba2_gnrl_starfield_v3", "ba2_gnrl_sfv3.ba2", "ba2_gnrl_sfv3_manifest.json", "geometries/packed/block.mesh",
             "valid/missing/path.txt", "folder//file.txt"},
            {"ba2_dx10", "ba2_dx10_fo4.ba2", "ba2_dx10_fo4_manifest.json", "textures/generated/fo4raw.dds",
             "valid/missing/path.txt", "folder//file.txt"}};
  }

  const nlohmann::json &find_manifest_entry(const nlohmann::json &manifest, std::string_view path)
  {
    const auto found = std::find_if(manifest.at("entries").begin(), manifest.at("entries").end(), [&](const auto &entry)
                                    { return entry.at("path").template get<std::string>() == path; });
    REQUIRE(found != manifest.at("entries").end());
    return *found;
  }

  std::vector<std::byte> expected_payload_bytes(const nlohmann::json &manifest_entry)
  {
    if (manifest_entry.contains("expected") && manifest_entry.at("expected").contains("bytes_hex"))
    {
      return bytes_from_hex(manifest_entry.at("expected").at("bytes_hex").get<std::string>());
    }
    return bytes_from_hex(manifest_entry.at("expected_dds_payload").at("bytes_hex").get<std::string>());
  }

  class collecting_sink final : public libbsa::payload_sink
  {
  public:
    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override
    {
      bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
      return bytes.size();
    }

    [[nodiscard]] const std::vector<std::byte> &bytes() const noexcept { return bytes_; }

  private:
    std::vector<std::byte> bytes_;
  };

  struct sink_capture
  {
    std::vector<std::byte> bytes;
  };

  class capturing_sink final : public libbsa::payload_sink
  {
  public:
    explicit capturing_sink(std::shared_ptr<sink_capture> capture) : capture_(std::move(capture)) {}

    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override
    {
      capture_->bytes.insert(capture_->bytes.end(), bytes.begin(), bytes.end());
      return bytes.size();
    }

  private:
    std::shared_ptr<sink_capture> capture_;
  };

  struct capture_report
  {
    std::map<std::string, std::size_t> create_count_by_path;
    std::map<std::string, std::vector<std::vector<std::byte>>> sink_bytes_by_path;
  };

  class recording_sink_factory final : public libbsa::bulk_extract_sink_factory
  {
  public:
    libbsa::result<std::unique_ptr<libbsa::payload_sink>> create(std::string_view path,
                                                                 const libbsa::entry_metadata &) override
    {
      auto capture = std::make_shared<sink_capture>();
      const auto key = std::string{path};
      {
        std::lock_guard lock{mutex_};
        ++create_count_by_path_[key];
        captures_[key].push_back(capture);
      }
      return std::unique_ptr<libbsa::payload_sink>{new capturing_sink{std::move(capture)}};
    }

    [[nodiscard]] capture_report report() const
    {
      std::lock_guard lock{mutex_};
      capture_report report;
      report.create_count_by_path = create_count_by_path_;
      for (const auto &[path, captures] : captures_)
      {
        auto &sink_bytes = report.sink_bytes_by_path[path];
        sink_bytes.reserve(captures.size());
        for (const auto &capture : captures)
        {
          sink_bytes.push_back(capture->bytes);
        }
      }
      return report;
    }

  private:
    mutable std::mutex mutex_;
    std::map<std::string, std::size_t> create_count_by_path_;
    std::map<std::string, std::vector<std::shared_ptr<sink_capture>>> captures_;
  };

  std::vector<std::string> sorted_manifest_paths(const nlohmann::json &manifest)
  {
    std::vector<std::string> paths;
    for (const auto &entry : manifest.at("entries"))
    {
      paths.push_back(entry.at("path").get<std::string>());
    }
    std::sort(paths.begin(), paths.end());
    return paths;
  }

  std::vector<std::string> lookup_variants_for(const nlohmann::json &manifest_entry)
  {
    if (!manifest_entry.contains("lookup_variants"))
    {
      return {manifest_entry.at("path").get<std::string>()};
    }

    std::vector<std::string> variants;
    for (const auto &variant : manifest_entry.at("lookup_variants"))
    {
      variants.push_back(variant.get<std::string>());
    }
    return variants;
  }

  void require_result_entry_matches_expected(const libbsa::bulk_extract_entry_result &result, const nlohmann::json &expected)
  {
    REQUIRE(result.entry.has_value());
    REQUIRE(result.entry->path == expected.at("path").get<std::string>());
    if (expected.contains("raw_size"))
    {
      REQUIRE(result.entry->raw_size == expected.at("raw_size").get<std::uint64_t>());
    }
    if (expected.contains("stored_size"))
    {
      REQUIRE(result.entry->stored_size == expected.at("stored_size").get<std::uint64_t>());
    }
  }

} // namespace

TEST_CASE("reader_backend_dispatch preserves reader operations across representative backends",
          "[unit][fixture][reader_backend_dispatch]")
{
  for (const auto &fixture : dispatch_fixtures())
  {
    INFO("fixture: " << fixture.name);
    const auto manifest = read_json_file(generated_archive_path(fixture.manifest_filename));
    const auto &expected_entry = find_manifest_entry(manifest, fixture.expected_extract_path);
    const auto expected_bytes = expected_payload_bytes(expected_entry);

    auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive_filename).string());
    REQUIRE(opened.has_value());
    const auto &reader = opened.value();

    auto entries = reader.entries();
    REQUIRE(entries.has_value());
    REQUIRE(entries.value().size() == manifest.at("entries").size());

    const auto expected_paths = sorted_manifest_paths(manifest);
    for (std::size_t index = 0; index < entries.value().size(); ++index)
    {
      CHECK(entries.value().at(index).path == expected_paths.at(index));
    }

    for (const auto &variant : lookup_variants_for(expected_entry))
    {
      auto found = reader.find(variant);
      REQUIRE(found.has_value());
      REQUIRE(found.value().has_value());
      REQUIRE(found.value()->path == expected_entry.at("path").get<std::string>());

      auto contains = reader.contains(variant);
      REQUIRE(contains.has_value());
      REQUIRE(contains.value());
    }

    auto missing_find = reader.find(fixture.expected_missing_path);
    REQUIRE(missing_find.has_value());
    REQUIRE_FALSE(missing_find.value().has_value());

    auto missing_contains = reader.contains(fixture.expected_missing_path);
    REQUIRE(missing_contains.has_value());
    REQUIRE_FALSE(missing_contains.value());

    auto invalid_find = reader.find(fixture.invalid_archive_path);
    REQUIRE_FALSE(invalid_find.has_value());
    REQUIRE(invalid_find.error().code == libbsa::error_code::invalid_argument);

    auto invalid_contains = reader.contains(fixture.invalid_archive_path);
    REQUIRE_FALSE(invalid_contains.has_value());
    REQUIRE(invalid_contains.error().code == libbsa::error_code::invalid_argument);

    collecting_sink sink;
    auto extracted = reader.extract(fixture.expected_extract_path, sink);
    REQUIRE(extracted.has_value());

    auto extracted_bytes = reader.extract_bytes(fixture.expected_extract_path);
    REQUIRE(extracted_bytes.has_value());

    if (expected_entry.contains("expected_dds_payload"))
    {
      REQUIRE(sink.bytes().size() > expected_bytes.size());
      REQUIRE(extracted_bytes.value() == sink.bytes());
      REQUIRE(std::vector<std::byte>{sink.bytes().end() - static_cast<std::ptrdiff_t>(expected_bytes.size()), sink.bytes().end()} ==
              expected_bytes);
    }
    else
    {
      REQUIRE(sink.bytes() == expected_bytes);
      REQUIRE(extracted_bytes.value() == expected_bytes);
    }

    collecting_sink missing_sink;
    auto missing_extract = reader.extract(fixture.expected_missing_path, missing_sink);
    REQUIRE_FALSE(missing_extract.has_value());
    REQUIRE(missing_extract.error().code == libbsa::error_code::not_found);

    auto missing_extract_bytes = reader.extract_bytes(fixture.expected_missing_path);
    REQUIRE_FALSE(missing_extract_bytes.has_value());
    REQUIRE(missing_extract_bytes.error().code == libbsa::error_code::not_found);

    collecting_sink invalid_sink;
    auto invalid_extract = reader.extract(fixture.invalid_archive_path, invalid_sink);
    REQUIRE_FALSE(invalid_extract.has_value());
    REQUIRE(invalid_extract.error().code == libbsa::error_code::invalid_argument);

    auto invalid_extract_bytes = reader.extract_bytes(fixture.invalid_archive_path);
    REQUIRE_FALSE(invalid_extract_bytes.has_value());
    REQUIRE(invalid_extract_bytes.error().code == libbsa::error_code::invalid_argument);

    recording_sink_factory sink_factory;
    std::vector requests{libbsa::bulk_extract_request{.path = fixture.expected_extract_path},
                         libbsa::bulk_extract_request{.path = fixture.expected_extract_path},
                         libbsa::bulk_extract_request{.path = fixture.expected_missing_path},
                         libbsa::bulk_extract_request{.path = fixture.invalid_archive_path}};

    auto extracted_entries = reader.extract_entries(requests, sink_factory);
    REQUIRE(extracted_entries.has_value());
    REQUIRE(extracted_entries.value().size() == requests.size());

    const auto report = sink_factory.report();
    REQUIRE(report.create_count_by_path.at(fixture.expected_extract_path) == 1U);
    REQUIRE(report.create_count_by_path.contains(fixture.expected_missing_path) == false);
    REQUIRE(report.create_count_by_path.contains(fixture.invalid_archive_path) == false);

    for (const auto result_index : {0U, 1U})
    {
      const auto &result = extracted_entries.value().at(result_index);
      REQUIRE(result.succeeded());
      require_result_entry_matches_expected(result, expected_entry);
    }

    REQUIRE(extracted_entries.value().at(2).failure.has_value());
    REQUIRE(extracted_entries.value().at(2).failure->code == libbsa::error_code::not_found);
    REQUIRE_FALSE(extracted_entries.value().at(2).entry.has_value());

    REQUIRE(extracted_entries.value().at(3).failure.has_value());
    REQUIRE(extracted_entries.value().at(3).failure->code == libbsa::error_code::invalid_argument);
    REQUIRE_FALSE(extracted_entries.value().at(3).entry.has_value());

    const auto &sink_bytes = report.sink_bytes_by_path.at(fixture.expected_extract_path);
    REQUIRE(sink_bytes.size() == 1U);
    if (expected_entry.contains("expected_dds_payload"))
    {
      REQUIRE(sink_bytes.front().size() > expected_bytes.size());
      REQUIRE(std::vector<std::byte>{sink_bytes.front().end() - static_cast<std::ptrdiff_t>(expected_bytes.size()),
                                     sink_bytes.front().end()} == expected_bytes);
    }
    else
    {
      REQUIRE(sink_bytes.front() == expected_bytes);
    }
  }
}
