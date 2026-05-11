#include "formats/ba2/ba2_gnrl_writer.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>
#include <detail/compression_router.hpp>
#include <detail/parallel_work.hpp>
#include <detail/writer_publish.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace libbsa {

struct ba2_gnrl_writer::state {
  ba2_gnrl_target target;
  ba2_gnrl_writer_options options;
  std::vector<formats::ba2::ba2_gnrl_writer_entry> entries;
};

namespace {

std::string preserved_archive_path(std::string_view archive_path) {
  std::string preserved{archive_path};
  std::replace(preserved.begin(), preserved.end(), '\\', '/');
  return preserved;
}

result<formats::ba2::ba2_gnrl_writer_entry> make_entry(std::string_view archive_path,
                                                       ba2_gnrl_entry_options options) {
  auto canonical = detail::normalize_archive_path(archive_path);
  if (!canonical) {
    return canonical.error();
  }

  formats::ba2::ba2_gnrl_writer_entry entry;
  entry.archive_path_original = preserved_archive_path(archive_path);
  entry.archive_path_canonical = std::move(canonical.value().value);
  entry.options = options;
  return entry;
}

} // namespace

ba2_gnrl_writer::ba2_gnrl_writer(ba2_gnrl_target target)
    : ba2_gnrl_writer(target, ba2_gnrl_writer_options{}) {}

ba2_gnrl_writer::ba2_gnrl_writer(ba2_gnrl_target target, ba2_gnrl_writer_options options)
    : state_(std::make_shared<state>(state{target, options, {}})) {}

ba2_gnrl_target ba2_gnrl_writer::target() const noexcept { return state_->target; }

const ba2_gnrl_writer_options& ba2_gnrl_writer::options() const noexcept { return state_->options; }

result<void> ba2_gnrl_writer::add_file(std::string_view archive_path,
                                       std::string_view host_path,
                                       entry_compression_policy compression) {
  ba2_gnrl_entry_options entry_options;
  entry_options.compression = compression;
  return add_file(archive_path, host_path, entry_options);
}

result<void> ba2_gnrl_writer::add_file(std::string_view archive_path,
                                       std::string_view host_path,
                                       ba2_gnrl_entry_options options) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "BA2 GNRL disk source host path must not be empty"};
  }

  auto entry = make_entry(archive_path, options);
  if (!entry) {
    return entry.error();
  }

  entry.value().host_path = std::string{host_path};
  entry.value().from_memory = false;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> ba2_gnrl_writer::add_bytes(std::string_view archive_path,
                                        std::span<const std::byte> bytes,
                                        entry_compression_policy compression) {
  ba2_gnrl_entry_options entry_options;
  entry_options.compression = compression;
  return add_bytes(archive_path, bytes, entry_options);
}

result<void> ba2_gnrl_writer::add_bytes(std::string_view archive_path,
                                        std::span<const std::byte> bytes,
                                        ba2_gnrl_entry_options options) {
  auto entry = make_entry(archive_path, options);
  if (!entry) {
    return entry.error();
  }

  entry.value().memory_bytes.assign(bytes.begin(), bytes.end());
  entry.value().from_memory = true;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> ba2_gnrl_writer::write_to(std::string_view host_path) const {
  return write_to(host_path, write_execution_options{});
}

result<void> ba2_gnrl_writer::write_to(std::string_view host_path, write_execution_options execution) const {
  if (execution.worker_count == 0U) {
    return error{error_code::invalid_argument, "BA2 GNRL writer worker_count must be positive"};
  }
  return formats::ba2::write_ba2_gnrl_archive(state_->target, state_->options, state_->entries, host_path, execution.worker_count);
}

} // namespace libbsa

