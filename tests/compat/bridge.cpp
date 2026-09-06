#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>

#include <libbsa/libbsa.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using json = nlohmann::json;
namespace fs = std::filesystem;

/// Converts bytes to lowercase hex without interpreting archive payload contents.
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(bytes.size() * 2U);
    for (const auto byte : bytes) {
        const auto value = std::to_integer<unsigned>(byte);
        result.push_back(digits[value >> 4U]);
        result.push_back(digits[value & 15U]);
    }
    return result;
}

/// Turns provider failures into bridge errors; no digest is returned on failure.
void crypto_check(NTSTATUS status) {
    if (status < 0) {
        throw std::runtime_error("Windows SHA-256 provider failed: " + std::to_string(status));
    }
}

/// Owns one streaming SHA-256 computation and its Windows provider lifetime.
class sha256 {
   public:
    /// Allocates the provider's required hash state independently of libbsa.
    sha256() {
        crypto_check(BCryptOpenAlgorithmProvider(&algorithm_, BCRYPT_SHA256_ALGORITHM, nullptr, 0));
        try {
            DWORD object_size = 0;
            DWORD returned = 0;
            crypto_check(BCryptGetProperty(algorithm_, BCRYPT_OBJECT_LENGTH,
                                           reinterpret_cast<PUCHAR>(&object_size),
                                           sizeof(object_size), &returned, 0));
            object_.resize(object_size);
            crypto_check(
                BCryptCreateHash(algorithm_, &hash_, object_.data(), object_size, nullptr, 0, 0));
        } catch (...) {
            BCryptCloseAlgorithmProvider(algorithm_, 0);
            throw;
        }
    }
    /// Releases provider state even when extraction or hashing failed.
    ~sha256() {
        if (hash_ != nullptr) BCryptDestroyHash(hash_);
        if (algorithm_ != nullptr) BCryptCloseAlgorithmProvider(algorithm_, 0);
    }
    sha256(const sha256&) = delete;
    sha256& operator=(const sha256&) = delete;

    /// Consumes bounded pieces because BCrypt accepts only an ULONG byte count.
    void update(std::span<const std::byte> bytes) {
        while (!bytes.empty()) {
            const auto count = static_cast<ULONG>(
                std::min<std::size_t>(bytes.size(), std::numeric_limits<ULONG>::max()));
            crypto_check(BCryptHashData(
                hash_, reinterpret_cast<PUCHAR>(const_cast<std::byte*>(bytes.data())), count, 0));
            bytes = bytes.subspan(count);
        }
    }
    /// Finalizes once and returns the 32-byte digest as lowercase hexadecimal.
    std::string finish() {
        std::array<std::byte, 32> digest{};
        crypto_check(BCryptFinishHash(hash_, reinterpret_cast<PUCHAR>(digest.data()),
                                      static_cast<ULONG>(digest.size()), 0));
        return hex(digest);
    }

   private:
    BCRYPT_ALG_HANDLE algorithm_{nullptr};
    BCRYPT_HASH_HANDLE hash_{nullptr};
    std::vector<UCHAR> object_;
};

/// Unwraps public results, retaining operation context in the bridge diagnostic.
template <class T>
T require(libbsa::result<T> result, std::string_view operation) {
    if (!result) throw std::runtime_error(std::string(operation) + ": " + result.error().message);
    return std::move(result).value();
}

/// Unwraps fallible public actions without exposing implementation helpers.
void require(libbsa::result<void> result, std::string_view operation) {
    if (!result) throw std::runtime_error(std::string(operation) + ": " + result.error().message);
}

