#include <catch2/catch_test_macros.hpp>

#include <libbsa/bsa.hpp>
#include <libbsa/bsa_writer.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::detail {

[[nodiscard]] std::uint64_t hash_tes3_path(std::string_view path);
[[nodiscard]] std::uint64_t hash_tes4_path(std::string_view path);

} // namespace libbsa::detail

namespace {

constexpr std::uint32_t bsa_magic = 0x00415342;
constexpr std::uint32_t version_tes3 = 0x00000100;
constexpr std::uint32_t version_tes4 = 0x67;
constexpr std::uint32_t version_fo3 = 0x68;
constexpr std::uint32_t version_sse = 0x69;
constexpr std::uint32_t archive_pathnames = 0x0001;
constexpr std::uint32_t archive_filenames = 0x0002;
constexpr std::uint32_t archive_compress = 0x0004;
constexpr std::uint32_t archive_embedname = 0x0100;
constexpr std::uint32_t file_meshes = 0x0001;
constexpr std::uint32_t file_textures = 0x0002;
constexpr std::uint32_t file_menus = 0x0004;
constexpr std::uint32_t file_sounds = 0x0008;
constexpr std::uint32_t file_size_compress = 0x40000000;
constexpr std::size_t tes3_header_size = 12;
constexpr std::size_t tes3_file_record_size = 8;
constexpr std::size_t tes3_name_offset_size = 4;
constexpr std::size_t tes3_hash_size = 8;

std::uint32_t le_u32(std::span<const std::byte> bytes, std::size_t offset)
{
    return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset])) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 1])) << 8U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 2])) << 16U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 3])) << 24U);
}

std::uint64_t le_u64(std::span<const std::byte> bytes, std::size_t offset)
{
    std::uint64_t value = 0;
    for (int shift = 0; shift < 64; shift += 8) {
        value |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes[offset + static_cast<std::size_t>(shift / 8)]))
                 << static_cast<unsigned>(shift);
    }
    return value;
}

std::uint64_t tes3_reference_hash_word_order(std::span<const std::byte> bytes, std::size_t offset)
{
    const auto high = static_cast<std::uint64_t>(le_u32(bytes, offset));
    const auto low = static_cast<std::uint64_t>(le_u32(bytes, offset + 4U));
    return (high << 32U) | low;
}

std::uint32_t tes3_hash_offset(std::span<const std::byte> bytes)
{
    return le_u32(bytes, 4);
}

std::size_t tes3_hash_table_offset(std::span<const std::byte> bytes)
{
    return tes3_header_size + tes3_hash_offset(bytes);
}

std::size_t tes3_data_section_offset(std::span<const std::byte> bytes)
{
    const auto count = le_u32(bytes, 8);
    return tes3_hash_table_offset(bytes) + (static_cast<std::size_t>(count) * tes3_hash_size);
}

std::vector<libbsa::bsa_memory_entry> tes4_family_entries()
{
    libbsa::bsa_memory_entry mesh{};
    mesh.path = "meshes/a.nif";
    mesh.payload = {std::byte{0x6d}, std::byte{0x65}, std::byte{0x73}, std::byte{0x68}};
    mesh.compression = libbsa::compression_policy::force_raw;

    libbsa::bsa_memory_entry texture{};
    texture.path = "textures/t.dds";
    texture.payload = {std::byte{0x44}, std::byte{0x44}, std::byte{0x53}, std::byte{0x20}, std::byte{0x01}};
    texture.compression = libbsa::compression_policy::force_raw;

    libbsa::bsa_memory_entry sound{};
    sound.path = "sounds/s.wav";
    sound.payload = {std::byte{0x52}, std::byte{0x49}, std::byte{0x46}, std::byte{0x46}};
    sound.compression = libbsa::compression_policy::force_raw;

    return {texture, sound, mesh};
}

std::uint32_t expected_version(libbsa::bsa_write_target target)
{
    switch (target) {
    case libbsa::bsa_write_target::oblivion_v103:
        return version_tes4;
    case libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104:
        return version_fo3;
    case libbsa::bsa_write_target::skyrim_se_ae_v105:
        return version_sse;
    case libbsa::bsa_write_target::tes3_morrowind:
        break;
    }
    return 0;
}

std::vector<std::byte> finalize_to_bytes(const libbsa::bsa_write_plan& plan)
{
    libbsa::memory_sink sink;
    const auto finalized = libbsa::finalize_bsa_write(plan, sink);
    REQUIRE(finalized.has_value());
    return sink.bytes();
}