namespace libbsa::formats::ba2 {

namespace {

constexpr std::uint32_t starfield_deflate_method = 0U;
constexpr std::uint32_t starfield_lz4_block_method = 3U;
// Serialize the BA2 `BTDX`/`GNRL` header and required `0xBAADF00D` record sentinel explicitly.
constexpr std::uint32_t ba2_btdx_magic = 0x5844'5442U;
constexpr std::uint32_t ba2_gnrl_magic = 0x4C52'4E47U;
constexpr std::uint32_t ba2_record_sentinel = 0xBAAD'F00DU;
constexpr std::size_t common_header_size = 24U;
constexpr std::size_t starfield_v2_header_size = 32U;
constexpr std::size_t starfield_v3_header_size = 36U;
constexpr std::size_t gnrl_record_size = 36U;

struct prepared_entry {
  std::string archive_path_original;
  std::string archive_path_canonical;
  std::string source_path;
  std::array<std::byte, 4> extension{};
  std::uint32_t name_hash{};
  std::uint32_t directory_hash{};
  std::uint32_t record_flags{};
  std::uint64_t payload_offset{};
  std::uint32_t packed_size{};
  std::uint32_t raw_size{};
  std::uint64_t payload_hash{};
  bool stream_from_disk{false};
  bool owns_payload_bytes{true};
  std::vector<std::byte> stored_payload;
};

struct payload_assignment {
  std::uint64_t offset{};
  std::uint32_t stored_size{};
  std::size_t entry_index{};
};

struct dedupe_identity {
  std::uint32_t stored_size{};
  std::uint64_t hash{};

  bool operator<(const dedupe_identity& other) const noexcept {
    if (stored_size != other.stored_size) {
      return stored_size < other.stored_size;
    }
    return hash < other.hash;
  }
};

class stream_writer {
 public:
  explicit stream_writer(std::ostream& output) : output_(output) {}

  result<void> write_bytes(std::span<const std::byte> bytes) {
    if (bytes.empty()) {
      return {};
    }
    output_.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!output_) {
      return error{error_code::io_error, "BA2 GNRL writer failed while streaming archive bytes"};
    }
    return {};
  }

  result<void> write_u8(std::uint8_t value) {
    const std::byte byte{value};
    return write_bytes(std::span<const std::byte>{&byte, 1U});
  }

  result<void> write_u16_le(std::uint16_t value) {
    const std::array bytes{static_cast<std::byte>(value & 0xFFU), static_cast<std::byte>((value >> 8U) & 0xFFU)};
    return write_bytes(std::span<const std::byte>{bytes.data(), bytes.size()});
  }

  result<void> write_u32_le(std::uint32_t value) {
    const std::array bytes{static_cast<std::byte>(value & 0xFFU),
                           static_cast<std::byte>((value >> 8U) & 0xFFU),
                           static_cast<std::byte>((value >> 16U) & 0xFFU),
                           static_cast<std::byte>((value >> 24U) & 0xFFU)};
    return write_bytes(std::span<const std::byte>{bytes.data(), bytes.size()});
  }

  result<void> write_u64_le(std::uint64_t value) {
    std::array<std::byte, 8U> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index) {
      bytes[index] = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
    }
    return write_bytes(std::span<const std::byte>{bytes.data(), bytes.size()});
  }

 private:
  std::ostream& output_;
};

bool add_fits_u64(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& total) noexcept {
  if (lhs > std::numeric_limits<std::uint64_t>::max() - rhs) {
    return false;
  }
  total = lhs + rhs;
  return true;
}

result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description) {
  if (value > std::numeric_limits<std::uint32_t>::max()) {
    return error{error_code::format_error, std::string{description} + " exceeds UInt32 range"};
  }
  return static_cast<std::uint32_t>(value);
}

result<std::uint16_t> checked_u16(std::uint64_t value, std::string_view description) {
  if (value > std::numeric_limits<std::uint16_t>::max()) {
    return error{error_code::format_error, std::string{description} + " exceeds UInt16 range"};
  }
  return static_cast<std::uint16_t>(value);
}

std::uint32_t version_for(ba2_gnrl_target target) noexcept {
  switch (target) {
  case ba2_gnrl_target::fallout4:
    return 1U;
  case ba2_gnrl_target::starfield_v2:
    return 2U;
  case ba2_gnrl_target::starfield_v3:
    return 3U;
  }
  return 0U;
}

