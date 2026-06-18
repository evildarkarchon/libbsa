#include <libbsa/libbsa.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct benchmark_result {
    std::string scenario;
    std::uint32_t worker_count{1U};
    double elapsed_ms{0.0};
    std::uint64_t bytes_processed{0};
    bool correctness_passed{false};
};

struct named_payload {
    std::string archive_path;
    std::vector<std::byte> bytes;
};

struct cli_options {
    std::filesystem::path output_json;
    std::filesystem::path output_markdown;
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
        if (value.size() != 4U) {
            throw std::runtime_error("DDS fourcc values must be exactly four bytes");
        }
        for (const char ch : value) {
            u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch)));
        }
    }

    void raw(std::span<const std::byte> values) {
        bytes.insert(bytes.end(), values.begin(), values.end());
    }
};

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

std::string describe_error(const libbsa::error& error) {
    return error_code_name(error.code) + ": " + error.message;
}

void require_success(libbsa::result<void> result, std::string_view context) {
    if (!result.has_value()) {
        throw std::runtime_error(std::string{context} + ": " + describe_error(result.error()));
    }
}

template <typename T>
T require_value(libbsa::result<T> result, std::string_view context) {
    if (!result.has_value()) {
        throw std::runtime_error(std::string{context} + ": " + describe_error(result.error()));
    }
    return std::move(result).value();
}

std::vector<std::byte> patterned_bytes(std::size_t size, std::uint8_t seed) {
    std::vector<std::byte> bytes;
    bytes.reserve(size);
    for (std::size_t index = 0; index < size; ++index) {
        bytes.push_back(static_cast<std::byte>((seed + (index * 17U)) & 0xFFU));
    }
    return bytes;
}

std::vector<std::byte> bytes_from_text(std::string_view text) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

void write_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    if (!output) {
        throw std::runtime_error("failed to open " + path.string());
    }
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    if (!output) {
        throw std::runtime_error("failed to write " + path.string());
    }
}

std::vector<std::byte> build_rgba8_dds(std::uint32_t width, std::uint32_t height,
                                       std::uint8_t seed) {
    constexpr std::uint32_t dds_flags = 0x0002'100FU;
    constexpr std::uint32_t dds_fourcc_dx10 = 0x3031'5844U;
    constexpr std::uint32_t dds_caps_texture = 0x0000'1000U;
    constexpr std::uint32_t dxgi_format_r8g8b8a8_unorm = 28U;

    byte_buffer writer;
    writer.ascii4("DDS ");
    for (const auto value : {124U, dds_flags, height, width, 0U, 0U, 1U}) {
        writer.u32(value);
    }
    for (std::uint32_t index = 0; index < 11U; ++index) {
        writer.u32(0U);
    }
    for (const auto value : {32U, 0x0000'0004U, dds_fourcc_dx10, 0U, 0U, 0U, 0U, 0U,
                             dds_caps_texture, 0U, 0U, 0U, 0U}) {
        writer.u32(value);
    }
    for (const auto value : {dxgi_format_r8g8b8a8_unorm, 3U, 0U, 1U, 0U}) {
        writer.u32(value);
    }

    const auto payload = patterned_bytes(static_cast<std::size_t>(width) * height * 4U, seed);
    writer.raw(payload);
    return writer.bytes;
}

std::filesystem::path make_work_dir() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    auto path =
        std::filesystem::temp_directory_path() / ("libbsa-benchmark-" + std::to_string(stamp));
    std::filesystem::create_directories(path);
    return path;
}

void add_disk_payloads(libbsa::tes4_bsa_writer& writer, const std::filesystem::path& root,
                       std::span<const named_payload> entries) {
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto source = root / ("tes4-source-" + std::to_string(index) + ".bin");
        write_binary_file(source, entries[index].bytes);
        require_success(writer.add_file(entries[index].archive_path, source.string()),
                        "add TES4 benchmark source");
    }
}

void add_disk_payloads(libbsa::ba2_gnrl_writer& writer, const std::filesystem::path& root,
                       std::span<const named_payload> entries) {
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto source = root / ("ba2-gnrl-source-" + std::to_string(index) + ".bin");
        write_binary_file(source, entries[index].bytes);
        require_success(writer.add_file(entries[index].archive_path, source.string()),
                        "add BA2 GNRL benchmark source");
    }
}

std::uint64_t payload_total(std::span<const named_payload> entries) {
    std::uint64_t total = 0;
    for (const auto& entry : entries) {
        total += entry.bytes.size();
    }
    return total;
}