const libbsa::planned_bsa_table_region& require_region(const libbsa::bsa_write_plan& plan, std::string_view name)
{
    const auto found = std::find_if(plan.table_regions.begin(), plan.table_regions.end(), [name](const auto& region) {
        return region.name == name;
    });
    REQUIRE(found != plan.table_regions.end());
    return *found;
}

std::vector<std::byte> extract_bytes(const std::vector<std::byte>& bytes, std::string path)
{
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    auto archive = libbsa::open_bsa(source);
    REQUIRE(archive.has_value());
    libbsa::memory_sink sink;
    auto extracted = libbsa::extract_bsa_entry(archive.value(), source, std::move(path), sink);
    REQUIRE(extracted.has_value());
    return sink.bytes();
}

const libbsa::planned_bsa_entry& find_planned_entry(const libbsa::bsa_write_plan& plan, std::string_view path)
{
    const auto found = std::find_if(plan.entries.begin(), plan.entries.end(), [path](const auto& entry) {
        return entry.path == path;
    });
    REQUIRE(found != plan.entries.end());
    return *found;
}

std::uint32_t raw_size_field_for(const libbsa::bsa_write_plan& plan, std::string_view path)
{
    const auto& entry = find_planned_entry(plan, path);
    const auto bytes = std::span<const std::byte>{plan.table_bytes};
    for (std::size_t offset = 0; offset + 16U <= bytes.size(); ++offset) {
        if (le_u64(bytes, offset) == entry.file_hash && le_u32(bytes, offset + 12U) == entry.offset) {
            return le_u32(bytes, offset + 8U);
        }
    }
    FAIL("file record not found for planned BSA entry");
    return 0;
}

