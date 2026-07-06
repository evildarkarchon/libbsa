#include <libbsa/libbsa.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t dds_dxt10_header_size = 148U;
constexpr std::uint32_t dds_fourcc_dx10 = 0x30315844U;
constexpr std::uint32_t dds_caps_texture = 0x00001000U;
constexpr std::uint32_t dds_resource_dimension_texture2d = 3U;
constexpr std::uint32_t dxgi_format_bc1_unorm = 71U;

class byte_vector_sink final : public libbsa::payload_sink {
   public:
    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
        return bytes.size();
    }

    const std::vector<std::byte>& bytes() const noexcept { return bytes_; }

   private:
    std::vector<std::byte> bytes_;
};

class byte_vector_sink_factory final : public libbsa::bulk_extract_sink_factory {
   public:
    libbsa::result<std::unique_ptr<libbsa::payload_sink>> create(
        std::string_view path, const libbsa::entry_metadata& entry) override {
        // Bulk extraction may call the factory concurrently, so even example
        // bookkeeping is protected.
        const std::lock_guard lock{mutex_};
        requested_paths_.push_back(std::string{path});
        observed_entries_.push_back(entry.path);
        std::unique_ptr<libbsa::payload_sink> sink = std::make_unique<byte_vector_sink>();
        return sink;
    }

    const std::vector<std::string>& requested_paths() const noexcept { return requested_paths_; }

    const std::vector<std::string>& observed_entries() const noexcept { return observed_entries_; }

   private:
    std::mutex mutex_;
    std::vector<std::string> requested_paths_;
    std::vector<std::string> observed_entries_;
};

struct byte_buffer {
    std::vector<std::byte> bytes;

    void u8(std::uint8_t value) { bytes.push_back(static_cast<std::byte>(value)); }

    void u32(std::uint32_t value) {
        for (std::uint32_t index = 0; index < 4U; ++index) {
            u8(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
        }
    }

    void ascii4(std::string_view value) {
        for (const char ch : value) {
            u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch)));
        }
    }

    void raw(std::span<const std::byte> values) {
        bytes.insert(bytes.end(), values.begin(), values.end());
    }
};

struct archive_runtime_case {
    std::string_view label;
    std::string_view archive_host_path;
    std::string_view archive_virtual_path;
    libbsa::archive_type expected_type;
    libbsa::archive_variant expected_variant;
    libbsa::entry_compression expected_default_compression;
    std::uint32_t expected_version;
    std::optional<std::uint32_t> expected_ba2_compression_method;
    const std::vector<std::byte>* expected_payload;
    bool expect_texture_metadata;
};

libbsa::result<void> fail(libbsa::error_code code, std::string message) {
    return libbsa::error{code, std::move(message)};
}

std::string host_path_string(const std::filesystem::path& path) {
    const auto utf8 = path.u8string();
    return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
}

std::vector<std::byte> bytes_from_text(std::string_view text) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

std::vector<std::byte> repeated_bytes(std::uint8_t seed, std::size_t count) {
    std::vector<std::byte> result;
    result.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        result.push_back(static_cast<std::byte>(static_cast<std::uint8_t>(seed + index)));
    }
    return result;
}

libbsa::result<void> write_binary_file(const std::filesystem::path& path,
                                       std::span<const std::byte> values) {
    std::error_code fs_error;
    std::filesystem::create_directories(path.parent_path(), fs_error);
    if (fs_error) {
        return fail(libbsa::error_code::io_error,
                    "failed to create package-consumer source directory: " + fs_error.message());
    }

    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    if (!output.good()) {
        return fail(libbsa::error_code::io_error,
                    "failed to open package-consumer source file: " + path.string());
    }

    output.write(reinterpret_cast<const char*>(values.data()),
                 static_cast<std::streamsize>(values.size()));
    if (!output.good()) {
        return fail(libbsa::error_code::io_error,
                    "failed to write package-consumer source file: " + path.string());
    }

    return {};
}