void verify_archive_payloads(const std::filesystem::path& archive,
                             std::span<const named_payload> entries) {
    auto reader =
        require_value(libbsa::archive_reader::open(archive.string()), "open benchmark archive");
    for (const auto& entry : entries) {
        auto extracted =
            require_value(reader.extract_bytes(entry.archive_path), "extract benchmark entry");
        if (extracted != entry.bytes) {
            throw std::runtime_error("benchmark extracted bytes mismatch for " +
                                     entry.archive_path);
        }
    }
}

benchmark_result run_tes4_bsa_pack_extract(const std::filesystem::path& work_dir,
                                           std::uint32_t worker_count) {
    const std::vector entries{
        named_payload{"Meshes/Benchmark/LargeA.nif", patterned_bytes(192U * 1024U, 0x11U)},
        named_payload{"Textures/Benchmark/LargeB.dds",
                      patterned_bytes((160U * 1024U) + 37U, 0x31U)},
        named_payload{"Scripts/Benchmark/Small.pex",
                      bytes_from_text("benchmark TES4 script payload")},
    };
    const auto root = work_dir / ("tes4-" + std::to_string(worker_count));
    const auto archive = root / "tes4-benchmark.bsa";

    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_compressed;
    options.overwrite_existing = true;
    libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
    add_disk_payloads(writer, root, entries);

    const auto start = std::chrono::steady_clock::now();
    libbsa::write_execution_options execution;
    execution.worker_count = worker_count;
    require_success(writer.write_to(archive.string(), execution), "write TES4 benchmark archive");
    verify_archive_payloads(archive, entries);
    const auto elapsed =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

    return {.scenario = "tes4_bsa_pack_extract",
            .worker_count = worker_count,
            .elapsed_ms = elapsed,
            .bytes_processed = payload_total(entries) * 2U,
            .correctness_passed = true};
}

benchmark_result run_ba2_gnrl_pack_extract(const std::filesystem::path& work_dir,
                                           std::uint32_t worker_count) {
    const std::vector entries{
        named_payload{"Meshes/Benchmark/GeneralA.nif", patterned_bytes(224U * 1024U, 0x41U)},
        named_payload{"Scripts/Benchmark/GeneralB.pex",
                      patterned_bytes((144U * 1024U) + 19U, 0x61U)},
    };
    const auto root = work_dir / ("ba2-gnrl-" + std::to_string(worker_count));
    const auto archive = root / "ba2-gnrl-benchmark.ba2";

    libbsa::ba2_gnrl_writer_options options;
    options.compression = libbsa::archive_compression_policy::all_compressed;
    options.overwrite_existing = true;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
    add_disk_payloads(writer, root, entries);

    const auto start = std::chrono::steady_clock::now();
    libbsa::write_execution_options execution;
    execution.worker_count = worker_count;
    require_success(writer.write_to(archive.string(), execution),
                    "write BA2 GNRL benchmark archive");
    verify_archive_payloads(archive, entries);
    const auto elapsed =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

    return {.scenario = "ba2_gnrl_pack_extract",
            .worker_count = worker_count,
            .elapsed_ms = elapsed,
            .bytes_processed = payload_total(entries) * 2U,
            .correctness_passed = true};
}

std::vector<benchmark_result> run_ba2_dx10_pack_extract(const std::filesystem::path& work_dir,
                                                        std::uint32_t worker_count) {
    const auto root = work_dir / ("ba2-dx10-" + std::to_string(worker_count));
    const auto source = root / "synthetic-rgba8.dds";
    const auto dds_bytes = build_rgba8_dds(256U, 256U, 0x73U);
    write_binary_file(source, dds_bytes);
    const auto archive = root / "ba2-dx10-benchmark.ba2";
    const auto archive_path = std::string{"textures/benchmark/synthetic-rgba8.dds"};

    libbsa::ba2_dx10_writer_options options;
    options.overwrite_existing = true;
    libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4, options};

    const auto add_start = std::chrono::steady_clock::now();
    require_success(writer.add_file(archive_path, source.string()),
                    "add BA2 DX10 benchmark source");
    const auto add_elapsed =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - add_start)
            .count();

    const auto start = std::chrono::steady_clock::now();
    libbsa::write_execution_options execution;
    execution.worker_count = worker_count;
    require_success(writer.write_to(archive.string(), execution),
                    "write BA2 DX10 benchmark archive");
    auto reader = require_value(libbsa::archive_reader::open(archive.string()),
                                "open BA2 DX10 benchmark archive");
    auto found = require_value(reader.find(archive_path), "find BA2 DX10 benchmark entry");
    if (!found.has_value() || !found->texture.has_value()) {
        throw std::runtime_error("BA2 DX10 benchmark texture metadata missing after reopen");
    }
    if (found->texture->width != 256U || found->texture->height != 256U ||
        found->texture->mip_count != 1U || found->texture->dxgi_format != 28U) {
        throw std::runtime_error("BA2 DX10 benchmark texture metadata mismatch after reopen");
    }
    const auto extracted =
        require_value(reader.extract_bytes(archive_path), "extract BA2 DX10 benchmark entry");
    if (extracted.size() != dds_bytes.size()) {
        throw std::runtime_error("BA2 DX10 benchmark extracted DDS size mismatch");
    }
    if (extracted != dds_bytes) {
        throw std::runtime_error("BA2 DX10 benchmark extracted DDS payload mismatch");
    }
    const auto elapsed =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

    return {benchmark_result{.scenario = "ba2_dx10_add_snapshot_staging",
                             .worker_count = worker_count,
                             .elapsed_ms = add_elapsed,
                             .bytes_processed = static_cast<std::uint64_t>(dds_bytes.size()),
                             .correctness_passed = true},
            benchmark_result{.scenario = "ba2_dx10_write_finalize_extract",
                             .worker_count = worker_count,
                             .elapsed_ms = elapsed,
                             .bytes_processed = static_cast<std::uint64_t>(dds_bytes.size()) * 2U,
                             .correctness_passed = true}};
}