std::vector<std::byte> ascii_bytes(std::string_view text)
{
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (char ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

libbsa::bsa_memory_entry bsa_entry(std::string path,
                                   std::vector<std::byte> payload,
                                   libbsa::compression_policy compression = libbsa::compression_policy::archive_default)
{
    libbsa::bsa_memory_entry entry{};
    entry.path = std::move(path);
    entry.payload = std::move(payload);
    entry.compression = compression;
    return entry;
}

std::vector<libbsa::bsa_memory_entry> tes3_entries_unordered()
{
    return {bsa_entry("textures/tx_sand.dds", ascii_bytes("sand"), libbsa::compression_policy::force_raw),
            bsa_entry("meshes/marker.nif", ascii_bytes("mesh"), libbsa::compression_policy::force_raw),
            bsa_entry("icons/marker.tga", ascii_bytes("icon"), libbsa::compression_policy::force_raw)};
}

std::vector<std::string> tes3_paths_sorted_by_hash(std::span<const libbsa::bsa_memory_entry> entries)
{
    std::vector<std::string> paths;
    paths.reserve(entries.size());
    for (const auto& entry : entries) {
        paths.push_back(entry.path);
    }
    std::sort(paths.begin(), paths.end(), [](const auto& left, const auto& right) {
        const auto left_hash = libbsa::detail::hash_tes3_path(left);
        const auto right_hash = libbsa::detail::hash_tes3_path(right);
        if (left_hash != right_hash) {
            return left_hash < right_hash;
        }
        return left < right;
    });
    return paths;
}

libbsa::bsa_disk_entry disk_entry(std::filesystem::path host_path,
                                  std::string path,
                                  libbsa::compression_policy compression = libbsa::compression_policy::archive_default)
{
    libbsa::bsa_disk_entry entry{};
    entry.host_path = host_path.string();
    entry.path = std::move(path);
    entry.compression = compression;
    return entry;
}

std::filesystem::path unique_temp_file(std::string_view stem)
{
    static int counter = 0;
    return std::filesystem::temp_directory_path() /
           (std::string{"libbsa-bsa-writer-"} + std::string{stem} + "-" + std::to_string(++counter) + ".bin");
}

void write_temp_file(const std::filesystem::path& path, std::span<const std::byte> bytes)
{
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    REQUIRE(output.good());
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
}

class failing_sink final : public libbsa::byte_sink {
public:
    explicit failing_sink(std::size_t fail_after) : fail_after_(fail_after) {}

    [[nodiscard]] libbsa::result<void> write(std::span<const std::byte> bytes) override
    {
        if (written_ + bytes.size() > fail_after_) {
            return libbsa::failure<void>({libbsa::error_code::io_failure, "injected BSA sink failure"});
        }

        written_ += bytes.size();
        return libbsa::success();
    }

private:
    std::size_t fail_after_{};
    std::size_t written_{};
};

void require_archives_equivalent(std::span<const std::byte> memory_bytes,
                                 std::span<const std::byte> disk_bytes,
                                 std::span<const libbsa::bsa_memory_entry> entries)
{
    const libbsa::memory_source memory_source{memory_bytes};
    const libbsa::memory_source disk_source{disk_bytes};
    const auto memory_archive = libbsa::open_bsa(memory_source);
    const auto disk_archive = libbsa::open_bsa(disk_source);
    REQUIRE(memory_archive.has_value());
    REQUIRE(disk_archive.has_value());
    CHECK(memory_archive.value().paths() == disk_archive.value().paths());

    for (const auto& entry : entries) {
        const auto memory_metadata = memory_archive.value().entry(entry.path);
        const auto disk_metadata = disk_archive.value().entry(entry.path);
        REQUIRE(memory_metadata.has_value());
        REQUIRE(disk_metadata.has_value());
        CHECK(memory_metadata.value().path == disk_metadata.value().path);
        CHECK(memory_metadata.value().size == disk_metadata.value().size);
        CHECK(memory_metadata.value().stored_size == disk_metadata.value().stored_size);
        CHECK(memory_metadata.value().compression == disk_metadata.value().compression);
        CHECK(memory_metadata.value().directory_hash == disk_metadata.value().directory_hash);
        CHECK(memory_metadata.value().name_hash == disk_metadata.value().name_hash);
        CHECK(extract_bytes(std::vector<std::byte>{memory_bytes.begin(), memory_bytes.end()}, entry.path) == entry.payload);
        CHECK(extract_bytes(std::vector<std::byte>{disk_bytes.begin(), disk_bytes.end()}, entry.path) == entry.payload);
    }
}

} // namespace

TEST_CASE("plans and finalizes TES4-family raw BSA archives for every required version", "[unit][bsa-writer][roundtrip]")
{
    const auto entries = tes4_family_entries();

    for (const auto target : {libbsa::bsa_write_target::oblivion_v103,
                              libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                              libbsa::bsa_write_target::skyrim_se_ae_v105}) {
        const auto plan = libbsa::plan_bsa_write(target, std::span<const libbsa::bsa_memory_entry>{entries});
        REQUIRE(plan.has_value());

        const auto bytes = finalize_to_bytes(plan.value());
        REQUIRE(bytes.size() == plan.value().total_size);
        REQUIRE(bytes.size() >= 36);
        CHECK(le_u32(bytes, 0) == bsa_magic);
        CHECK(le_u32(bytes, 4) == expected_version(target));
        CHECK(le_u32(bytes, 8) == 36);
        CHECK(le_u32(bytes, 16) == 3);
        CHECK(le_u32(bytes, 20) == 3);
        CHECK(le_u32(bytes, 24) == 26);
        CHECK(le_u32(bytes, 32) == (file_meshes | file_textures | file_sounds));

        const libbsa::memory_source source{std::span<const std::byte>{bytes}};
        const auto archive = libbsa::open_bsa(source);
        REQUIRE(archive.has_value());
        CHECK(archive.value().summary().version == expected_version(target));
        CHECK(archive.value().summary().folder_count == 3);
        CHECK(archive.value().summary().file_count == 3);

        for (const auto& entry : entries) {
            const auto normalized = entry.path;
            const auto metadata = archive.value().entry(normalized);
            REQUIRE(metadata.has_value());
            const auto& planned = find_planned_entry(plan.value(), normalized);
            CHECK(metadata.value().directory_hash == planned.folder_hash);
            CHECK(metadata.value().name_hash == planned.file_hash);
            CHECK(metadata.value().offset == planned.offset);
            CHECK(metadata.value().stored_size == planned.stored_size);
            CHECK(metadata.value().compression == libbsa::compression_state::raw);
            CHECK(extract_bytes(bytes, normalized) == entry.payload);
        }
    }
}

TEST_CASE("writes TES3 BSA records in TES3-compatible hash sorting order", "[unit][bsa-writer][tes3]")
{
    const auto entries = tes3_entries_unordered();
    const auto expected_paths = tes3_paths_sorted_by_hash(std::span<const libbsa::bsa_memory_entry>{entries});

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::tes3_morrowind,
                                            std::span<const libbsa::bsa_memory_entry>{entries});

    REQUIRE(plan.has_value());
    REQUIRE(plan.value().entries.size() == expected_paths.size());
    for (std::size_t i = 0; i < expected_paths.size(); ++i) {
        CHECK(plan.value().entries[i].path == expected_paths[i]);
        CHECK(plan.value().entries[i].file_hash == libbsa::detail::hash_tes3_path(expected_paths[i]));
    }
}