/// Hashes decoded bytes while retaining only the DDS header-sized prefix.
class digest_sink final : public libbsa::payload_sink {
   public:
    /// Updates whole-file and both possible DDS surface digests across arbitrary writes.
    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        whole_.update(bytes);
        const auto prefix_count = std::min<std::size_t>(148U - header_.size(), bytes.size());
        header_.insert(header_.end(), bytes.begin(), bytes.begin() + prefix_count);
        // DDS has either a legacy 128-byte header or the 148-byte DXT10 header.
        // Compute both offsets; independent Python code decides which is semantic.
        if (size_ >= 128U)
            surface128_.update(bytes);
        else if (bytes.size() > 128U - size_)
            surface128_.update(bytes.subspan(128U - size_));
        if (size_ >= 148U)
            surface148_.update(bytes);
        else if (bytes.size() > 148U - size_)
            surface148_.update(bytes.subspan(148U - size_));
        size_ += bytes.size();
        return bytes.size();
    }

    /// Returns evidence only after successful complete extraction.
    json finish(const std::string& path) {
        json result{{"path", path}, {"size", size_}, {"sha256", whole_.finish()}};
        if (path.ends_with(".dds")) {
            // A legacy envelope ends at128; retaining148 would retain twenty
            // decoded pixel bytes instead of metadata in the local evidence.
            const bool dx10 = header_.size() >= 88U && header_[84] == std::byte{'D'} &&
                              header_[85] == std::byte{'X'} && header_[86] == std::byte{'1'} &&
                              header_[87] == std::byte{'0'};
            result["dds_header_hex"] = hex(std::span<const std::byte>{header_}.first(
                std::min(header_.size(), dx10 ? std::size_t{148} : std::size_t{128})));
            result["payload_sha256_128"] = surface128_.finish();
            result["payload_sha256_148"] = surface148_.finish();
        }
        return result;
    }

   private:
    sha256 whole_, surface128_, surface148_;
    std::uint64_t size_{0};
    std::vector<std::byte> header_;
};

/// Streams a decoded source into one caller-owned numbered scratch file.
class file_sink final : public libbsa::payload_sink {
   public:
    /// Opens a fresh scratch path; the enclosing scratch directory owns cleanup.
    explicit file_sink(const fs::path& path) : stream_(path, std::ios::binary) {
        if (!stream_) throw std::runtime_error("Cannot create repack scratch file");
    }
    /// Reports failed writes through the public sink contract.
    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        stream_.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(bytes.size()));
        if (!stream_) return libbsa::error{libbsa::error_code::io_error, "Scratch write failed"};
        return bytes.size();
    }
    /// Checks buffered I/O before handing the scratch file to a writer.
    void finish() {
        stream_.flush();
        if (!stream_) throw std::runtime_error("Scratch flush failed");
        stream_.close();
        if (stream_.fail()) throw std::runtime_error("Scratch close failed");
    }

   private:
    std::ofstream stream_;
};

/// Owns only a uniquely created scratch child, never a caller-supplied directory.
class scratch_directory {
   public:
    /// Creates a random child exclusively so teardown cannot own preexisting data.
    explicit scratch_directory(const fs::path& parent) {
        std::array<std::byte, 16> random{};
        crypto_check(BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(random.data()),
                                     static_cast<ULONG>(random.size()),
                                     BCRYPT_USE_SYSTEM_PREFERRED_RNG));
        path = parent / (".bridge-scratch-" + hex(random));
        if (!fs::create_directory(path))
            throw std::runtime_error("Scratch directory already exists");
    }
    /// Cleans only this instance's scratch tree on both success and failure.
    ~scratch_directory() {
        std::error_code ignored;
        fs::remove_all(path, ignored);
    }
    scratch_directory(const scratch_directory&) = delete;
    scratch_directory& operator=(const scratch_directory&) = delete;
    fs::path path;
};

/// Maps explicit JSON compression values and rejects misspellings.
libbsa::archive_compression_policy archive_compression(const json& options) {
    const auto value = options.value("compression", "default");
    if (value == "raw") return libbsa::archive_compression_policy::all_raw;
    if (value == "compressed") return libbsa::archive_compression_policy::all_compressed;
    if (value == "default") return libbsa::archive_compression_policy::target_default;
    throw std::runtime_error("Unknown archive compression policy");
}

/// Maps per-entry overrides without conflating raw and target codec selection.
libbsa::entry_compression_policy entry_compression(const json& entry) {
    const auto value = entry.value("compression", "inherit");
    if (value == "raw") return libbsa::entry_compression_policy::raw;
    if (value == "compressed") return libbsa::entry_compression_policy::compressed;
    if (value == "inherit") return libbsa::entry_compression_policy::inherit;
    throw std::runtime_error("Unknown entry compression policy");
}