std::size_t header_size_for(std::uint32_t version) noexcept {
  if (version >= 3U) {
    return starfield_v3_header_size;
  }
  if (version >= 2U) {
    return starfield_v2_header_size;
  }
  return common_header_size;
}

std::pair<std::string_view, std::string_view> split_directory_file(std::string_view archive_path) noexcept {
  const auto slash = archive_path.find_last_of('/');
  if (slash == std::string_view::npos) {
    return {{}, archive_path};
  }
  return {archive_path.substr(0, slash), archive_path.substr(slash + 1U)};
}

result<std::vector<std::byte>> read_source_bytes(const ba2_gnrl_writer_entry& entry) {
  if (entry.from_memory) {
    return entry.memory_bytes;
  }

  std::ifstream input{entry.host_path, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "BA2 GNRL writer failed to open disk source"};
  }
  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  if (input.bad()) {
    return error{error_code::io_error, "BA2 GNRL writer failed while reading disk source"};
  }
  return bytes;
}

result<std::uint64_t> disk_file_size(const std::string& host_path) {
  std::error_code fs_error;
  const auto size = std::filesystem::file_size(host_path, fs_error);
  if (fs_error) {
    return error{error_code::io_error, "BA2 GNRL writer failed to inspect disk source size"};
  }
  return size;
}

std::uint64_t hash_bytes(std::span<const std::byte> bytes) noexcept {
  std::uint64_t hash = 14695981039346656037ULL;
  for (const auto byte : bytes) {
    hash ^= std::to_integer<std::uint8_t>(byte);
    hash *= 1099511628211ULL;
  }
  return hash;
}

result<std::uint64_t> hash_disk_payload(const std::string& host_path) {
  std::ifstream input{host_path, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "BA2 GNRL writer failed to open disk source"};
  }

  std::uint64_t hash = 14695981039346656037ULL;
  std::array<char, 64U * 1024U> scratch{};
  while (input) {
    input.read(scratch.data(), static_cast<std::streamsize>(scratch.size()));
    const auto count = input.gcount();
    for (std::streamsize index = 0; index < count; ++index) {
      hash ^= static_cast<std::uint8_t>(static_cast<unsigned char>(scratch[static_cast<std::size_t>(index)]));
      hash *= 1099511628211ULL;
    }
  }
  if (input.bad()) {
    return error{error_code::io_error, "BA2 GNRL writer failed while hashing disk source"};
  }
  return hash;
}

result<bool> payloads_equal(const prepared_entry& lhs, const prepared_entry& rhs) {
  if (lhs.stream_from_disk && rhs.stream_from_disk) {
    return gnrl_detail::compare_disk_payloads(lhs.source_path, rhs.source_path, lhs.raw_size);
  }
  if (lhs.stream_from_disk) {
    return gnrl_detail::compare_disk_payload_to_bytes(lhs.source_path, rhs.stored_payload);
  }
  if (rhs.stream_from_disk) {
    return gnrl_detail::compare_disk_payload_to_bytes(rhs.source_path, lhs.stored_payload);
  }
  return lhs.stored_payload == rhs.stored_payload;
}

bool archive_default_compressed(archive_compression_policy policy) noexcept {
  switch (policy) {
  case archive_compression_policy::target_default:
  case archive_compression_policy::all_compressed:
    return true;
  case archive_compression_policy::all_raw:
    return false;
  }
  return true;
}

bool requested_entry_compression(bool archive_compressed, entry_compression_policy policy) noexcept {
  switch (policy) {
  case entry_compression_policy::inherit:
    return archive_compressed;
  case entry_compression_policy::raw:
    return false;
  case entry_compression_policy::compressed:
    return true;
  }
  return archive_compressed;
}