std::vector<std::byte> build_tiny_bc1_dds_dxt10_source() {
    byte_buffer writer;
    writer.ascii4("DDS ");
    for (const auto value : {124U, 0x0002100FU, 16U, 16U, 0U, 0U, 1U}) {
        writer.u32(value);
    }
    for (std::uint32_t index = 0; index < 11U; ++index) {
        writer.u32(0U);
    }

    // The package consumer owns this tiny DXT10 DDS shape so the
    // installed-package smoke never reaches into source-tree fixture directories
    // at runtime.
    for (const auto value : {32U, 0x00000004U, dds_fourcc_dx10, 0U, 0U, 0U, 0U, 0U,
                             dds_caps_texture, 0U, 0U, 0U, 0U}) {
        writer.u32(value);
    }
    for (const auto value : {dxgi_format_bc1_unorm, dds_resource_dimension_texture2d, 0U, 1U, 0U}) {
        writer.u32(value);
    }

    const auto payload = repeated_bytes(0x52U, 16U * 8U);
    writer.raw(std::span<const std::byte>{payload.data(), payload.size()});
    return writer.bytes;
}

libbsa::result<void> recreate_work_directory(const std::filesystem::path& work_dir) {
    std::error_code fs_error;
    std::filesystem::remove_all(work_dir, fs_error);
    if (fs_error) {
        return fail(libbsa::error_code::io_error,
                    "failed to remove package-consumer work directory: " + fs_error.message());
    }

    std::filesystem::create_directories(work_dir, fs_error);
    if (fs_error) {
        return fail(libbsa::error_code::io_error,
                    "failed to create package-consumer work directory: " + fs_error.message());
    }

    return {};
}

libbsa::result<void> example_open_list_extract(std::string_view archive_host_path,
                                               std::string_view archive_virtual_path,
                                               libbsa::payload_sink& sink) {
    auto opened = libbsa::archive_reader::open(archive_host_path);
    if (!opened) {
        return opened.error();
    }

    auto reader = std::move(opened).value();
    auto metadata = reader.metadata();
    if (!metadata) {
        return metadata.error();
    }

    auto entries = reader.entries();
    if (!entries) {
        return entries.error();
    }

    auto found = reader.find(archive_virtual_path);
    if (!found) {
        return found.error();
    }
    if (!found.value()) {
        return libbsa::error{libbsa::error_code::not_found, "archive path is not present"};
    }

    auto present = reader.contains(archive_virtual_path);
    if (!present) {
        return present.error();
    }
    if (!present.value()) {
        return libbsa::error{libbsa::error_code::not_found, "archive path is not present"};
    }

    auto buffered_payload = reader.extract_bytes(archive_virtual_path);
    if (!buffered_payload) {
        return buffered_payload.error();
    }
    [[maybe_unused]] const auto buffered_payload_size = buffered_payload.value().size();

    return reader.extract(archive_virtual_path, sink);
}

libbsa::result<std::vector<libbsa::bulk_extract_entry_result>> example_bulk_extract(
    std::string_view archive_host_path, std::span<const std::string> archive_virtual_paths,
    libbsa::bulk_extract_sink_factory& sink_factory) {
    auto opened = libbsa::archive_reader::open(archive_host_path);
    if (!opened) {
        return opened.error();
    }

    std::vector<libbsa::bulk_extract_request> requests;
    requests.reserve(archive_virtual_paths.size());
    for (const auto& archive_virtual_path : archive_virtual_paths) {
        requests.push_back(libbsa::bulk_extract_request{archive_virtual_path});
    }

    libbsa::bulk_extract_options options;
    options.worker_count = 2U;

    auto reader = std::move(opened).value();
    return reader.extract_entries(requests, sink_factory, options);
}

