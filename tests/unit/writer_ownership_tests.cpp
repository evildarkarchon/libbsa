#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

static_assert(!std::is_copy_constructible_v<libbsa::tes3_bsa_writer>);
static_assert(!std::is_copy_assignable_v<libbsa::tes3_bsa_writer>);
static_assert(std::is_move_constructible_v<libbsa::tes3_bsa_writer>);
static_assert(std::is_move_assignable_v<libbsa::tes3_bsa_writer>);

static_assert(!std::is_copy_constructible_v<libbsa::tes4_bsa_writer>);
static_assert(!std::is_copy_assignable_v<libbsa::tes4_bsa_writer>);
static_assert(std::is_move_constructible_v<libbsa::tes4_bsa_writer>);
static_assert(std::is_move_assignable_v<libbsa::tes4_bsa_writer>);

static_assert(!std::is_copy_constructible_v<libbsa::ba2_gnrl_writer>);
static_assert(!std::is_copy_assignable_v<libbsa::ba2_gnrl_writer>);
static_assert(std::is_move_constructible_v<libbsa::ba2_gnrl_writer>);
static_assert(std::is_move_assignable_v<libbsa::ba2_gnrl_writer>);

static_assert(!std::is_copy_constructible_v<libbsa::ba2_dx10_writer>);
static_assert(!std::is_copy_assignable_v<libbsa::ba2_dx10_writer>);
static_assert(std::is_move_constructible_v<libbsa::ba2_dx10_writer>);
static_assert(std::is_move_assignable_v<libbsa::ba2_dx10_writer>);

namespace {

std::filesystem::path writer_test_dir() {
    auto path = std::filesystem::temp_directory_path() / "libbsa_writer_ownership_tests";
    std::filesystem::create_directories(path);
    return path;
}

std::filesystem::path unique_output_path(std::string_view name) {
    static std::atomic_uint64_t counter{0};
    auto path = writer_test_dir() / std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
    std::filesystem::create_directories(path);
    return path / std::string{name};
}

std::filesystem::path generated_source_dir() {
    return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "source";
}

std::vector<std::byte> bytes_from_text(std::string_view text) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

void require_extracted_bytes(const libbsa::archive_reader& reader, std::string_view path,
                             std::span<const std::byte> expected) {
    auto extracted = reader.extract_bytes(path);
    REQUIRE(extracted.has_value());
    CHECK(std::vector<std::byte>{expected.begin(), expected.end()} == extracted.value());
}

}  // namespace

TEST_CASE("TES3 writer move construction transfers staged entries", "[unit][writer_ownership]") {
    const auto payload = bytes_from_text("tes3 moved payload");
    libbsa::tes3_bsa_writer_options options;
    options.overwrite_existing = true;
    libbsa::tes3_bsa_writer writer{options};
    REQUIRE(writer.add_bytes("Meshes/Moved.nif", payload).has_value());

    libbsa::tes3_bsa_writer moved{std::move(writer)};
    const auto output = unique_output_path("tes3-move-constructed.bsa");
    REQUIRE(moved.write_to(output.string()).has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    require_extracted_bytes(opened.value(), "Meshes/Moved.nif", payload);
}

TEST_CASE("TES4 writer move assignment transfers staged entries", "[unit][writer_ownership]") {
    const auto payload = bytes_from_text("tes4 moved payload");
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    options.overwrite_existing = true;
    libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
    REQUIRE(writer.add_bytes("Meshes/Moved.nif", payload).has_value());

    libbsa::tes4_bsa_writer moved{libbsa::tes4_bsa_target::oblivion};
    moved = std::move(writer);
    const auto output = unique_output_path("tes4-move-assigned.bsa");
    REQUIRE(moved.write_to(output.string()).has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    require_extracted_bytes(opened.value(), "Meshes/Moved.nif", payload);
}

TEST_CASE("BA2 GNRL writer move construction transfers staged entries",
          "[unit][writer_ownership]") {
    const auto payload = bytes_from_text("ba2 gnrl moved payload");
    libbsa::ba2_gnrl_writer_options options;
    options.compression = libbsa::archive_compression_policy::all_raw;
    options.overwrite_existing = true;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
    REQUIRE(writer.add_bytes("Meshes/Moved.nif", payload).has_value());

    libbsa::ba2_gnrl_writer moved{std::move(writer)};
    const auto output = unique_output_path("ba2-gnrl-move-constructed.ba2");
    REQUIRE(moved.write_to(output.string()).has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    require_extracted_bytes(opened.value(), "Meshes/Moved.nif", payload);
}

TEST_CASE("BA2 DX10 writer move assignment transfers snapshot-backed entries",
          "[unit][writer_ownership]") {
    const auto source = generated_source_dir() / "ba2_dx10_bc1_unorm.dds";
    libbsa::ba2_dx10_writer_options options;
    options.overwrite_existing = true;
    libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4, options};
    REQUIRE(writer.add_file("textures/moved.dds", source.string()).has_value());

    libbsa::ba2_dx10_writer moved{libbsa::ba2_dx10_target::fallout4};
    moved = std::move(writer);
    const auto output = unique_output_path("ba2-dx10-move-assigned.ba2");
    REQUIRE(moved.write_to(output.string()).has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    auto found = opened.value().find("textures/moved.dds");
    REQUIRE(found.has_value());
    REQUIRE(found.value().has_value());
    REQUIRE(found.value()->texture.has_value());
    auto extracted = opened.value().extract_bytes("textures/moved.dds");
    REQUIRE(extracted.has_value());
    CHECK_FALSE(extracted.value().empty());
}
