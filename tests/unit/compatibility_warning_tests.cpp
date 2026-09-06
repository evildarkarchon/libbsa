#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

std::filesystem::path warning_test_dir() {
    auto path = std::filesystem::temp_directory_path() / "libbsa_compatibility_warning_tests";
    std::filesystem::create_directories(path);
    return path;
}

std::filesystem::path source_root() { return std::filesystem::path{LIBBSA_SOURCE_DIR}; }

std::filesystem::path unique_output_path(std::string_view stem, std::string_view extension) {
    static std::atomic_uint64_t counter{0};
    auto path =
        warning_test_dir() /
        (std::string{stem} + "-" + std::to_string(counter.fetch_add(1, std::memory_order_relaxed)) +
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

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream stream{path};
    REQUIRE(stream.is_open());

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

std::string trim_copy(std::string value) {
    const auto first = std::find_if(value.begin(), value.end(),
                                    [](unsigned char ch) { return !std::isspace(ch); });
    const auto last = std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
                          return !std::isspace(ch);
                      }).base();

    if (first >= last) {
        return {};
    }
    return std::string{first, last};
}

std::vector<std::string> compatibility_warning_codes_from_public_header() {
    const auto header = read_text_file(source_root() / "include/libbsa/validation.hpp");
    const auto enum_name = std::string{"enum class compatibility_warning_code"};
    const auto enum_start = header.find(enum_name);
    REQUIRE(enum_start != std::string::npos);

    const auto body_start = header.find('{', enum_start);
    REQUIRE(body_start != std::string::npos);
    const auto body_end = header.find("};", body_start);
    REQUIRE(body_end != std::string::npos);

    std::vector<std::string> codes;
    std::istringstream lines{header.substr(body_start + 1, body_end - body_start - 1)};
    std::string line;
    while (std::getline(lines, line)) {
        if (const auto comment = line.find("//"); comment != std::string::npos) {
            line.erase(comment);
        }
        if (const auto comma = line.find(','); comma != std::string::npos) {
            line.erase(comma);
        }

        auto code = trim_copy(line);
        if (!code.empty()) {
            codes.push_back(std::move(code));
        }
    }
    return codes;
}

std::string warning_code_name(libbsa::compatibility_warning_code code) {
    switch (code) {
        case libbsa::compatibility_warning_code::compressed_sound_payload:
            return "compressed_sound_payload";
        case libbsa::compatibility_warning_code::bsa_embedded_name_compatibility_risk:
            return "bsa_embedded_name_compatibility_risk";
        case libbsa::compatibility_warning_code::target_family_mismatch:
            return "target_family_mismatch";
        case libbsa::compatibility_warning_code::ba2_record_identity_mismatch:
            return "ba2_record_identity_mismatch";
        case libbsa::compatibility_warning_code::bsa_file_name_table_trailing_bytes:
            return "bsa_file_name_table_trailing_bytes";
        case libbsa::compatibility_warning_code::bsa_folder_name_table_length_mismatch:
            return "bsa_folder_name_table_length_mismatch";
    }

    FAIL("unknown compatibility_warning_code");
    return {};
}

const libbsa::compatibility_warning& require_warning(
    const libbsa::validation_report& report, libbsa::compatibility_warning_code code,
    libbsa::compatibility_warning_severity severity, bool has_archive_path) {
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
    REQUIRE(
        writer
            .add_bytes("Meshes/Mismatch/Probe.nif", bytes_from_text("ba2 target mismatch payload"))
            .has_value());
    REQUIRE(writer.write_to(output.string()).has_value());
    return output;
}

/// Writes a writer-output BA2 GNRL archive whose first record's stored NameHash
/// disagrees with its own filename-table path.
///
/// The disagreement is produced by byte-patching writer output rather than by a
/// committed archive so the evidence stays reproducible from repository sources
/// alone. Retail archives such as `Fallout4 - Voices.ba2` carry the same
/// condition (issue #43), but their bytes are not redistributable.
std::filesystem::path write_record_identity_mismatch_ba2_archive() {
    const auto output = unique_output_path("record-identity-mismatch", ".ba2");
    libbsa::ba2_gnrl_writer_options options;
    options.compression = libbsa::archive_compression_policy::all_raw;
    options.overwrite_existing = true;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
    REQUIRE(writer
                .add_bytes("Meshes/Unreachable/Probe.nif",
                           bytes_from_text("record identity mismatch payload"))
                .has_value());
    REQUIRE(writer.write_to(output.string()).has_value());

    // The Fallout 4 BA2 header is 24 bytes and a GNRL record leads with its
    // UInt32 NameHash, so record 0's hash starts at offset 24.
    constexpr std::size_t first_record_name_hash_offset = 24U;
    std::fstream patch{output, std::ios::binary | std::ios::in | std::ios::out};
    REQUIRE(patch.is_open());
    patch.seekg(static_cast<std::streamoff>(first_record_name_hash_offset));
    std::array<char, 4U> stored{};
    patch.read(stored.data(), static_cast<std::streamsize>(stored.size()));
    REQUIRE(patch.gcount() == static_cast<std::streamsize>(stored.size()));
    stored[0] = static_cast<char>(static_cast<unsigned char>(stored[0]) ^ 0x10U);
    patch.seekp(static_cast<std::streamoff>(first_record_name_hash_offset));
    patch.write(stored.data(), static_cast<std::streamsize>(stored.size()));
    REQUIRE(patch.good());
    patch.close();

    return output;
}