libbsa::result<void> example_create_tes3_bsa(std::string_view source_host_path,
                                             std::string_view output_host_path) {
    libbsa::tes3_bsa_writer_options options;
    options.overwrite_existing = true;

    libbsa::tes3_bsa_writer writer{options};
    if (auto added = writer.add_file("meshes/package/tes3.nif", source_host_path); !added) {
        return added.error();
    }

    libbsa::write_execution_options execution;
    execution.worker_count = 1U;
    return writer.write_to(output_host_path, execution);
}

libbsa::result<void> example_create_tes4_bsa(std::string_view source_host_path,
                                             std::string_view output_host_path) {
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::target_default;
    options.overwrite_existing = true;

    libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::skyrim_se, options};
    if (auto added = writer.add_file("meshes/example/example.nif", source_host_path,
                                     libbsa::entry_compression_policy::inherit);
        !added) {
        return added.error();
    }

    libbsa::write_execution_options execution;
    execution.worker_count = 2U;
    return writer.write_to(output_host_path, execution);
}

libbsa::result<void> example_create_ba2_gnrl(std::string_view source_host_path,
                                             std::string_view output_host_path) {
    libbsa::ba2_gnrl_writer_options options;
    options.compression = libbsa::archive_compression_policy::target_default;
    options.overwrite_existing = true;
    options.starfield_compression_method = 3U;

    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::starfield_v3, options};
    if (auto added = writer.add_file("scripts/example/example.pex", source_host_path,
                                     libbsa::entry_compression_policy::compressed);
        !added) {
        return added.error();
    }

    libbsa::write_execution_options execution;
    execution.worker_count = 2U;
    return writer.write_to(output_host_path, execution);
}

libbsa::result<void> example_create_ba2_dx10(std::string_view dds_host_path,
                                             std::string_view output_host_path) {
    libbsa::ba2_dx10_writer_options options;
    options.overwrite_existing = true;
    options.starfield_compression_method = 3U;

    libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::starfield_v3, options};
    if (auto added = writer.add_file("textures/example/example_d.dds", dds_host_path); !added) {
        return added.error();
    }

    libbsa::write_execution_options execution;
    execution.worker_count = 2U;
    return writer.write_to(output_host_path, execution);
}

int example_handle_result_errors(std::string_view archive_host_path) {
    auto opened = libbsa::archive_reader::open(archive_host_path);
    if (opened) {
        return 0;
    }

    const auto code = opened.error().code;
    if (code == libbsa::error_code::io_error) {
        return 10;
    }
    if (code == libbsa::error_code::format_error) {
        return 20;
    }
    if (code == libbsa::error_code::unsupported) {
        return 30;
    }
    return 1;
}

libbsa::result<libbsa::validation_report> example_validate_archive(
    std::string_view archive_host_path, libbsa::archive_type expected_type) {
    libbsa::validation_options options;
    options.expected_type = expected_type;
    options.validate_entry_extractability = true;

    auto report = libbsa::validate_archive(archive_host_path, options);
    if (!report) {
        return report.error();
    }

    for (const auto& warning : report.value().warnings) {
        if (warning.code == libbsa::compatibility_warning_code::compressed_sound_payload) {
            break;
        }
    }

    return report;
}