result<detail::compression_method> compression_method_for_compressed_entry(ba2_gnrl_target target,
                                                                           std::uint32_t starfield_method) {
  switch (target) {
  case ba2_gnrl_target::fallout4:
  case ba2_gnrl_target::starfield_v2:
    return detail::compression_method::deflate;
  case ba2_gnrl_target::starfield_v3:
    if (starfield_method == starfield_deflate_method) {
      return detail::compression_method::deflate;
    }
    if (starfield_method == starfield_lz4_block_method) {
      return detail::compression_method::lz4_block;
    }
    return error{error_code::unsupported, "BA2 GNRL Starfield v3 compression method is unsupported"};
  }
  return error{error_code::invalid_argument, "BA2 GNRL writer target profile is not supported"};
}

result<std::array<std::byte, 4>> extension_fourcc_for(std::string_view archive_path);

result<prepared_entry> prepare_entry(ba2_gnrl_target target,
                                     const ba2_gnrl_writer_options& options,
                                     const ba2_gnrl_writer_entry& entry) {
  const bool archive_compressed = archive_default_compressed(options.compression);
  std::vector<std::byte> stored_payload;
  std::uint64_t source_size = entry.from_memory ? entry.memory_bytes.size() : 0U;
  if (!entry.from_memory) {
    auto disk_size = disk_file_size(entry.host_path);
    if (!disk_size) {
      return disk_size.error();
    }
    source_size = disk_size.value();
  }

  const bool entry_compressed =
      source_size != 0U && requested_entry_compression(archive_compressed, entry.options.compression);
  auto raw_size = checked_u32(source_size, "BA2 GNRL raw payload size");
  if (!raw_size) {
    return raw_size.error();
  }

  std::uint32_t packed_size = 0U;
  bool stream_from_disk = !entry.from_memory && !entry_compressed;
  std::uint64_t payload_hash = 0U;
  if (entry_compressed || entry.from_memory) {
    auto payload = read_source_bytes(entry);
    if (!payload) {
      return payload.error();
    }
    if (entry_compressed) {
      auto method = compression_method_for_compressed_entry(target, options.starfield_compression_method);
      if (!method) {
        return method.error();
      }
      auto compressed = detail::compress_payload(method.value(), payload.value());
      if (!compressed) {
        return compressed.error();
      }
      auto packed = checked_u32(compressed.value().size(), "BA2 GNRL packed payload size");
      if (!packed) {
        return packed.error();
      }
      packed_size = packed.value();
      stored_payload = std::move(compressed.value());
    } else {
      stored_payload = std::move(payload.value());
    }
    payload_hash = hash_bytes(stored_payload);
  } else {
    auto hash = hash_disk_payload(entry.host_path);
    if (!hash) {
      return hash.error();
    }
    payload_hash = hash.value();
  }

  auto extension = extension_fourcc_for(entry.archive_path_original);
  if (!extension) {
    return extension.error();
  }

  const auto [directory, file_name] = split_directory_file(entry.archive_path_canonical);
  if (file_name.empty()) {
    return error{error_code::invalid_argument, "BA2 GNRL archive path must include a file name"};
  }

  return prepared_entry{entry.archive_path_original,
                        entry.archive_path_canonical,
                        entry.host_path,
                        extension.value(),
                        detail::hash_fo4(file_name),
                        detail::hash_fo4(directory),
                        entry.options.record_flags.value_or(0U),
                        0U,
                        packed_size,
                        raw_size.value(),
                        payload_hash,
                        stream_from_disk,
                        true,
                        std::move(stored_payload)};
}

