#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

std::filesystem::path generated_archive_dir() {
    return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" /
           "archives";
}

std::filesystem::path generated_archive_path(std::string_view filename) {
    return generated_archive_dir() / std::string{filename};
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
    std::ifstream stream{path, std::ios::binary};
    return nlohmann::json::parse(stream);
}

libbsa::error_code error_code_from_manifest(std::string_view value) {
    if (value == "format_error") {
        return libbsa::error_code::format_error;
    }
    if (value == "unsupported") {
        return libbsa::error_code::unsupported;
    }
    FAIL("unknown BA2 DX10 malformed expected_error: " << value);
    return libbsa::error_code::format_error;
}

class collecting_sink final : public libbsa::payload_sink {
   public:
    /// Captures extracted bytes when malformed DX10 cases are expected to fail
    /// during extraction.
    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
        return bytes.size();
    }

   private:
    std::vector<std::byte> bytes_;
};

}  // namespace

TEST_CASE("ba2_dx10_malformed manifest cases fail with stable errors",
          "[unit][fixture][malformed][ba2_dx10_malformed]") {
    const auto manifest =
        read_json_file(generated_archive_path("ba2_dx10_malformed_manifest.json"));
    std::vector<std::string> observed_cases;

    for (const auto& test_case : manifest.at("cases")) {
        const auto id = test_case.at("id").get<std::string>();
        observed_cases.push_back(id);
        const auto archive =
            generated_archive_path(test_case.at("archive").get<std::string>()).string();
        const auto expected_error =
            error_code_from_manifest(test_case.at("expected_error").get<std::string>());
        const auto phase = test_case.at("phase").get<std::string>();

        INFO("BA2 DX10 malformed case: " << id);
        if (phase == "open") {
            auto opened = libbsa::archive_reader::open(archive);

            REQUIRE_FALSE(opened.has_value());
            REQUIRE(opened.error().code == expected_error);
            continue;
        }

        REQUIRE(phase == "extraction");
        auto opened = libbsa::archive_reader::open(archive);
        REQUIRE(opened.has_value());
        collecting_sink sink;

        auto extracted =
            opened.value().extract(test_case.at("target_path").get<std::string>(), sink);

        REQUIRE_FALSE(extracted.has_value());
        REQUIRE(extracted.error().code == expected_error);
    }

    for (const auto required :
         {"ba2_dx10_truncated_header", "ba2_dx10_truncated_records", "ba2_dx10_truncated_chunks",
          "ba2_dx10_invalid_payload_span", "ba2_dx10_inconsistent_chunk_sizes",
          "ba2_dx10_bad_compressed_chunk", "ba2_dx10_decoded_size_mismatch", "ba2_dx10_mip_gap",
          "ba2_dx10_duplicate_mip_face", "ba2_dx10_duplicate_canonical_path",
          "ba2_dx10_unsupported_compression"}) {
        INFO("required malformed BA2 DX10 case: " << required);
        REQUIRE(std::find(observed_cases.begin(), observed_cases.end(), required) !=
                observed_cases.end());
    }
}