libbsa::result<void> verify_archive_runtime_case(const archive_runtime_case& test_case) {
    byte_vector_sink sink;
    if (auto extracted = example_open_list_extract(test_case.archive_host_path,
                                                   test_case.archive_virtual_path, sink);
        !extracted) {
        return extracted.error();
    }

    auto opened = libbsa::archive_reader::open(test_case.archive_host_path);
    if (!opened) {
        return opened.error();
    }

    auto reader = std::move(opened).value();
    auto metadata = reader.metadata();
    if (!metadata) {
        return metadata.error();
    }
    if (metadata.value().type != test_case.expected_type ||
        metadata.value().variant != test_case.expected_variant) {
        return fail(libbsa::error_code::format_error,
                    std::string{test_case.label} + " metadata family mismatch");
    }
    if (metadata.value().version != test_case.expected_version) {
        return fail(libbsa::error_code::format_error,
                    std::string{test_case.label} + " metadata version mismatch");
    }
    if (metadata.value().file_count != 1U) {
        return fail(libbsa::error_code::format_error,
                    std::string{test_case.label} + " metadata file count mismatch");
    }
    if (metadata.value().default_compression != test_case.expected_default_compression) {
        return fail(libbsa::error_code::format_error,
                    std::string{test_case.label} + " metadata compression mismatch");
    }
    if (test_case.expected_ba2_compression_method.has_value()) {
        if (!metadata.value().ba2.has_value() ||
            !metadata.value().ba2->compression_method.has_value() ||
            metadata.value().ba2->compression_method.value() !=
                test_case.expected_ba2_compression_method.value()) {
            return fail(libbsa::error_code::format_error,
                        std::string{test_case.label} + " BA2 compression method mismatch");
        }
    }

    auto entries = reader.entries();
    if (!entries) {
        return entries.error();
    }
    if (entries.value().size() != 1U) {
        return fail(libbsa::error_code::format_error,
                    std::string{test_case.label} + " entry listing mismatch");
    }

    auto found = reader.find(test_case.archive_virtual_path);
    if (!found) {
        return found.error();
    }
    if (!found.value()) {
        return fail(libbsa::error_code::not_found,
                    std::string{test_case.label} + " entry was not found");
    }

    auto contains = reader.contains(test_case.archive_virtual_path);
    if (!contains) {
        return contains.error();
    }
    if (!contains.value()) {
        return fail(libbsa::error_code::not_found,
                    std::string{test_case.label} + " entry was not contained");
    }

    auto extracted_bytes = reader.extract_bytes(test_case.archive_virtual_path);
    if (!extracted_bytes) {
        return extracted_bytes.error();
    }
    if (sink.bytes() != extracted_bytes.value()) {
        return fail(libbsa::error_code::format_error,
                    std::string{test_case.label} + " sink extraction did not match extract_bytes");
    }

    if (test_case.expected_payload != nullptr &&
        extracted_bytes.value() != *test_case.expected_payload) {
        return fail(libbsa::error_code::format_error,
                    std::string{test_case.label} + " extracted payload mismatch");
    }

    if (test_case.expect_texture_metadata) {
        if (extracted_bytes.value().size() <= dds_dxt10_header_size ||
            extracted_bytes.value()[0] != std::byte{0x44} ||
            extracted_bytes.value()[1] != std::byte{0x44} ||
            extracted_bytes.value()[2] != std::byte{0x53} ||
            extracted_bytes.value()[3] != std::byte{0x20}) {
            return fail(libbsa::error_code::format_error,
                        std::string{test_case.label} + " extracted DDS payload mismatch");
        }
        if (!found.value()->texture.has_value() || found.value()->texture->width != 16U ||
            found.value()->texture->height != 16U || found.value()->texture->mip_count != 1U ||
            found.value()->texture->dxgi_format != dxgi_format_bc1_unorm ||
            found.value()->texture->chunks.empty()) {
            return fail(libbsa::error_code::format_error,
                        std::string{test_case.label} + " texture metadata mismatch");
        }
    }

    byte_vector_sink_factory sink_factory;
    std::vector<std::string> paths{std::string{test_case.archive_virtual_path}};
    auto bulk = example_bulk_extract(test_case.archive_host_path,
                                     std::span<const std::string>{paths.data(), paths.size()},
                                     sink_factory);
    if (!bulk) {
        return bulk.error();
    }
    if (bulk.value().size() != 1U || !bulk.value().front().succeeded() ||
        sink_factory.requested_paths().size() != 1U ||
        sink_factory.observed_entries().size() != 1U) {
        return fail(libbsa::error_code::format_error,
                    std::string{test_case.label} + " bulk extraction mismatch");
    }

    auto validated = example_validate_archive(test_case.archive_host_path, test_case.expected_type);
    if (!validated) {
        return validated.error();
    }
    if (!validated.value().valid || !validated.value().is_valid() ||
        !validated.value().errors.empty() || !validated.value().metadata.has_value() ||
        validated.value().metadata->type != test_case.expected_type ||
        validated.value().metadata->variant != test_case.expected_variant) {
        return fail(libbsa::error_code::format_error,
                    std::string{test_case.label} + " validation report mismatch");
    }

    return {};
}