result<std::vector<prepared_entry>> prepare_entries(ba2_gnrl_target target,
                                                    const ba2_gnrl_writer_options& options,
                                                    std::span<const ba2_gnrl_writer_entry> entries,
                                                    std::uint32_t worker_count) {
  std::vector<std::optional<prepared_entry>> prepared_by_index(entries.size());
  auto work = [&](std::size_t index) -> result<void> {
    auto prepared = prepare_entry(target, options, entries[index]);
    if (!prepared) {
      return prepared.error();
    }
    prepared_by_index[index] = std::move(prepared.value());
    return {};
  };

  auto prepared_work = detail::run_indexed_work(entries.size(), worker_count, work);
  if (!prepared_work) {
    return prepared_work.error();
  }

  std::vector<prepared_entry> prepared;
  prepared.reserve(entries.size());
  for (auto& entry : prepared_by_index) {
    if (!entry.has_value()) {
      return error{error_code::io_error, "BA2 GNRL worker did not prepare an entry"};
    }
    prepared.push_back(std::move(entry.value()));
  }

  // D-17 keeps Phase 8 deterministic with a canonical-path fallback because traced BA2 writer evidence does not
  // prove a stricter hash sort requirement. Records and final filename-table entries stay paired by this order.
  std::sort(prepared.begin(), prepared.end(), [](const prepared_entry& lhs, const prepared_entry& rhs) {
    return lhs.archive_path_canonical < rhs.archive_path_canonical;
  });
  return prepared;
}

result<void> assign_payload_offsets(std::span<prepared_entry> entries,
                                    std::uint32_t version,
                                    bool deduplicate_payloads,
                                    std::uint64_t& file_table_offset) {
  std::uint64_t record_bytes = 0;
  if (entries.size() > std::numeric_limits<std::uint64_t>::max() / gnrl_record_size) {
    return error{error_code::format_error, "BA2 GNRL record table size overflows"};
  }
  record_bytes = static_cast<std::uint64_t>(entries.size()) * gnrl_record_size;

  std::uint64_t cursor = 0;
  if (!add_fits_u64(header_size_for(version), record_bytes, cursor)) {
    return error{error_code::format_error, "BA2 GNRL metadata size overflows"};
  }
  const auto first_payload_offset = cursor;

  std::map<dedupe_identity, std::vector<payload_assignment>> deduplicated_payloads;
  for (std::size_t index = 0; index < entries.size(); ++index) {
    auto& entry = entries[index];
    auto stored_size = checked_u32(entry.stream_from_disk ? entry.raw_size : entry.stored_payload.size(),
                                   "BA2 GNRL stored payload size");
    if (!stored_size) {
      return stored_size.error();
    }
    if (deduplicate_payloads) {
      // D-23 requires dedupe after raw-vs-compressed routing, so this key is the exact byte span
      // the writer would store in the BA2 payload area rather than the caller's source bytes.
      const dedupe_identity identity{stored_size.value(), entry.payload_hash};
      auto duplicate = deduplicated_payloads.find(identity);
      bool reused_payload = false;
      if (duplicate != deduplicated_payloads.end()) {
        for (const auto& candidate : duplicate->second) {
          auto equal = payloads_equal(entry, entries[candidate.entry_index]);
          if (!equal) {
            return equal.error();
          }
          if (equal.value()) {
            entry.payload_offset = candidate.offset;
            entry.owns_payload_bytes = false;
            reused_payload = true;
            break;
          }
        }
      }
      if (reused_payload) {
        continue;
      }
    }

    // Empty BA2 GNRL entries do not own a physical payload span. Point them at the first payload byte so
    // FileTableOffset remains after every real payload while readers validate the zero-length span safely.
    entry.payload_offset = stored_size.value() == 0U ? first_payload_offset : cursor;
    entry.owns_payload_bytes = true;
    if (deduplicate_payloads) {
      deduplicated_payloads[dedupe_identity{stored_size.value(), entry.payload_hash}].push_back(
          payload_assignment{entry.payload_offset, stored_size.value(), index});
    }
    if (!add_fits_u64(cursor, stored_size.value(), cursor)) {
      return error{error_code::format_error, "BA2 GNRL payload span overflows"};
    }
  }

  file_table_offset = cursor;
  return {};
}