struct capture {
    std::vector<std::byte> bytes;
};

class vector_sink final : public libbsa::payload_sink {
   public:
    explicit vector_sink(std::shared_ptr<capture> target) : target_(std::move(target)) {}

    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        target_->bytes.insert(target_->bytes.end(), bytes.begin(), bytes.end());
        return bytes.size();
    }

   private:
    std::shared_ptr<capture> target_;
};

class benchmark_sink_factory final : public libbsa::bulk_extract_sink_factory {
   public:
    libbsa::result<std::unique_ptr<libbsa::payload_sink>> create(
        std::string_view path, const libbsa::entry_metadata&) override {
        auto target = std::make_shared<capture>();
        {
            std::lock_guard lock{mutex_};
            captures_.emplace(std::string{path}, target);
        }
        return std::unique_ptr<libbsa::payload_sink>{new vector_sink{std::move(target)}};
    }

    [[nodiscard]] std::map<std::string, std::vector<std::byte>> bytes_by_path() const {
        std::lock_guard lock{mutex_};
        std::map<std::string, std::vector<std::byte>> bytes;
        for (const auto& [path, target] : captures_) {
            bytes.emplace(path, target->bytes);
        }
        return bytes;
    }

   private:
    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<capture>> captures_;
};

benchmark_result run_bulk_extract(const std::filesystem::path& work_dir,
                                  std::uint32_t worker_count) {
    const std::vector entries{
        named_payload{"Meshes/Benchmark/BulkA.nif", patterned_bytes(128U * 1024U, 0x21U)},
        named_payload{"Textures/Benchmark/BulkB.dds", patterned_bytes((96U * 1024U) + 5U, 0x51U)},
        named_payload{"Docs/Benchmark/BulkC.txt",
                      bytes_from_text("bulk extraction correctness payload")},
    };
    const auto root = work_dir / ("bulk-" + std::to_string(worker_count));
    const auto archive = root / "bulk-source.bsa";
    std::filesystem::create_directories(root);

    libbsa::tes3_bsa_writer_options options;
    options.overwrite_existing = true;
    libbsa::tes3_bsa_writer writer{options};
    for (const auto& entry : entries) {
        require_success(writer.add_bytes(entry.archive_path, entry.bytes),
                        "add bulk extraction source");
    }
    require_success(writer.write_to(archive.string()), "write bulk extraction source archive");

    auto reader = require_value(libbsa::archive_reader::open(archive.string()),
                                "open bulk extraction benchmark archive");
    std::vector<libbsa::bulk_extract_request> requests;
    requests.reserve(entries.size());
    for (const auto& entry : entries) {
        requests.push_back(libbsa::bulk_extract_request{.path = entry.archive_path});
    }

    const auto start = std::chrono::steady_clock::now();
    benchmark_sink_factory sink_factory;
    libbsa::bulk_extract_options bulk_options;
    bulk_options.worker_count = worker_count;
    // This scenario specifically exercises archive_reader::extract_entries as the
    // public bulk path.
    auto extracted = require_value(reader.extract_entries(requests, sink_factory, bulk_options),
                                   "bulk extract benchmark entries");
    const auto elapsed =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

    if (extracted.size() != entries.size()) {
        throw std::runtime_error("bulk extraction benchmark result count mismatch");
    }
    const auto bytes_by_path = sink_factory.bytes_by_path();
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (!extracted[index].succeeded() || extracted[index].path != entries[index].archive_path) {
            throw std::runtime_error(
                "bulk extraction benchmark result ordering or success mismatch");
        }
        const auto found = bytes_by_path.find(entries[index].archive_path);
        if (found == bytes_by_path.end() || found->second != entries[index].bytes) {
            throw std::runtime_error("bulk extraction benchmark payload mismatch");
        }
    }

    return {.scenario = "bulk_extract",
            .worker_count = worker_count,
            .elapsed_ms = elapsed,
            .bytes_processed = payload_total(entries),
            .correctness_passed = true};
}