libbsa::result<void> run_installed_package_archive_runtime_smoke() {
    std::error_code fs_error;
    const auto work_dir =
        std::filesystem::current_path(fs_error) / "libbsa-package-consumer-smoke-work";
    if (fs_error) {
        return fail(libbsa::error_code::io_error,
                    "failed to resolve package-consumer current directory: " + fs_error.message());
    }
    if (auto recreated = recreate_work_directory(work_dir); !recreated) {
        return recreated.error();
    }

    const auto payload = bytes_from_text("libbsa installed package runtime smoke payload");
    const auto payload_source = work_dir / "payload.bin";
    if (auto written = write_binary_file(
            payload_source, std::span<const std::byte>{payload.data(), payload.size()});
        !written) {
        return written.error();
    }

    const auto dds_bytes = build_tiny_bc1_dds_dxt10_source();
    if (dds_bytes.size() <= dds_dxt10_header_size) {
        return fail(libbsa::error_code::format_error,
                    "internal package-consumer DDS source was not generated");
    }
    const auto dds_source = work_dir / "tiny-bc1.dds";
    if (auto written = write_binary_file(
            dds_source, std::span<const std::byte>{dds_bytes.data(), dds_bytes.size()});
        !written) {
        return written.error();
    }

    const auto payload_source_host_path = host_path_string(payload_source);
    const auto dds_source_host_path = host_path_string(dds_source);

    const auto tes3_archive = work_dir / "tes3-package-consumer.bsa";
    const auto tes4_archive = work_dir / "tes4-package-consumer.bsa";
    const auto ba2_gnrl_archive = work_dir / "ba2-gnrl-package-consumer.ba2";
    const auto ba2_dx10_archive = work_dir / "ba2-dx10-package-consumer.ba2";

    const auto tes3_archive_host_path = host_path_string(tes3_archive);
    const auto tes4_archive_host_path = host_path_string(tes4_archive);
    const auto ba2_gnrl_archive_host_path = host_path_string(ba2_gnrl_archive);
    const auto ba2_dx10_archive_host_path = host_path_string(ba2_dx10_archive);

    if (auto created = example_create_tes3_bsa(payload_source_host_path, tes3_archive_host_path);
        !created) {
        return created.error();
    }
    if (auto created = example_create_tes4_bsa(payload_source_host_path, tes4_archive_host_path);
        !created) {
        return created.error();
    }
    if (auto created =
            example_create_ba2_gnrl(payload_source_host_path, ba2_gnrl_archive_host_path);
        !created) {
        return created.error();
    }
    if (auto created = example_create_ba2_dx10(dds_source_host_path, ba2_dx10_archive_host_path);
        !created) {
        return created.error();
    }

    const std::array cases{
        archive_runtime_case{"TES3 BSA", tes3_archive_host_path, "meshes/package/tes3.nif",
                             libbsa::archive_type::bsa, libbsa::archive_variant::tes3,
                             libbsa::entry_compression::none, 0x00000100U, std::nullopt, &payload,
                             false},
        archive_runtime_case{"TES4-family BSA", tes4_archive_host_path,
                             "meshes/example/example.nif", libbsa::archive_type::bsa,
                             libbsa::archive_variant::tes4, libbsa::entry_compression::lz4_frame,
                             105U, std::nullopt, &payload, false},
        archive_runtime_case{"BA2 GNRL", ba2_gnrl_archive_host_path, "scripts/example/example.pex",
                             libbsa::archive_type::ba2, libbsa::archive_variant::starfield,
                             libbsa::entry_compression::lz4_block, 3U,
                             std::optional<std::uint32_t>{3U}, &payload, false},
        archive_runtime_case{"BA2 DX10", ba2_dx10_archive_host_path,
                             "textures/example/example_d.dds", libbsa::archive_type::ba2,
                             libbsa::archive_variant::starfield,
                             libbsa::entry_compression::lz4_block, 3U,
                             std::optional<std::uint32_t>{3U}, nullptr, true},
    };

    for (const auto& test_case : cases) {
        if (auto verified = verify_archive_runtime_case(test_case); !verified) {
            return verified.error();
        }
    }

    const auto missing_archive = host_path_string(work_dir / "missing-package-consumer.bsa");
    if (example_handle_result_errors(missing_archive) != 10) {
        return fail(libbsa::error_code::io_error,
                    "missing archive did not report io_error through open branch");
    }

    auto missing_validation = libbsa::validate_archive(missing_archive);
    if (missing_validation || missing_validation.error().code != libbsa::error_code::io_error) {
        return fail(libbsa::error_code::io_error,
                    "missing archive did not report io_error through validation branch");
    }

    return {};
}