result<void> write_name(stream_writer& writer, std::string_view name) {
  auto length = checked_u16(name.size(), "BA2 GNRL filename-table entry length");
  if (!length) {
    return length.error();
  }
  auto written = writer.write_u16_le(length.value());
  if (!written) {
    return written.error();
  }
  for (const char ch : name) {
    if (!(written = writer.write_u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch))))) {
      return written.error();
    }
  }
  return {};
}

result<void> write_archive_bytes(ba2_gnrl_target target,
                                 const ba2_gnrl_writer_options& options,
                                 std::span<const prepared_entry> entries,
                                 std::uint32_t version,
                                 std::uint64_t file_table_offset,
                                 const std::filesystem::path& output_path) {
  std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
  if (!output) {
    return error{error_code::io_error, "BA2 GNRL writer failed to create temporary output"};
  }

  stream_writer writer{output};
  auto written = writer.write_u32_le(ba2_btdx_magic);
  if (!(written = writer.write_u32_le(version)) || !(written = writer.write_u32_le(ba2_gnrl_magic))) {
    return written.error();
  }
  auto file_count = checked_u32(entries.size(), "BA2 GNRL file count");
  if (!file_count) {
    return file_count.error();
  }
  if (!(written = writer.write_u32_le(file_count.value())) || !(written = writer.write_u64_le(file_table_offset))) {
    return written.error();
  }
  if (version >= 2U) {
    // xEdit/BSArchPro initializes Starfield writer Unknown1/Unknown2 to 1/0; options can override these raw
    // compatibility fields while keeping them library-owned and version-gated in public metadata.
    if (!(written = writer.write_u32_le(options.starfield_unknown1)) ||
        !(written = writer.write_u32_le(options.starfield_unknown2))) {
      return written.error();
    }
  }
  if (version >= 3U) {
    // Phase 8 treats v3 GNRL as a structurally supported profile. Method 3 remains the default raw-LZ4-block
    // method for later compression support; raw entries still serialize with PackedSize == 0 in this plan.
    if (!(written = writer.write_u32_le(options.starfield_compression_method))) {
      return written.error();
    }
  }

  for (const auto& entry : entries) {
    if (!(written = writer.write_u32_le(entry.name_hash)) || !(written = writer.write_bytes(entry.extension)) ||
        !(written = writer.write_u32_le(entry.directory_hash)) || !(written = writer.write_u32_le(entry.record_flags)) ||
        !(written = writer.write_u64_le(entry.payload_offset)) || !(written = writer.write_u32_le(entry.packed_size)) ||
        !(written = writer.write_u32_le(entry.raw_size)) || !(written = writer.write_u32_le(ba2_record_sentinel))) {
      return written.error();
    }
  }

  for (const auto& entry : entries) {
    if (!entry.owns_payload_bytes) {
      continue;
    }
    if (entry.stream_from_disk) {
      auto streamed = gnrl_detail::stream_disk_payload(entry.source_path, entry.raw_size, output);
      if (!streamed) {
        return streamed.error();
      }
    } else {
      if (!(written = writer.write_bytes(entry.stored_payload))) {
        return written.error();
      }
    }
  }

  for (const auto& entry : entries) {
    if (!(written = write_name(writer, entry.archive_path_original))) {
      return written.error();
    }
  }

  (void)target;
  if (!output) {
    return error{error_code::io_error, "BA2 GNRL writer failed while writing temporary output"};
  }
  return {};
}

result<void> validate_target_options(ba2_gnrl_target target, const ba2_gnrl_writer_options& options) {
  switch (target) {
  case ba2_gnrl_target::fallout4:
  case ba2_gnrl_target::starfield_v2:
    return {};
  case ba2_gnrl_target::starfield_v3:
    if (options.starfield_compression_method == starfield_deflate_method ||
        options.starfield_compression_method == starfield_lz4_block_method) {
      return {};
    }
    return error{error_code::unsupported, "BA2 GNRL Starfield v3 compression method is unsupported"};
  }
  return error{error_code::invalid_argument, "BA2 GNRL writer target profile is not supported"};
}

bool is_ascii_extension_byte(unsigned char value) noexcept { return value > 0x20U && value <= 0x7EU; }