std::filesystem::path write_embedded_name_bsa_archive() {
    const auto output = unique_output_path("embedded-name-risk", ".bsa");
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    options.embed_file_names = true;
    options.overwrite_existing = true;
    libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
    REQUIRE(writer
                .add_bytes("Meshes/Embedded/Model.nif",
                           bytes_from_text("embedded name compatibility payload"))
                .has_value());
    REQUIRE(writer.write_to(output.string()).has_value());
    return output;
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
    std::ifstream stream{path, std::ios::binary};
    REQUIRE(stream.is_open());

    const std::string text{std::istreambuf_iterator<char>{stream},
                           std::istreambuf_iterator<char>{}};
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

std::uint32_t read_u32_le(std::span<const std::byte> bytes, std::size_t offset) {
    REQUIRE(offset + 4U <= bytes.size());
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < 4U; ++index) {
        value |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + index]))
                 << (index * 8U);
    }
    return value;
}

void write_u32_le(std::span<std::byte> bytes, std::size_t offset, std::uint32_t value) {
    REQUIRE(offset + 4U <= bytes.size());
    for (std::size_t index = 0; index < 4U; ++index) {
        bytes[offset + index] = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
    }
}

/// Writes a writer-output TES4-family BSA whose file-name table declares more
/// bytes than its names consume.
///
/// Retail `Fallout - Voices1.bsa` ships this condition -- 105 declared bytes
/// past the last of its 105,517 names, all NUL (issue #45) -- but retail bytes
/// are not redistributable, so the evidence is reconstructed by byte-patching
/// writer output instead. Growing the table means every archive-absolute offset
/// past it moves, so the patch rewrites the folder-record and file-record offset
/// fields as well as `TotalFileNameLength`. Folder-record offsets are stored
/// biased by `TotalFileNameLength` (`wbBSArchive.pas:1491`), which shifts them by
/// the same amount, so one delta covers both.
std::filesystem::path write_trailing_file_name_bytes_bsa_archive() {
    const auto output = unique_output_path("trailing-file-name-bytes", ".bsa");
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    options.overwrite_existing = true;
    libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
    REQUIRE(writer
                .add_bytes("Meshes/Trailing/Probe.nif",
                           bytes_from_text("trailing file-name table payload"))
                .has_value());
    REQUIRE(writer.write_to(output.string()).has_value());

    auto bytes = read_binary_file(output);

    // Fixed TES4 header: magic, version, FoldersOffset, ArchiveFlags,
    // FolderCount, FileCount, TotalFolderNameLength, TotalFileNameLength,
    // FileFlags.
    constexpr std::size_t header_size = 36U;
    constexpr std::size_t folder_count_offset = 16U;
    constexpr std::size_t file_count_offset = 20U;
    constexpr std::size_t total_folder_name_length_offset = 24U;
    constexpr std::size_t total_file_name_length_offset = 28U;
    // Legacy (non-SSE) folder record: Hash u64, FileCount u32, Offset u32.
    constexpr std::size_t folder_record_size = 16U;
    constexpr std::size_t folder_record_offset_field = 12U;
    // File record: Hash u64, Size u32, Offset u32.
    constexpr std::size_t file_record_size = 16U;
    constexpr std::size_t file_record_offset_field = 12U;
    constexpr std::uint32_t trailing_bytes = 105U;

    const auto folder_count = read_u32_le(bytes, folder_count_offset);
    const auto file_count = read_u32_le(bytes, file_count_offset);
    const auto total_folder_name_length = read_u32_le(bytes, total_folder_name_length_offset);
    const auto total_file_name_length = read_u32_le(bytes, total_file_name_length_offset);

    write_u32_le(bytes, total_file_name_length_offset, total_file_name_length + trailing_bytes);

    const std::size_t folder_records_start = header_size;
    for (std::uint32_t index = 0; index < folder_count; ++index) {
        const auto field =
            folder_records_start + (index * folder_record_size) + folder_record_offset_field;
        write_u32_le(bytes, field, read_u32_le(bytes, field) + trailing_bytes);
    }

    // The folder-name block is one byte per folder wider than
    // TotalFolderNameLength, which counts the bzstring without its length prefix.
    const std::size_t folder_blocks_start =
        folder_records_start + (static_cast<std::size_t>(folder_count) * folder_record_size);
    const std::size_t folder_blocks_size = total_folder_name_length + folder_count +
                                           (static_cast<std::size_t>(file_count) * file_record_size);
    std::size_t cursor = folder_blocks_start;
    for (std::uint32_t index = 0; index < folder_count; ++index) {
        const auto name_size = std::to_integer<std::uint8_t>(bytes[cursor]);
        cursor += 1U + name_size;
        const auto folder_file_count =
            read_u32_le(bytes, folder_records_start + (index * folder_record_size) + 8U);
        for (std::uint32_t file_index = 0; file_index < folder_file_count; ++file_index) {
            write_u32_le(bytes, cursor + file_record_offset_field,
                         read_u32_le(bytes, cursor + file_record_offset_field) + trailing_bytes);
            cursor += file_record_size;
        }
    }
    REQUIRE(cursor == folder_blocks_start + folder_blocks_size);

    // NUL to match what retail archives actually pad with. The parser does not
    // require it: it stops after FileCount names, so any surplus byte is slack
    // whatever its value, exactly as the reference's FileCount sequential
    // ReadStringTerm calls leave it.
    const std::size_t names_end = cursor + total_file_name_length;
    bytes.insert(bytes.begin() + static_cast<std::ptrdiff_t>(names_end), trailing_bytes,
                 std::byte{0});

    std::ofstream out{output, std::ios::binary | std::ios::trunc};
    REQUIRE(out.is_open());
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    REQUIRE(out.good());
    return output;
}