int example_link_representative_public_api() {
    std::vector<std::byte> bytes{std::byte{0x41}};

    auto opened = libbsa::archive_reader::open("consumer-smoke-missing.bsa");
    if (opened) {
        return 1;
    }

    auto validated = libbsa::validate_archive("consumer-smoke-missing.bsa");
    if (validated) {
        return 1;
    }

    libbsa::tes3_bsa_writer tes3_writer;
    if (!tes3_writer.add_bytes("meshes/consumer/link.nif", bytes)) {
        return 1;
    }
    [[maybe_unused]] const auto& tes3_options = tes3_writer.options();

    libbsa::tes4_bsa_writer tes4_writer{libbsa::tes4_bsa_target::fallout3};
    if (!tes4_writer.add_bytes("meshes/consumer/link.nif", bytes)) {
        return 1;
    }
    [[maybe_unused]] const auto tes4_target = tes4_writer.target();

    libbsa::ba2_gnrl_writer ba2_gnrl_writer{libbsa::ba2_gnrl_target::fallout4};
    if (!ba2_gnrl_writer.add_bytes("meshes/consumer/link.nif", bytes)) {
        return 1;
    }
    [[maybe_unused]] const auto ba2_gnrl_target = ba2_gnrl_writer.target();

    libbsa::ba2_dx10_writer ba2_dx10_writer{libbsa::ba2_dx10_target::fallout4};
    [[maybe_unused]] const auto& ba2_dx10_options = ba2_dx10_writer.options();

    byte_vector_sink_factory factory;
    [[maybe_unused]] const auto& requested = factory.requested_paths();
    return 0;
}

std::string error_code_name(libbsa::error_code code) {
    switch (code) {
        case libbsa::error_code::unsupported:
            return "unsupported";
        case libbsa::error_code::invalid_argument:
            return "invalid_argument";
        case libbsa::error_code::not_found:
            return "not_found";
        case libbsa::error_code::io_error:
            return "io_error";
        case libbsa::error_code::format_error:
            return "format_error";
    }
    return "unknown";
}

}  // namespace

int main() {
    if (example_link_representative_public_api() != 0) {
        std::cerr << "libbsa package-consumer public API link smoke failed\n";
        return 1;
    }

    auto smoke = run_installed_package_archive_runtime_smoke();
    if (!smoke) {
        std::cerr << "libbsa package-consumer runtime smoke failed ["
                  << error_code_name(smoke.error().code) << "]: " << smoke.error().message << '\n';
        return 1;
    }

    return 0;
}