result<std::array<std::byte, 4>> extension_fourcc_for(std::string_view archive_path) {
  const auto slash = archive_path.find_last_of('/');
  const auto file_name = slash == std::string_view::npos ? archive_path : archive_path.substr(slash + 1U);
  const auto dot = file_name.find_last_of('.');
  if (dot == std::string_view::npos || dot + 1U == file_name.size()) {
    return error{error_code::invalid_argument, "BA2 GNRL archive path must include a file extension"};
  }

  const auto extension = file_name.substr(dot + 1U);
  if (extension.size() > 4U) {
    return error{error_code::invalid_argument, "BA2 GNRL extension exceeds four-byte record field"};
  }

  std::array<std::byte, 4> fourcc{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};
  for (std::size_t index = 0; index < extension.size(); ++index) {
    const auto value = static_cast<unsigned char>(extension[index]);
    if (!is_ascii_extension_byte(value)) {
      return error{error_code::invalid_argument, "BA2 GNRL extension must contain printable ASCII bytes"};
    }
    fourcc[index] = static_cast<std::byte>(value);
  }
  return fourcc;
}

result<void> validate_entries(std::span<const ba2_gnrl_writer_entry> entries) {
  if (entries.empty()) {
    return error{error_code::invalid_argument, "BA2 GNRL writer requires at least one file entry"};
  }

  std::unordered_set<std::string> canonical_paths;
  for (const auto& entry : entries) {
    if (!canonical_paths.insert(entry.archive_path_canonical).second) {
      return error{error_code::format_error, "BA2 GNRL writer has duplicate canonical archive paths"};
    }

    auto fourcc = extension_fourcc_for(entry.archive_path_original);
    if (!fourcc) {
      return fourcc.error();
    }

    if (!entry.from_memory) {
      std::ifstream input{entry.host_path, std::ios::binary};
      if (!input) {
        return error{error_code::io_error, "BA2 GNRL writer failed to open disk source"};
      }
    }
  }

  return {};
}

} // namespace

namespace gnrl_detail {

result<bool> compare_disk_payload_to_bytes(const std::string& host_path, std::span<const std::byte> expected) {
  std::ifstream input{host_path, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "BA2 GNRL writer failed to open disk source"};
  }

  std::array<char, 64U * 1024U> scratch{};
  std::size_t offset = 0;
  while (offset < expected.size()) {
    const auto requested = std::min<std::size_t>(scratch.size(), expected.size() - offset);
    input.read(scratch.data(), static_cast<std::streamsize>(requested));
    if (input.gcount() != static_cast<std::streamsize>(requested)) {
      return error{error_code::io_error, "BA2 GNRL disk source changed during dedupe preparation"};
    }
    for (std::size_t index = 0; index < requested; ++index) {
      if (static_cast<std::byte>(static_cast<unsigned char>(scratch[index])) != expected[offset + index]) {
        return false;
      }
    }
    offset += requested;
  }

  // Dedupe decisions reuse earlier size/hash metadata; reject any source that no longer ends at that boundary.
  char extra = '\0';
  if (input.get(extra)) {
    return error{error_code::io_error, "BA2 GNRL disk source changed during dedupe preparation"};
  }
  if (input.bad()) {
    return error{error_code::io_error, "BA2 GNRL writer failed while comparing disk source"};
  }
  return true;
}

