#include "formats/ba2/ba2_gnrl_prepare.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/compression_router.hpp>
#include <detail/parallel_work.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <unordered_set>
#include <utility>

namespace libbsa::formats::ba2 {

namespace {

constexpr std::uint32_t starfield_deflate_method = 0U;
constexpr std::uint32_t starfield_lz4_block_method = 3U;

std::string preserved_archive_path(std::string_view archive_path) {
  std::string preserved{archive_path};
  std::replace(preserved.begin(), preserved.end(), '\\', '/');
  return preserved;
}

result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description) {
  if (value > std::numeric_limits<std::uint32_t>::max()) {
    return error{error_code::format_error, std::string{description} + " exceeds UInt32 range"};
  }
  return static_cast<std::uint32_t>(value);
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

result<ba2_gnrl_prepared_entry> prepare_entry(ba2_gnrl_target target,
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

  return ba2_gnrl_prepared_entry{entry.archive_path_original,
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

} // namespace

result<ba2_gnrl_writer_entry> ba2_gnrl_make_writer_entry(std::string_view archive_path,
                                                         ba2_gnrl_entry_options options) {
  auto canonical = detail::normalize_archive_path(archive_path);
  if (!canonical) {
    return canonical.error();
  }

  ba2_gnrl_writer_entry entry;
  entry.archive_path_original = preserved_archive_path(archive_path);
  entry.archive_path_canonical = std::move(canonical.value().value);
  entry.options = options;
  return entry;
}

result<void> ba2_gnrl_validate_target_options(ba2_gnrl_target target, const ba2_gnrl_writer_options& options) {
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

result<void> ba2_gnrl_validate_entries(std::span<const ba2_gnrl_writer_entry> entries) {
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

result<std::vector<ba2_gnrl_prepared_entry>> ba2_gnrl_prepare_entries(ba2_gnrl_target target,
                                                                      const ba2_gnrl_writer_options& options,
                                                                      std::span<const ba2_gnrl_writer_entry> entries,
                                                                      std::uint32_t worker_count) {
  std::vector<std::optional<ba2_gnrl_prepared_entry>> prepared_by_index(entries.size());
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

  std::vector<ba2_gnrl_prepared_entry> prepared;
  prepared.reserve(entries.size());
  for (auto& entry : prepared_by_index) {
    if (!entry.has_value()) {
      return error{error_code::io_error, "BA2 GNRL worker did not prepare an entry"};
    }
    prepared.push_back(std::move(entry.value()));
  }

  // D-17 keeps Phase 8 deterministic with a canonical-path fallback because traced BA2 writer evidence does not
  // prove a stricter hash sort requirement. Records and final filename-table entries stay paired by this order.
  std::sort(prepared.begin(), prepared.end(), [](const ba2_gnrl_prepared_entry& lhs,
                                                 const ba2_gnrl_prepared_entry& rhs) {
    return lhs.archive_path_canonical < rhs.archive_path_canonical;
  });
  return prepared;
}

} // namespace libbsa::formats::ba2