TEST_CASE("serializes TES3 name offsets hash table and data-section-relative offsets", "[unit][bsa-writer][tes3]")
{
    const auto entries = tes3_entries_unordered();
    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::tes3_morrowind,
                                            std::span<const libbsa::bsa_memory_entry>{entries});
    REQUIRE(plan.has_value());
    const auto bytes = finalize_to_bytes(plan.value());

    CHECK(le_u32(bytes, 0) == version_tes3);
    CHECK(le_u32(bytes, 8) == entries.size());
    const auto hash_table_offset = tes3_hash_table_offset(bytes);
    const auto data_section_offset = tes3_data_section_offset(bytes);
    CHECK(hash_table_offset < data_section_offset);
    REQUIRE(bytes.size() == plan.value().total_size);

    std::size_t expected_name_offset = 0;
    for (std::size_t i = 0; i < plan.value().entries.size(); ++i) {
        const auto record_offset = tes3_header_size + (i * tes3_file_record_size);
        const auto name_offset_offset = tes3_header_size + (plan.value().entries.size() * tes3_file_record_size) +
                                        (i * tes3_name_offset_size);
        const auto relative_offset = le_u32(bytes, record_offset + 4U);
        CHECK(le_u32(bytes, name_offset_offset) == expected_name_offset);
        CHECK(tes3_reference_hash_word_order(bytes, hash_table_offset + (i * tes3_hash_size)) == plan.value().entries[i].file_hash);
        CHECK(plan.value().entries[i].offset == data_section_offset + relative_offset);
        expected_name_offset += plan.value().entries[i].path.size() + 1U;
    }
}

TEST_CASE("round trips TES3 raw payloads through open_bsa and extract_bsa_entry", "[unit][bsa-writer][tes3][roundtrip]")
{
    const auto entries = tes3_entries_unordered();
    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::tes3_morrowind,
                                            std::span<const libbsa::bsa_memory_entry>{entries});
    REQUIRE(plan.has_value());
    const auto bytes = finalize_to_bytes(plan.value());
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    const auto archive = libbsa::open_bsa(source);
    REQUIRE(archive.has_value());
    CHECK(archive.value().summary().format == libbsa::archive_format::tes3_bsa);

    for (const auto& entry : entries) {
        const auto metadata = archive.value().entry(entry.path);
        REQUIRE(metadata.has_value());
        const auto& planned = find_planned_entry(plan.value(), entry.path);
        CHECK(metadata.value().offset == planned.offset);
        CHECK(metadata.value().stored_size == entry.payload.size());
        CHECK(metadata.value().compression == libbsa::compression_state::raw);
        CHECK(extract_bytes(bytes, entry.path) == entry.payload);
    }
}

TEST_CASE("rejects TES3 compression requests with unsupported_format", "[unit][bsa-writer][tes3][failure]")
{
    const std::vector entries{bsa_entry("meshes/packed.nif", ascii_bytes("packed"), libbsa::compression_policy::force_compressed)};

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::tes3_morrowind,
                                            std::span<const libbsa::bsa_memory_entry>{entries});

    REQUIRE_FALSE(plan.has_value());
    CHECK(plan.error().code == libbsa::error_code::unsupported_format);
}

TEST_CASE("rejects TES3 embedded-name requests with malformed_archive or unsupported_format", "[unit][bsa-writer][tes3][failure]")
{
    const std::vector entries{bsa_entry("meshes/plain.nif", ascii_bytes("plain"), libbsa::compression_policy::force_raw)};
    libbsa::bsa_write_options options{};
    options.embedded_names = true;

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::tes3_morrowind,
                                            std::span<const libbsa::bsa_memory_entry>{entries},
                                            options);

    REQUIRE_FALSE(plan.has_value());
    CHECK((plan.error().code == libbsa::error_code::malformed_archive || plan.error().code == libbsa::error_code::unsupported_format));
}