result<bool> compare_disk_payloads(const std::string& lhs_path,
                                   const std::string& rhs_path,
                                   std::uint32_t expected_size) {
  std::ifstream lhs{lhs_path, std::ios::binary};
  std::ifstream rhs{rhs_path, std::ios::binary};
  if (!lhs || !rhs) {
    return error{error_code::io_error, "BA2 GNRL writer failed to open disk source"};
  }

  std::array<char, 64U * 1024U> lhs_scratch{};
  std::array<char, 64U * 1024U> rhs_scratch{};
  std::uint64_t remaining = expected_size;
  while (remaining > 0U) {
    const auto requested = static_cast<std::size_t>(
        std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(lhs_scratch.size())));
    lhs.read(lhs_scratch.data(), static_cast<std::streamsize>(requested));
    rhs.read(rhs_scratch.data(), static_cast<std::streamsize>(requested));
    if (lhs.gcount() != static_cast<std::streamsize>(requested) ||
        rhs.gcount() != static_cast<std::streamsize>(requested)) {
      return error{error_code::io_error, "BA2 GNRL disk source changed during dedupe preparation"};
    }
    if (!std::equal(lhs_scratch.begin(), lhs_scratch.begin() + static_cast<std::ptrdiff_t>(requested), rhs_scratch.begin())) {
      return false;
    }
    remaining -= requested;
  }

  // A file that grew after preparation can otherwise compare equal for the prepared prefix and corrupt offsets.
  char lhs_extra = '\0';
  char rhs_extra = '\0';
  if (lhs.get(lhs_extra) || rhs.get(rhs_extra)) {
    return error{error_code::io_error, "BA2 GNRL disk source changed during dedupe preparation"};
  }
  if (lhs.bad() || rhs.bad()) {
    return error{error_code::io_error, "BA2 GNRL writer failed while comparing disk sources"};
  }
  return true;
}

result<void> stream_disk_payload(const std::string& host_path,
                                 std::uint32_t expected_size,
                                 std::ostream& output) {
  std::ifstream input{host_path, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "BA2 GNRL writer failed to open disk source"};
  }

  std::array<char, 64U * 1024U> scratch{};
  std::uint64_t remaining = expected_size;
  while (remaining > 0U) {
    const auto requested = static_cast<std::size_t>(
        std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(scratch.size())));
    input.read(scratch.data(), static_cast<std::streamsize>(requested));
    if (input.gcount() != static_cast<std::streamsize>(requested)) {
      return error{error_code::io_error, "BA2 GNRL disk source changed during finalization"};
    }
    output.write(scratch.data(), static_cast<std::streamsize>(requested));
    if (!output) {
      return error{error_code::io_error, "BA2 GNRL writer failed while streaming disk source"};
    }
    remaining -= requested;
  }

  // Metadata offsets and FileTableOffset are fixed before streaming, so an appended byte must fail the write.
  char extra = '\0';
  if (input.get(extra)) {
    return error{error_code::io_error, "BA2 GNRL disk source changed during finalization"};
  }
  if (input.bad()) {
    return error{error_code::io_error, "BA2 GNRL writer failed while reading disk source"};
  }
  return {};
}

} // namespace gnrl_detail

result<void> write_ba2_gnrl_archive(ba2_gnrl_target target,
                                    const ba2_gnrl_writer_options& options,
                                    std::span<const ba2_gnrl_writer_entry> entries,
                                    std::string_view output_host_path,
                                    std::uint32_t worker_count) {
  if (output_host_path.empty()) {
    return error{error_code::invalid_argument, "BA2 GNRL output host path must not be empty"};
  }

  auto target_options = validate_target_options(target, options);
  if (!target_options) {
    return target_options.error();
  }

  const auto output_path = std::filesystem::path{output_host_path};

  auto validated = validate_entries(entries);
  if (!validated) {
    return validated.error();
  }

  const auto version = version_for(target);
  auto prepared = prepare_entries(target, options, entries, worker_count);
  if (!prepared) {
    return prepared.error();
  }

  std::uint64_t file_table_offset = 0;
  auto offsets = assign_payload_offsets(prepared.value(), version, options.deduplicate_payloads, file_table_offset);
  if (!offsets) {
    return offsets.error();
  }

  return detail::publish_writer_output(
      output_path, options.overwrite_existing, "BA2 GNRL writer",
      [&](const std::filesystem::path& temp_path) -> result<void> {
        return write_archive_bytes(target, options, prepared.value(), version, file_table_offset, temp_path);
      });
}

} // namespace libbsa::formats::ba2