/// Writes a writer-output TES4-family BSA whose header declares a folder-name
/// table length that disagrees with the folder names it stores.
///
/// This is a one-field patch, unlike the file-name slack fixture: nothing in the
/// layout is located by `TotalFolderNameLength`, so inflating it moves no byte
/// and shifts no offset. That is precisely the claim -- an archive carrying a
/// wrong value is still completely readable, and the reference never reads the
/// field back at all.
std::filesystem::path write_folder_name_length_mismatch_bsa_archive() {
    const auto output = unique_output_path("folder-name-length-mismatch", ".bsa");
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    options.overwrite_existing = true;
    libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
    REQUIRE(writer
                .add_bytes("Meshes/Mismatched/Probe.nif",
                           bytes_from_text("folder name length mismatch payload"))
                .has_value());
    REQUIRE(writer.write_to(output.string()).has_value());

    auto bytes = read_binary_file(output);

    // TotalFolderNameLength sits at offset 24 in the fixed TES4 header. Inflating
    // it is the direction that used to be doubly fatal: it failed the total
    // cross-check, and it pushed the derived payload/metadata boundary past legal
    // payload bytes.
    constexpr std::size_t total_folder_name_length_offset = 24U;
    constexpr std::uint32_t inflated_length = 4096U;
    write_u32_le(bytes, total_folder_name_length_offset, inflated_length);

    std::ofstream out{output, std::ios::binary | std::ios::trunc};
    REQUIRE(out.is_open());
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    REQUIRE(out.good());
    return output;
}

std::filesystem::path write_compressed_sound_bsa_archive() {
    const auto output = unique_output_path("compressed-sound-risk", ".bsa");
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_compressed;
    options.overwrite_existing = true;
    libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
    REQUIRE(writer.add_bytes("sound/fx/alert.wav", repeated_text_bytes("alert sound payload ", 16U))
                .has_value());
    REQUIRE(writer.write_to(output.string()).has_value());
    return output;
}

}  // namespace

TEST_CASE("compatibility_warning reports BA2 target family mismatch",
          "[unit][compat][compatibility_warning]") {
    libbsa::validation_options options;
    options.expected_type = libbsa::archive_type::bsa;

    const auto report = require_validated_report(write_raw_ba2_archive(), options);

    require_warning(report, libbsa::compatibility_warning_code::target_family_mismatch,
                    libbsa::compatibility_warning_severity::risky, false);
}

TEST_CASE("compatibility_warning reports BSA embedded name compatibility risk",
          "[unit][compat][compatibility_warning]") {
    const auto report = require_validated_report(write_embedded_name_bsa_archive());

    require_warning(report,
                    libbsa::compatibility_warning_code::bsa_embedded_name_compatibility_risk,
                    libbsa::compatibility_warning_severity::risky, true);
}