TEST_CASE("exposes native TES4 table regions and offsets before finalization", "[unit][bsa-writer]")
{
    const auto entries = tes4_family_entries();
    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::skyrim_se_ae_v105,
                                            std::span<const libbsa::bsa_memory_entry>{entries});
    REQUIRE(plan.has_value());

    const auto& header = require_region(plan.value(), "tes4 header");
    const auto& folder_records = require_region(plan.value(), "tes4 folder records");
    const auto& folder_blocks = require_region(plan.value(), "tes4 folder blocks");
    const auto& file_names = require_region(plan.value(), "tes4 file names");

    CHECK(header.offset == 0);
    CHECK(header.size == 36);
    CHECK(folder_records.offset == 36);
    CHECK(folder_records.size == 72);
    CHECK(folder_blocks.offset == 108);
    CHECK(file_names.offset > folder_blocks.offset);
    REQUIRE(plan.value().data_regions.size() == 3);
    for (const auto& entry : plan.value().entries) {
        CHECK(entry.offset >= file_names.offset + file_names.size);
        CHECK(entry.offset == plan.value().data_regions[entry.data_region_id].offset);
    }
}

TEST_CASE("computes TES4 FileFlags from entry extensions", "[unit][bsa-writer]")
{
    std::vector entries = tes4_family_entries();
    libbsa::bsa_memory_entry menu{};
    menu.path = "menus/main.xml";
    menu.payload = {std::byte{0x3c}, std::byte{0x78}, std::byte{0x2f}};
    menu.compression = libbsa::compression_policy::force_raw;
    entries.push_back(std::move(menu));

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                                            std::span<const libbsa::bsa_memory_entry>{entries});
    REQUIRE(plan.has_value());
    const auto bytes = finalize_to_bytes(plan.value());

    CHECK(le_u32(bytes, 32) == (file_meshes | file_textures | file_sounds | file_menus));
}

TEST_CASE("orders TES4 folders and files by reference hashes independent of input order", "[unit][bsa-writer]")
{
    std::vector first_order = tes4_family_entries();
    std::vector second_order = first_order;
    std::reverse(second_order.begin(), second_order.end());

    const auto first = libbsa::plan_bsa_write(libbsa::bsa_write_target::oblivion_v103,
                                             std::span<const libbsa::bsa_memory_entry>{first_order});
    const auto second = libbsa::plan_bsa_write(libbsa::bsa_write_target::oblivion_v103,
                                              std::span<const libbsa::bsa_memory_entry>{second_order});
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    CHECK(finalize_to_bytes(first.value()) == finalize_to_bytes(second.value()));

    const auto bytes = finalize_to_bytes(first.value());
    const auto first_folder_record = std::size_t{36};
    const auto second_folder_record = first_folder_record + 16;
    const auto third_folder_record = second_folder_record + 16;
    const std::vector folder_hashes{le_u64(bytes, first_folder_record),
                                    le_u64(bytes, second_folder_record),
                                    le_u64(bytes, third_folder_record)};
    CHECK(std::is_sorted(folder_hashes.begin(), folder_hashes.end()));
    CHECK(find_planned_entry(first.value(), "meshes/a.nif").file_hash == libbsa::detail::hash_tes4_path("a.nif"));
    CHECK(find_planned_entry(first.value(), "textures/t.dds").file_hash == libbsa::detail::hash_tes4_path("t.dds"));
    CHECK(find_planned_entry(first.value(), "sounds/s.wav").file_hash == libbsa::detail::hash_tes4_path("s.wav"));
    CHECK((le_u32(bytes, 12) & (archive_pathnames | archive_filenames | archive_compress)) == (archive_pathnames | archive_filenames));
    CHECK((le_u32(bytes, 36 + 8) == 1));
}

TEST_CASE("archive-default compressed TES4 entries use XOR size flags", "[unit][bsa-writer][codec]")
{
    const std::vector entries{bsa_entry("meshes/armor/iron.nif", ascii_bytes("mesh payload that compresses"))};
    libbsa::bsa_write_options options{};
    options.archive_default_compressed = true;

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::oblivion_v103,
                                            std::span<const libbsa::bsa_memory_entry>{entries},
                                            options);

    REQUIRE(plan.has_value());
    CHECK((plan.value().flags & archive_compress) == archive_compress);
    CHECK((raw_size_field_for(plan.value(), "meshes/armor/iron.nif") & file_size_compress) == 0U);
    CHECK(find_planned_entry(plan.value(), "meshes/armor/iron.nif").compression == libbsa::compression_state::deflate);
    CHECK(extract_bytes(finalize_to_bytes(plan.value()), "meshes/armor/iron.nif") == entries.front().payload);
}