/// Packs explicit host sources exclusively through public writer APIs.
json pack(const json& request) {
    const auto target = request.at("target").get<std::string>();
    const auto output = request.at("output").get<std::string>();
    const auto& entries = request.at("entries");
    const auto options = request.value("options", json::object());
    const auto compression = archive_compression(options);
    const libbsa::write_execution_options execution{options.value("workers", 1U)};
    const bool sharing = options.value("sharing", false);
    if (execution.worker_count == 0U) throw std::runtime_error("workers must be positive");
    if (target == "tes3") {
        if (sharing || compression == libbsa::archive_compression_policy::all_compressed)
            throw std::runtime_error("TES3 has no sharing or compression option");
        libbsa::tes3_bsa_writer writer;
        for (const auto& entry : entries) {
            if (entry_compression(entry) == libbsa::entry_compression_policy::compressed)
                throw std::runtime_error("TES3 cannot compress entries");
            require(writer.add_file(entry.at("path").get<std::string>(),
                                    entry.at("source").get<std::string>()),
                    "add TES3 source");
        }
        require(writer.write_to(output, execution), "write TES3");
    } else if (target == "bsa103" || target == "bsa104" || target == "bsa105") {
        libbsa::tes4_bsa_writer_options settings;
        settings.compression_policy = compression;
        settings.embed_file_names = options.value("embedded_names", false);
        settings.deduplicate_payloads = sharing;
        const auto profile = target == "bsa103"   ? libbsa::tes4_bsa_target::oblivion
                             : target == "bsa104" ? libbsa::tes4_bsa_target::fallout3
                                                  : libbsa::tes4_bsa_target::skyrim_se;
        libbsa::tes4_bsa_writer writer{profile, settings};
        for (const auto& entry : entries)
            require(
                writer.add_file(entry.at("path").get<std::string>(),
                                entry.at("source").get<std::string>(), entry_compression(entry)),
                "add BSA source");
        require(writer.write_to(output, execution), "write BSA");
    } else if (target == "gnrl1" || target == "gnrl2" || target == "gnrl3-zlib" ||
               target == "gnrl3-lz4") {
        libbsa::ba2_gnrl_writer_options settings;
        settings.compression = compression;
        settings.deduplicate_payloads = sharing;
        settings.starfield_compression_method = target == "gnrl3-zlib" ? 0U : 3U;
        const auto profile = target == "gnrl1"   ? libbsa::ba2_gnrl_target::fallout4
                             : target == "gnrl2" ? libbsa::ba2_gnrl_target::starfield_v2
                                                 : libbsa::ba2_gnrl_target::starfield_v3;
        libbsa::ba2_gnrl_writer writer{profile, settings};
        for (const auto& entry : entries)
            require(
                writer.add_file(entry.at("path").get<std::string>(),
                                entry.at("source").get<std::string>(), entry_compression(entry)),
                "add GNRL source");
        require(writer.write_to(output, execution), "write GNRL");
    } else if (target == "dx101" || target == "dx102" || target == "dx103-zlib" ||
               target == "dx103-lz4") {
        if (compression != libbsa::archive_compression_policy::target_default)
            throw std::runtime_error(
                "Public DX10 writer has no archive compression policy control");
        libbsa::ba2_dx10_writer_options settings;
        settings.deduplicate_payloads = sharing;
        settings.max_decoded_chunk_bytes = options.value("chunk_bytes", 0U);
        settings.starfield_compression_method = target == "dx103-zlib" ? 0U : 3U;
        const auto profile = target == "dx101"   ? libbsa::ba2_dx10_target::fallout4
                             : target == "dx102" ? libbsa::ba2_dx10_target::starfield_v2
                                                 : libbsa::ba2_dx10_target::starfield_v3;
        libbsa::ba2_dx10_writer writer{profile, settings};
        for (const auto& entry : entries) {
            if (entry_compression(entry) != libbsa::entry_compression_policy::inherit)
                throw std::runtime_error("Public DX10 writer has no per-entry compression control");
            require(writer.add_file(entry.at("path").get<std::string>(),
                                    entry.at("source").get<std::string>()),
                    "add DDS source");
        }
        require(writer.write_to(output, execution), "write DX10");
    } else
        throw std::runtime_error("Unknown writer target: " + target);
    return {{"ok", true}, {"path", output}};
}