TEST_CASE("compatibility_warning reports compressed sound payloads",
          "[unit][compat][compatibility_warning]") {
    const auto report = require_validated_report(write_compressed_sound_bsa_archive());

    require_warning(report, libbsa::compatibility_warning_code::compressed_sound_payload,
                    libbsa::compatibility_warning_severity::advisory, true);
}

TEST_CASE("compatibility_warning reports BA2 record identity mismatch",
          "[unit][compat][compatibility_warning]") {
    const auto archive = write_record_identity_mismatch_ba2_archive();
    const auto report = require_validated_report(archive);

    const auto& warning =
        require_warning(report, libbsa::compatibility_warning_code::ba2_record_identity_mismatch,
                        libbsa::compatibility_warning_severity::risky, true);
    CHECK(warning.archive_path == "meshes/unreachable/probe.nif");

    // The archive stays open-able and the entry stays extractable by path; only
    // Bethesda-style lookup by recomputed hash cannot reach it.
    auto opened = libbsa::archive_reader::open(archive.string());
    REQUIRE(opened.has_value());
    auto extracted = opened.value().extract_bytes("Meshes/Unreachable/Probe.nif");
    REQUIRE(extracted.has_value());
    CHECK(extracted.value() == bytes_from_text("record identity mismatch payload"));
}

TEST_CASE("compatibility_warning reports a file-name table with trailing bytes",
          "[unit][compat][compatibility_warning]") {
    const auto archive = write_trailing_file_name_bytes_bsa_archive();
    const auto report = require_validated_report(archive);

    // Archive-wide condition: the surplus bytes belong to no entry, so unlike the
    // record-identity warning this one carries no archive path.
    require_warning(report,
                    libbsa::compatibility_warning_code::bsa_file_name_table_trailing_bytes,
                    libbsa::compatibility_warning_severity::advisory, false);
    REQUIRE(report.metadata.has_value());
    CHECK(report.metadata.value().file_name_table_has_trailing_bytes);

    // The slack must not swallow or invent an entry, and the archive must stay
    // fully listable and extractable.
    auto opened = libbsa::archive_reader::open(archive.string());
    REQUIRE(opened.has_value());
    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());
    REQUIRE(entries.value().size() == 1U);
    auto extracted = opened.value().extract_bytes("Meshes/Trailing/Probe.nif");
    REQUIRE(extracted.has_value());
    CHECK(extracted.value() == bytes_from_text("trailing file-name table payload"));
}

TEST_CASE("compatibility_warning reports a declared folder-name length that disagrees",
          "[unit][compat][compatibility_warning]") {
    const auto archive = write_folder_name_length_mismatch_bsa_archive();
    const auto report = require_validated_report(archive);

    require_warning(report,
                    libbsa::compatibility_warning_code::bsa_folder_name_table_length_mismatch,
                    libbsa::compatibility_warning_severity::advisory, false);
    REQUIRE(report.metadata.has_value());
    CHECK(report.metadata.value().folder_name_table_length_mismatch);

    // The payload sits immediately after the true metadata table. Deriving the
    // table size by walking is what keeps it extractable: sizing from the inflated
    // header field would place it inside a phantom metadata region and reject it.
    auto opened = libbsa::archive_reader::open(archive.string());
    REQUIRE(opened.has_value());
    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());
    REQUIRE(entries.value().size() == 1U);
    auto extracted = opened.value().extract_bytes("Meshes/Mismatched/Probe.nif");
    REQUIRE(extracted.has_value());
    CHECK(extracted.value() == bytes_from_text("folder name length mismatch payload"));
}

TEST_CASE("compatibility_warning behavior covers every public warning code",
          "[unit][compat][compatibility_warning][validation_policy]") {
    libbsa::validation_options mismatch_options;
    mismatch_options.expected_type = libbsa::archive_type::bsa;

    const std::array reports{
        require_validated_report(write_raw_ba2_archive(), mismatch_options),
        require_validated_report(write_embedded_name_bsa_archive()),
        require_validated_report(write_compressed_sound_bsa_archive()),
        require_validated_report(write_record_identity_mismatch_ba2_archive()),
        require_validated_report(write_trailing_file_name_bytes_bsa_archive()),
        require_validated_report(write_folder_name_length_mismatch_bsa_archive()),
    };

    std::vector<std::string> observed_codes;
    for (const auto& report : reports) {
        for (const auto& warning : report.warnings) {
            observed_codes.push_back(warning_code_name(warning.code));
        }
    }

    const auto public_codes = compatibility_warning_codes_from_public_header();
    REQUIRE_FALSE(public_codes.empty());
    for (const auto& public_code : public_codes) {
        INFO("missing behavior-backed warning code: " << public_code);
        REQUIRE(std::find(observed_codes.begin(), observed_codes.end(), public_code) !=
                observed_codes.end());
    }
}