TEST_CASE("force raw and force compressed override archive defaults", "[unit][bsa-writer][codec]")
{
    const std::vector entries{bsa_entry("meshes/raw.nif", ascii_bytes("raw bytes"), libbsa::compression_policy::force_raw),
                              bsa_entry("meshes/packed.nif", ascii_bytes("packed bytes packed bytes"),
                                        libbsa::compression_policy::force_compressed)};
    libbsa::bsa_write_options options{};
    options.archive_default_compressed = true;

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                                            std::span<const libbsa::bsa_memory_entry>{entries},
                                            options);

    REQUIRE(plan.has_value());
    CHECK((raw_size_field_for(plan.value(), "meshes/raw.nif") & file_size_compress) == file_size_compress);
    CHECK((raw_size_field_for(plan.value(), "meshes/packed.nif") & file_size_compress) == 0U);
    CHECK(find_planned_entry(plan.value(), "meshes/raw.nif").compression == libbsa::compression_state::raw);
    CHECK(find_planned_entry(plan.value(), "meshes/packed.nif").compression == libbsa::compression_state::deflate);
    const auto bytes = finalize_to_bytes(plan.value());
    CHECK(extract_bytes(bytes, "meshes/raw.nif") == entries[0].payload);
    CHECK(extract_bytes(bytes, "meshes/packed.nif") == entries[1].payload);
}

TEST_CASE("v105 compressed BSA entries use LZ4-frame not deflate", "[unit][bsa-writer][codec]")
{
    const std::vector entries{bsa_entry("meshes/armor/iron.nif", ascii_bytes("SSE payload uses LZ4-frame"),
                                        libbsa::compression_policy::force_compressed)};

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::skyrim_se_ae_v105,
                                            std::span<const libbsa::bsa_memory_entry>{entries});

    REQUIRE(plan.has_value());
    CHECK(find_planned_entry(plan.value(), "meshes/armor/iron.nif").compression == libbsa::compression_state::lz4_frame);
    CHECK(extract_bytes(finalize_to_bytes(plan.value()), "meshes/armor/iron.nif") == entries.front().payload);
}

TEST_CASE("embedded names are opt-in and extract after prefix skip", "[unit][bsa-writer][roundtrip]")
{
    const std::vector entries{bsa_entry("meshes/armor/iron.nif", ascii_bytes("embedded payload"),
                                        libbsa::compression_policy::force_raw)};

    const auto without_embed = libbsa::plan_bsa_write(libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                                                     std::span<const libbsa::bsa_memory_entry>{entries});
    libbsa::bsa_write_options options{};
    options.embedded_names = true;
    const auto with_embed = libbsa::plan_bsa_write(libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                                                  std::span<const libbsa::bsa_memory_entry>{entries},
                                                  options);

    REQUIRE(without_embed.has_value());
    REQUIRE(with_embed.has_value());
    CHECK((without_embed.value().flags & archive_embedname) == 0U);
    CHECK((with_embed.value().flags & archive_embedname) == archive_embedname);
    CHECK(extract_bytes(finalize_to_bytes(with_embed.value()), "meshes/armor/iron.nif") == entries.front().payload);
}

TEST_CASE("embedded names serialize exact archive path prefix bytes", "[unit][bsa-writer]")
{
    const std::vector entries{bsa_entry("meshes/armor/iron.nif", ascii_bytes("mesh"), libbsa::compression_policy::force_raw)};
    libbsa::bsa_write_options options{};
    options.embedded_names = true;

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::skyrim_se_ae_v105,
                                            std::span<const libbsa::bsa_memory_entry>{entries},
                                            options);

    REQUIRE(plan.has_value());
    const auto& region = plan.value().data_regions[find_planned_entry(plan.value(), "meshes/armor/iron.nif").data_region_id];
    const std::string expected_name = "meshes\\armor\\iron.nif";
    REQUIRE(region.stored_payload.size() > expected_name.size());
    CHECK(std::to_integer<unsigned char>(region.stored_payload[0]) == expected_name.size());
    CHECK(std::vector<std::byte>{region.stored_payload.begin() + 1, region.stored_payload.begin() + 1 + expected_name.size()} ==
          ascii_bytes(expected_name));
}

TEST_CASE("unsupported compression requests fail during planning with unsupported_format", "[unit][bsa-writer]")
{
    const std::vector entries{bsa_entry("meshes/armor/iron.nif", ascii_bytes("mesh"),
                                        libbsa::compression_policy::force_compressed)};
    libbsa::memory_sink sink;

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::tes3_morrowind,
                                            std::span<const libbsa::bsa_memory_entry>{entries});

    REQUIRE_FALSE(plan.has_value());
    CHECK(plan.error().code == libbsa::error_code::unsupported_format);
    CHECK(sink.bytes().empty());
}