/// Opens, resolves, and hashes every decoded entry, including complete DDS evidence.
json scan(const json& request) {
    auto reader =
        require(libbsa::archive_reader::open(request.at("archive").get<std::string>()), "open");
    const auto metadata = require(reader.metadata(), "metadata");
    const auto entries = require(reader.entries(), "entries");
    if (entries.size() != metadata.file_count) throw std::runtime_error("Catalog count mismatch");
    std::string subtype;
    if (metadata.type == libbsa::archive_type::ba2) {
        // Public metadata expresses subtype through texture metadata. Empty
        // catalogs cannot establish it; independently read just the FourCC.
        std::ifstream stream(fs::path(request.at("archive").get<std::string>()), std::ios::binary);
        std::array<char, 4> magic{};
        stream.seekg(8);
        stream.read(magic.data(), static_cast<std::streamsize>(magic.size()));
        if (!stream) throw std::runtime_error("Cannot read BA2 subtype");
        subtype.assign(magic.begin(), magic.end());
    }
    json result{{"ok", true},
                {"metadata",
                 {{"type", metadata.type == libbsa::archive_type::bsa ? "bsa" : "ba2"},
                  {"version", metadata.version},
                  {"subtype", subtype}}},
                {"entries", json::array()}};
    for (const auto& entry : entries) {
        const auto found = require(reader.find(entry.path), "find " + entry.path);
        if (!found || found->path != entry.path)
            throw std::runtime_error("Catalog lookup mismatch");
        digest_sink sink;
        require(reader.extract(entry.path, sink), "extract " + entry.path);
        auto evidence = sink.finish(entry.path);
        if (evidence.at("size").get<std::uint64_t>() != entry.raw_size)
            throw std::runtime_error("Decoded size mismatch: " + entry.path);
        result["entries"].push_back(std::move(evidence));
    }
    return result;
}

/// Repackages every entry in bounded decoded-byte batches using temporary host sources.
/// An entry larger than the budget forms its own batch; extraction remains streaming.
json repack(const json& request) {
    auto reader = require(libbsa::archive_reader::open(request.at("archive").get<std::string>()),
                          "open repack source");
    const auto entries = require(reader.entries(), "repack entries");
    const auto target = request.at("target").get<std::string>();
    const auto output_dir = fs::absolute(fs::path(request.at("output_dir").get<std::string>()));
    fs::create_directories(output_dir);
    const auto budget = request.value("batch_bytes", std::uint64_t{256U * 1024U * 1024U});
    if (budget == 0U) throw std::runtime_error("batch_bytes must be positive");
    json result{{"ok", true}, {"archives", json::array()}};
    std::size_t cursor = 0;
    std::size_t batch_index = 0;
    while (cursor < entries.size()) {
        scratch_directory scratch{output_dir};
        json batch_entries = json::array();
        json paths = json::array();
        std::uint64_t batch_size = 0;
        do {
            const auto& entry = entries[cursor];
            if (!batch_entries.empty() && entry.raw_size > budget - std::min(budget, batch_size))
                break;
            const auto source = scratch.path / (std::to_string(cursor) + ".source");
            file_sink sink{source};
            require(reader.extract(entry.path, sink), "extract repack source " + entry.path);
            sink.finish();
            batch_entries.push_back({{"path", entry.path}, {"source", source.string()}});
            paths.push_back(entry.path);
            batch_size += entry.raw_size;
            ++cursor;
        } while (cursor < entries.size() && batch_size < budget);
        const auto output =
            output_dir / ("batch-" + std::to_string(batch_index++) +
                          (target == "tes3" || target.starts_with("bsa") ? ".bsa" : ".ba2"));
        pack({{"target", target},
              {"output", output.string()},
              {"entries", batch_entries},
              {"options", request.value("options", json::object())}});
        result["archives"].push_back({{"path", output.string()}, {"entries", paths}});
    }
    return result;
}
}  // namespace

/// Executes one JSON request and writes a machine-readable result even on failure.
int main(int argc, char** argv) {
    fs::path request_path, response_path;
    json response;
    try {
        for (int i = 1; i < argc; ++i) {
            const std::string_view argument{argv[i]};
            if ((argument == "--request" || argument == "--response") && i + 1 < argc) {
                (argument == "--request" ? request_path : response_path) = argv[++i];
            } else
                throw std::runtime_error("Usage: bridge --request FILE --response FILE");
        }
        if (request_path.empty() || response_path.empty())
            throw std::runtime_error("Request and response paths are required");
        std::ifstream input(request_path);
        if (!input) throw std::runtime_error("Cannot open request JSON");
        const auto request = json::parse(input);
        const auto command = request.at("command").get<std::string>();
        if (command == "scan")
            response = scan(request);
        else if (command == "pack")
            response = pack(request);
        else if (command == "repack")
            response = repack(request);
        else
            throw std::runtime_error("Unknown command: " + command);
    } catch (const std::exception& error) {
        response = {{"ok", false}, {"error", error.what()}};
    }
    if (!response_path.empty()) {
        std::ofstream output(response_path);
        output << response.dump(2) << '\n';
        output.close();
        if (!output) {
            std::cerr << "Cannot write response JSON\n";
            return 1;
        }
    } else
        std::cerr << response.dump() << '\n';
    return response.at("ok").get<bool>() ? 0 : 1;
}