std::vector<benchmark_result> run_benchmarks() {
    const auto work_dir = make_work_dir();
    std::vector<benchmark_result> results;
    for (const auto worker_count : {1U, 4U}) {
        results.push_back(run_tes4_bsa_pack_extract(work_dir, worker_count));
        results.push_back(run_ba2_gnrl_pack_extract(work_dir, worker_count));
        auto dx10_results = run_ba2_dx10_pack_extract(work_dir, worker_count);
        results.insert(results.end(), dx10_results.begin(), dx10_results.end());
        results.push_back(run_bulk_extract(work_dir, worker_count));
    }
    return results;
}

std::string json_escape(std::string_view value) {
    std::ostringstream out;
    for (const char ch : value) {
        switch (ch) {
            case '\\':
                out << "\\\\";
                break;
            case '"':
                out << "\\\"";
                break;
            case '\n':
                out << "\\n";
                break;
            default:
                out << ch;
                break;
        }
    }
    return out.str();
}

void write_json_report(const std::filesystem::path& path,
                       std::span<const benchmark_result> results) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    if (!out) {
        throw std::runtime_error("failed to open JSON report " + path.string());
    }

    out << "[\n";
    out << std::fixed << std::setprecision(3);
    for (std::size_t index = 0; index < results.size(); ++index) {
        const auto& result = results[index];
        out << "  {\"scenario\": \"" << json_escape(result.scenario)
            << "\", \"worker_count\": " << result.worker_count
            << ", \"elapsed_ms\": " << result.elapsed_ms
            << ", \"bytes_processed\": " << result.bytes_processed
            << ", \"correctness_passed\": " << (result.correctness_passed ? "true" : "false") << "}"
            << (index + 1U == results.size() ? "\n" : ",\n");
    }
    out << "]\n";
    if (!out) {
        throw std::runtime_error("failed to write JSON report " + path.string());
    }
}

void write_markdown_report(const std::filesystem::path& path,
                           std::span<const benchmark_result> results) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    if (!out) {
        throw std::runtime_error("failed to open Markdown report " + path.string());
    }

    out << "# libbsa Benchmark Report\n\n";
    out << "| scenario | worker_count | elapsed_ms | bytes_processed | "
           "correctness_passed |\n";
    out << "|----------|--------------|------------|-----------------|-----------"
           "---------|\n";
    out << std::fixed << std::setprecision(3);
    for (const auto& result : results) {
        out << "| " << result.scenario << " | " << result.worker_count << " | " << result.elapsed_ms
            << " | " << result.bytes_processed << " | "
            << (result.correctness_passed ? "true" : "false") << " |\n";
    }
    if (!out) {
        throw std::runtime_error("failed to write Markdown report " + path.string());
    }
}

cli_options parse_cli(int argc, char** argv) {
    cli_options options;
    for (int index = 1; index < argc; ++index) {
        const auto arg = std::string_view{argv[index]};
        if (arg == "--output-json" && index + 1 < argc) {
            options.output_json = argv[++index];
        } else if (arg == "--output-markdown" && index + 1 < argc) {
            options.output_markdown = argv[++index];
        } else {
            throw std::runtime_error(
                "usage: libbsa_benchmarks --output-json <path> "
                "--output-markdown <path>");
        }
    }
    if (options.output_json.empty() || options.output_markdown.empty()) {
        throw std::runtime_error(
            "usage: libbsa_benchmarks --output-json <path> "
            "--output-markdown <path>");
    }
    return options;
}

}  // namespace

/// Runs correctness-checked synthetic worker-count benchmark scenarios and
/// writes reports.
int main(int argc, char** argv) {
    try {
        const auto options = parse_cli(argc, argv);
        const auto results = run_benchmarks();
        write_json_report(options.output_json, results);
        write_markdown_report(options.output_markdown, results);
    } catch (const std::exception& exception) {
        std::cerr << "libbsa_benchmarks: " << exception.what() << '\n';
        return 1;
    }
    return 0;
}