TEST_CASE("malformed embedded-name requests fail during planning with malformed_archive", "[unit][bsa-writer]")
{
    std::string long_folder(260, 'a');
    const std::vector entries{bsa_entry("meshes/" + long_folder + "/iron.nif", ascii_bytes("mesh"),
                                        libbsa::compression_policy::force_raw)};
    libbsa::bsa_write_options options{};
    options.embedded_names = true;
    libbsa::memory_sink sink;

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                                            std::span<const libbsa::bsa_memory_entry>{entries},
                                            options);

    REQUIRE_FALSE(plan.has_value());
    CHECK(plan.error().code == libbsa::error_code::malformed_archive);
    CHECK(sink.bytes().empty());
}

TEST_CASE("dedup shares only identical native BSA stored payloads", "[unit][bsa-writer]")
{
    const auto shared_payload = ascii_bytes("same source bytes");
    const std::vector entries{bsa_entry("meshes/a.nif", shared_payload, libbsa::compression_policy::force_raw),
                              bsa_entry("textures/a.dds", shared_payload, libbsa::compression_policy::force_raw),
                              bsa_entry("sounds/a.wav", ascii_bytes("different bytes"), libbsa::compression_policy::force_raw)};
    libbsa::bsa_write_options options{};
    options.deduplicate = true;

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::oblivion_v103,
                                            std::span<const libbsa::bsa_memory_entry>{entries},
                                            options);

    REQUIRE(plan.has_value());
    const auto& first = find_planned_entry(plan.value(), "meshes/a.nif");
    const auto& second = find_planned_entry(plan.value(), "textures/a.dds");
    const auto& different = find_planned_entry(plan.value(), "sounds/a.wav");
    CHECK(first.data_region_id == second.data_region_id);
    CHECK(first.offset == second.offset);
    CHECK(first.data_region_id != different.data_region_id);
    CHECK(plan.value().data_regions.size() == 2);
}

TEST_CASE("disk-backed and memory-backed BSA inputs read back equivalently", "[unit][bsa-writer][disk][roundtrip]")
{
    const std::vector memory_entries{bsa_entry("meshes/from-disk.nif", ascii_bytes("disk mesh"), libbsa::compression_policy::force_raw),
                                     bsa_entry("textures/from-disk.dds", ascii_bytes("disk texture"), libbsa::compression_policy::force_raw)};
    const auto mesh_path = unique_temp_file("mesh");
    const auto texture_path = unique_temp_file("texture");
    write_temp_file(mesh_path, memory_entries[0].payload);
    write_temp_file(texture_path, memory_entries[1].payload);
    const std::vector disk_entries{disk_entry(mesh_path, memory_entries[0].path, memory_entries[0].compression),
                                   disk_entry(texture_path, memory_entries[1].path, memory_entries[1].compression)};

    const auto memory_plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                                                   std::span<const libbsa::bsa_memory_entry>{memory_entries});
    const auto disk_plan = libbsa::plan_bsa_write_from_disk(libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                                                           std::span<const libbsa::bsa_disk_entry>{disk_entries});

    REQUIRE(memory_plan.has_value());
    REQUIRE(disk_plan.has_value());
    require_archives_equivalent(finalize_to_bytes(memory_plan.value()), finalize_to_bytes(disk_plan.value()), memory_entries);
}

TEST_CASE("disk files are read during planning and not reopened during finalization", "[unit][bsa-writer][disk]")
{
    const auto host_path = unique_temp_file("owned-by-plan");
    const auto original = ascii_bytes("original disk bytes");
    write_temp_file(host_path, original);
    const std::vector disk_entries{disk_entry(host_path, "meshes/owned.nif", libbsa::compression_policy::force_raw)};

    const auto plan = libbsa::plan_bsa_write_from_disk(libbsa::bsa_write_target::oblivion_v103,
                                                       std::span<const libbsa::bsa_disk_entry>{disk_entries});
    REQUIRE(plan.has_value());
    write_temp_file(host_path, ascii_bytes("mutated bytes that must not leak into finalization"));
    std::filesystem::remove(host_path);

    const auto bytes = finalize_to_bytes(plan.value());

    CHECK(extract_bytes(bytes, "meshes/owned.nif") == original);
}

TEST_CASE("duplicate normalized BSA paths fail during planning", "[unit][bsa-writer][failure]")
{
    const std::vector entries{bsa_entry("Meshes/Armor/Iron.NIF", ascii_bytes("first")),
                              bsa_entry("meshes\\armor\\iron.nif", ascii_bytes("second"))};
    libbsa::memory_sink sink;

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::skyrim_se_ae_v105,
                                             std::span<const libbsa::bsa_memory_entry>{entries});

    REQUIRE_FALSE(plan.has_value());
    CHECK(plan.error().code == libbsa::error_code::malformed_archive);
    CHECK(sink.bytes().empty());
}

TEST_CASE("duplicate normalized disk BSA paths fail before host file reads", "[unit][bsa-writer][disk][failure]")
{
    const auto existing_path = unique_temp_file("duplicate-existing");
    const auto missing_path = unique_temp_file("duplicate-missing");
    write_temp_file(existing_path, ascii_bytes("first"));
    std::filesystem::remove(missing_path);
    const std::vector entries{disk_entry(existing_path, "Meshes/Armor/Iron.NIF"),
                              disk_entry(missing_path, "meshes\\armor\\iron.nif")};
    libbsa::memory_sink sink;

    const auto plan = libbsa::plan_bsa_write_from_disk(libbsa::bsa_write_target::oblivion_v103,
                                                       std::span<const libbsa::bsa_disk_entry>{entries});

    REQUIRE_FALSE(plan.has_value());
    CHECK(plan.error().code == libbsa::error_code::malformed_archive);
    CHECK(sink.bytes().empty());
}

TEST_CASE("invalid archive paths fail during planning", "[unit][bsa-writer][failure]")
{
    const std::vector entries{bsa_entry("", ascii_bytes("payload"))};
    libbsa::memory_sink sink;

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::oblivion_v103,
                                             std::span<const libbsa::bsa_memory_entry>{entries});

    REQUIRE_FALSE(plan.has_value());
    CHECK(plan.error().code == libbsa::error_code::malformed_archive);
    CHECK(sink.bytes().empty());
}

TEST_CASE("missing disk inputs fail before finalization", "[unit][bsa-writer][disk][failure]")
{
    const auto missing_path = unique_temp_file("missing");
    std::filesystem::remove(missing_path);
    const std::vector disk_entries{disk_entry(missing_path, "meshes/missing.nif", libbsa::compression_policy::force_raw)};
    libbsa::memory_sink sink;

    const auto plan = libbsa::plan_bsa_write_from_disk(libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                                                       std::span<const libbsa::bsa_disk_entry>{disk_entries});

    REQUIRE_FALSE(plan.has_value());
    CHECK(plan.error().code == libbsa::error_code::io_failure);
    CHECK(sink.bytes().empty());
}

TEST_CASE("impossible BSA layout arithmetic fails before finalization", "[unit][bsa-writer][failure]")
{
    const std::string folder_too_large_for_len8(260, 'a');
    const std::vector entries{bsa_entry("meshes/" + folder_too_large_for_len8 + "/iron.nif", ascii_bytes("mesh"),
                                        libbsa::compression_policy::force_raw)};
    libbsa::memory_sink sink;

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                                             std::span<const libbsa::bsa_memory_entry>{entries});

    REQUIRE_FALSE(plan.has_value());
    CHECK(plan.error().code == libbsa::error_code::malformed_archive);
    CHECK(sink.bytes().empty());
}

TEST_CASE("unsupported BSA target version combinations fail during planning", "[unit][bsa-writer][failure]")
{
    const std::vector entries{bsa_entry("meshes/packed.nif", ascii_bytes("payload"),
                                        libbsa::compression_policy::force_compressed)};
    libbsa::memory_sink sink;

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::tes3_morrowind,
                                             std::span<const libbsa::bsa_memory_entry>{entries});

    REQUIRE_FALSE(plan.has_value());
    CHECK(plan.error().code == libbsa::error_code::unsupported_format);
    CHECK(sink.bytes().empty());
}

TEST_CASE("BSA finalization returns first sink failure unchanged", "[unit][bsa-writer][failure]")
{
    const std::vector entries{bsa_entry("meshes/a.nif", ascii_bytes("payload"), libbsa::compression_policy::force_raw)};
    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::oblivion_v103,
                                             std::span<const libbsa::bsa_memory_entry>{entries});
    REQUIRE(plan.has_value());
    failing_sink sink{8};

    const auto finalized = libbsa::finalize_bsa_write(plan.value(), sink);

    REQUIRE_FALSE(finalized.has_value());
    CHECK(finalized.error().code == libbsa::error_code::io_failure);
    CHECK(finalized.error().message == "injected BSA sink failure");
}
