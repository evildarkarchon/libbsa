#include "formats/ba2/ba2_dx10_prepare.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/byte_vector.hpp>
#include <detail/parallel_work.hpp>
#include <detail/writer_disk_source.hpp>

#include "texture/dds_layout.hpp"
#include "texture/directxtex_analyzer.hpp"

// BCrypt depends on Windows base types; keep both headers private and prevent min/max macros.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <bcrypt.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>

namespace libbsa::formats::ba2 {

namespace {

/// Returns true for DDS DXGI formats accepted by the Starfield DX10 profile but not Fallout 4.
bool is_starfield_only_dx10_format(std::uint32_t dxgi_format) noexcept {
  switch (dxgi_format) {
  case 29U: // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB.
  case 31U: // DXGI_FORMAT_R8G8B8A8_SNORM.
  case 72U: // DXGI_FORMAT_BC1_UNORM_SRGB.
  case 84U: // DXGI_FORMAT_BC5_SNORM.
  case 95U: // DXGI_FORMAT_BC6H_UF16.
  case 96U: // DXGI_FORMAT_BC6H_SF16.
  case 99U: // DXGI_FORMAT_BC7_UNORM_SRGB.
    return true;
  default:
    return false;
  }
}

std::string preserved_archive_path(std::string_view archive_path) {
  std::string preserved{archive_path};
  std::replace(preserved.begin(), preserved.end(), '\\', '/');
  return preserved;
}

constexpr detail::writer_disk_source_context ba2_dx10_dds_source_context{
    "BA2 DX10 writer failed to open DDS source",
    "BA2 DX10 writer failed to inspect DDS source",
    "BA2 DX10 writer failed while reading DDS source",
    "BA2 DX10 DDS source changed during analysis",
    "BA2 DX10 DDS source"};

constexpr detail::writer_disk_source_context ba2_dx10_snapshot_source_context{
    "BA2 DX10 writer failed to open snapshot temp file",
    "BA2 DX10 writer failed to inspect snapshot temp file",
    "BA2 DX10 writer failed while reading snapshot temp file",
    "BA2 DX10 writer failed while reading snapshot temp file",
    "BA2 DX10 snapshot temp file"};

constexpr std::size_t snapshot_random_suffix_bytes = 16U;

result<std::vector<std::byte>> read_dds_file(std::string_view dds_host_path) {
  return detail::read_disk_source_exact(dds_host_path, ba2_dx10_dds_source_context);
}

/// Generates a 128-bit lowercase hex suffix using the Windows system-preferred RNG.
/// Returns `io_error` if Windows cannot provide fresh cryptographic randomness for a candidate.
result<std::string> make_snapshot_random_suffix() {
  std::array<std::byte, snapshot_random_suffix_bytes> random_bytes{};
  const auto status = BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(random_bytes.data()),
                                      static_cast<ULONG>(random_bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
  if (status < 0) {
    return error{error_code::io_error, "BA2 DX10 writer failed to generate snapshot temp directory name"};
  }

  constexpr char hex_digits[] = "0123456789abcdef";
  std::string suffix;
  suffix.resize(random_bytes.size() * 2U);
  for (std::size_t index = 0; index < random_bytes.size(); ++index) {
    const auto value = static_cast<unsigned char>(random_bytes[index]);
    suffix[index * 2U] = hex_digits[(value >> 4U) & 0x0FU];
    suffix[index * 2U + 1U] = hex_digits[value & 0x0FU];
  }
  return suffix;
}

result<std::filesystem::path> make_unique_snapshot_directory() {
  std::error_code fs_error;
  const auto root = std::filesystem::temp_directory_path(fs_error);
  if (fs_error) {
    return error{error_code::io_error, "BA2 DX10 writer failed to locate snapshot temp root"};
  }

  for (std::uint32_t attempt = 0; attempt < 1024U; ++attempt) {
    auto suffix = make_snapshot_random_suffix();
    if (!suffix) {
      return suffix.error();
    }
    const auto candidate = root / ("libbsa-dx10-snapshot-" + suffix.value());
    fs_error.clear();
    // Directory creation remains the atomic reservation boundary; existing paths are collisions.
    if (std::filesystem::create_directory(candidate, fs_error)) {
      return candidate;
    }
    if (fs_error) {
      return error{error_code::io_error, "BA2 DX10 writer failed to reserve snapshot temp directory"};
    }
  }
  return error{error_code::io_error, "BA2 DX10 writer exhausted snapshot temp directory names"};
}

result<void> write_snapshot_file(const std::filesystem::path& snapshot_path, std::span<const std::byte> bytes) {
  std::ofstream output{snapshot_path, std::ios::binary | std::ios::trunc};
  if (!output) {
    return error{error_code::io_error, "BA2 DX10 writer failed to create snapshot temp file"};
  }
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!output) {
    return error{error_code::io_error, "BA2 DX10 writer failed while writing snapshot temp file"};
  }
  return {};
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

result<std::uint8_t> checked_u8(std::uint64_t value, std::string_view description) {
  if (value > std::numeric_limits<std::uint8_t>::max()) {
    return error{error_code::format_error, std::string{description} + " exceeds UInt8 range"};
  }
  return static_cast<std::uint8_t>(value);
}

result<std::size_t> checked_size_t(std::uint64_t value, std::string_view description) {
  if (value > std::numeric_limits<std::size_t>::max()) {
    return error{error_code::format_error, std::string{description} + " exceeds platform size range"};
  }
  return static_cast<std::size_t>(value);
}

std::pair<std::string_view, std::string_view> split_directory_file(std::string_view archive_path) noexcept {
  const auto slash = archive_path.find_last_of('/');
  if (slash == std::string_view::npos) {
    return {{}, archive_path};
  }
  return {archive_path.substr(0, slash), archive_path.substr(slash + 1U)};
}

std::pair<std::string_view, std::string_view> split_stem_extension(std::string_view file_name) noexcept {
  const auto dot = file_name.find_last_of('.');
  if (dot == std::string_view::npos || dot == 0U || dot + 1U == file_name.size()) {
    return {{}, {}};
  }
  return {file_name.substr(0, dot), file_name.substr(dot + 1U)};
}

bool is_ascii_extension_byte(unsigned char value) noexcept { return value > 0x20U && value <= 0x7EU; }

result<std::array<std::byte, 4>> extension_fourcc_for(std::string_view extension) {
  if (extension.size() > 4U) {
    return error{error_code::invalid_argument, "BA2 DX10 extension exceeds four-byte record field"};
  }

  std::array<std::byte, 4> fourcc{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};
  for (std::size_t index = 0; index < extension.size(); ++index) {
    const auto value = static_cast<unsigned char>(extension[index]);
    if (!is_ascii_extension_byte(value)) {
      return error{error_code::invalid_argument, "BA2 DX10 extension must contain printable ASCII bytes"};
    }
    fourcc[index] = static_cast<std::byte>(value);
  }
  return fourcc;
}

result<detail::compression_method> compression_method_for(ba2_dx10_target target, std::uint32_t starfield_method) {
  switch (target) {
  case ba2_dx10_target::fallout4:
    return detail::compression_method::deflate;
  case ba2_dx10_target::starfield_v3:
    if (starfield_method == ba2_starfield_compression_deflate) {
      return detail::compression_method::deflate;
    }
    if (starfield_method == ba2_starfield_compression_lz4_block) {
      return detail::compression_method::lz4_block;
    }
    return error{error_code::unsupported, "BA2 DX10 Starfield v3 compression method is unsupported"};
  }
  return error{error_code::invalid_argument, "BA2 DX10 writer target profile is not supported"};
}

result<void> append_snapshot_bytes(std::vector<std::byte>& bytes, const ba2_dx10_subresource_snapshot& snapshot) {
  return detail::for_each_disk_source_chunk(
      snapshot.snapshot_path.string(),
      snapshot.size,
      ba2_dx10_snapshot_source_context,
      [&](std::span<const std::byte> chunk) -> result<void> {
        return detail::append_byte_vector(bytes, chunk, "BA2 DX10 raw texture chunk bytes");
      });
}

struct ba2_dx10_chunk_snapshot_batch {
  std::vector<const ba2_dx10_subresource_snapshot*> snapshots;
  std::uint64_t aggregate_size{};
};

/// Collects snapshot subresources for one planned BA2 DX10 chunk in archive mip order.
result<ba2_dx10_chunk_snapshot_batch> collect_chunk_snapshots(const ba2_dx10_writer_entry& source,
                                                              const texture::planned_texture_chunk& chunk) {
  if (chunk.start_mip > chunk.end_mip) {
    return error{error_code::format_error, "BA2 DX10 planned chunk mip range is invalid"};
  }

  ba2_dx10_chunk_snapshot_batch batch;
  batch.snapshots.reserve(static_cast<std::size_t>(chunk.end_mip - chunk.start_mip) + 1U);

  // The writer does not transform DDS image data: it copies existing subresource bytes in the
  // BA2-required chunk layout without resizing, transcoding, mip generation, repair, or reordering.
  for (std::uint32_t mip = chunk.start_mip;; ++mip) {
    const auto found = std::ranges::find_if(source.subresources, [&](const ba2_dx10_subresource_snapshot& subresource) {
      return subresource.array_index == chunk.array_index && subresource.face_index == chunk.face_index &&
             subresource.mip == mip;
    });
    if (found == source.subresources.end()) {
      return error{error_code::format_error, "BA2 DX10 source DDS is missing a planned subresource"};
    }
    if (found->size > std::numeric_limits<std::uint64_t>::max() - batch.aggregate_size) {
      return error{error_code::format_error, "BA2 DX10 planned chunk snapshot size overflows"};
    }
    batch.aggregate_size += found->size;
    batch.snapshots.push_back(&*found);
    if (mip == chunk.end_mip) {
      break;
    }
  }
  return batch;
}

} // namespace

result<void> ba2_dx10_validate_texture_format_for_target(ba2_dx10_target target, std::uint32_t dxgi_format) {
  switch (target) {
  case ba2_dx10_target::fallout4:
    if (is_starfield_only_dx10_format(dxgi_format)) {
      return error{error_code::format_error,
                   "BA2 DX10 Fallout 4 target does not support BC6, SRGB, or SNORM DDS formats"};
    }
    return {};
  case ba2_dx10_target::starfield_v3:
    return {};
  }
  return error{error_code::invalid_argument, "BA2 DX10 writer target profile is not supported"};
}

result<void> ba2_dx10_ensure_snapshot_directory(std::filesystem::path& snapshot_dir_path) {
  if (!snapshot_dir_path.empty()) {
    return {};
  }
  auto snapshot_dir = make_unique_snapshot_directory();
  if (!snapshot_dir) {
    return snapshot_dir.error();
  }
  snapshot_dir_path = std::move(snapshot_dir.value());
  return {};
}

result<ba2_dx10_writer_entry> ba2_dx10_make_writer_entry(std::string_view archive_path,
                                                         std::string_view dds_host_path,
                                                         ba2_dx10_target target,
                                                         const std::filesystem::path& snapshot_dir,
                                                         std::size_t entry_index) {
  auto canonical = detail::normalize_archive_path(archive_path);
  if (!canonical) {
    return canonical.error();
  }

  auto dds_bytes = read_dds_file(dds_host_path);
  if (!dds_bytes) {
    return dds_bytes.error();
  }

  auto source = texture::analyze_dds_source(dds_bytes.value());
  if (!source) {
    return source.error();
  }
  auto target_format = ba2_dx10_validate_texture_format_for_target(target, source.value().metadata.dxgi_format);
  if (!target_format) {
    return target_format.error();
  }

  ba2_dx10_writer_entry entry;
  entry.archive_path_original = preserved_archive_path(archive_path);
  entry.archive_path_canonical = std::move(canonical.value().value);
  entry.metadata = source.value().metadata;
  entry.subresources.reserve(source.value().subresources.size());
  for (std::size_t index = 0; index < source.value().subresources.size(); ++index) {
    const auto& subresource = source.value().subresources[index];
    auto snapshot_path = snapshot_dir / ("entry-" + std::to_string(entry_index) + "-subresource-" +
                                         std::to_string(index) + ".bin");
    auto written = write_snapshot_file(snapshot_path, subresource.bytes);
    if (!written) {
      return written.error();
    }
    entry.subresources.push_back(ba2_dx10_subresource_snapshot{
        subresource.array_index, subresource.face_index, subresource.mip, subresource.bytes.size(), std::move(snapshot_path)});
  }
  return entry;
}

std::uint32_t ba2_dx10_version_for(ba2_dx10_target target) noexcept {
  switch (target) {
  case ba2_dx10_target::fallout4:
    return ba2_fallout4_version;
  case ba2_dx10_target::starfield_v3:
    return ba2_starfield_v3_version;
  }
  return 0U;
}

std::size_t ba2_dx10_header_size_for(std::uint32_t version) noexcept {
  return version >= ba2_starfield_v3_version ? ba2_starfield_v3_header_size : ba2_common_header_size;
}

result<void> ba2_dx10_validate_target_options(ba2_dx10_target target, const ba2_dx10_writer_options& options) {
  switch (target) {
  case ba2_dx10_target::fallout4:
    return {};
  case ba2_dx10_target::starfield_v3:
    if (options.starfield_compression_method == ba2_starfield_compression_deflate ||
        options.starfield_compression_method == ba2_starfield_compression_lz4_block) {
      return {};
    }
    return error{error_code::unsupported, "BA2 DX10 Starfield v3 compression method is unsupported"};
  }
  return error{error_code::invalid_argument, "BA2 DX10 writer target profile is not supported"};
}

result<void> ba2_dx10_validate_entries(ba2_dx10_target target, std::span<const ba2_dx10_writer_entry> entries) {
  if (entries.empty()) {
    return error{error_code::invalid_argument, "BA2 DX10 writer requires at least one texture entry"};
  }

  std::unordered_set<std::string> canonical_paths;
  for (const auto& entry : entries) {
    if (!canonical_paths.insert(entry.archive_path_canonical).second) {
      return error{error_code::format_error, "BA2 DX10 writer has duplicate canonical archive paths"};
    }
    auto target_format = ba2_dx10_validate_texture_format_for_target(target, entry.metadata.dxgi_format);
    if (!target_format) {
      return target_format.error();
    }
    const auto [directory, file_name] = split_directory_file(entry.archive_path_canonical);
    (void)directory;
    const auto [stem, extension_text] = split_stem_extension(file_name);
    if (stem.empty() || extension_text.empty()) {
      return error{error_code::invalid_argument, "BA2 DX10 archive path must include a file stem and extension"};
    }
    auto fourcc = extension_fourcc_for(extension_text);
    if (!fourcc) {
      return fourcc.error();
    }
    if (entry.subresources.empty()) {
      return error{error_code::format_error, "BA2 DX10 writer entry has no DDS image payload"};
    }
  }
  return {};
}

result<ba2_dx10_prepared_chunk> ba2_dx10_prepare_chunk(ba2_dx10_target target,
                                                        const ba2_dx10_writer_options& options,
                                                        const ba2_dx10_writer_entry& source,
                                                        const texture::planned_texture_chunk& planned) {
  auto snapshots = collect_chunk_snapshots(source, planned);
  if (!snapshots) {
    return snapshots.error();
  }
  if (snapshots.value().aggregate_size != planned.raw_size) {
    return error{error_code::format_error, "BA2 DX10 planned chunk size does not match source DDS bytes"};
  }
  auto raw_capacity = checked_size_t(planned.raw_size, "BA2 DX10 raw chunk size");
  if (!raw_capacity) {
    return raw_capacity.error();
  }

  std::vector<std::byte> raw_bytes;
  auto reserved = detail::reserve_byte_vector(raw_bytes, raw_capacity.value(), "BA2 DX10 raw texture chunk bytes");
  if (!reserved) {
    return reserved.error();
  }
  for (const auto* snapshot : snapshots.value().snapshots) {
    auto appended = append_snapshot_bytes(raw_bytes, *snapshot);
    if (!appended) {
      return appended.error();
    }
  }
  if (raw_bytes.empty()) {
    return error{error_code::format_error, "BA2 DX10 writer refuses to serialize an empty texture chunk"};
  }
  auto raw_size = checked_u32(raw_bytes.size(), "BA2 DX10 raw chunk size");
  if (!raw_size) {
    return raw_size.error();
  }
  if (raw_size.value() != planned.raw_size) {
    return error{error_code::format_error, "BA2 DX10 planned chunk size does not match source DDS bytes"};
  }

  auto method = compression_method_for(target, options.starfield_compression_method);
  if (!method) {
    return method.error();
  }
  auto compressed = detail::compress_payload(method.value(), raw_bytes);
  if (!compressed) {
    return compressed.error();
  }
  auto packed_size = checked_u32(compressed.value().size(), "BA2 DX10 packed chunk size");
  if (!packed_size) {
    return packed_size.error();
  }
  auto start_mip = checked_u16(planned.start_mip, "BA2 DX10 chunk start mip");
  auto end_mip = checked_u16(planned.end_mip, "BA2 DX10 chunk end mip");
  if (!start_mip || !end_mip) {
    return !start_mip ? start_mip.error() : end_mip.error();
  }
  return ba2_dx10_prepared_chunk{0U,
                                 packed_size.value(),
                                 raw_size.value(),
                                 start_mip.value(),
                                 end_mip.value(),
                                 method.value(),
                                 true,
                                 std::move(compressed.value())};
}

namespace {

result<ba2_dx10_prepared_entry> prepare_entry(ba2_dx10_target target,
                                              const ba2_dx10_writer_options& options,
                                              const ba2_dx10_writer_entry& entry,
                                              std::uint32_t worker_count) {
  texture::dds_texture_layout layout{entry.metadata.width,
                                     entry.metadata.height,
                                     entry.metadata.mip_count,
                                     entry.metadata.dxgi_format,
                                     entry.metadata.array_size,
                                     entry.metadata.is_cubemap};
  auto planned_chunks = texture::plan_dx10_chunks(layout, options.max_decoded_chunk_bytes);
  if (!planned_chunks) {
    return planned_chunks.error();
  }
  if (planned_chunks.value().empty()) {
    return error{error_code::format_error, "BA2 DX10 writer planned no chunks for texture"};
  }

  const auto [directory, file_name] = split_directory_file(entry.archive_path_canonical);
  const auto [stem, extension_text] = split_stem_extension(file_name);
  if (stem.empty() || extension_text.empty()) {
    return error{error_code::invalid_argument, "BA2 DX10 archive path must include a file stem and extension"};
  }
  auto extension = extension_fourcc_for(extension_text);
  if (!extension) {
    return extension.error();
  }

  auto chunk_count = checked_u8(planned_chunks.value().size(), "BA2 DX10 chunk count");
  auto height = checked_u16(entry.metadata.height, "BA2 DX10 texture height");
  auto width = checked_u16(entry.metadata.width, "BA2 DX10 texture width");
  auto mip_count = checked_u8(entry.metadata.mip_count, "BA2 DX10 mip count");
  auto dxgi_format = checked_u8(entry.metadata.dxgi_format, "BA2 DX10 DXGI format");
  if (!chunk_count || !height || !width || !mip_count || !dxgi_format) {
    return !chunk_count ? chunk_count.error()
                        : (!height ? height.error() : (!width ? width.error() : (!mip_count ? mip_count.error() : dxgi_format.error())));
  }

  ba2_dx10_prepared_entry prepared{entry.archive_path_original,
                                   entry.archive_path_canonical,
                                   extension.value(),
                                   detail::hash_fo4(stem),
                                   detail::hash_fo4(directory),
                                   ba2_dx10_unknown_tex_default,
                                   chunk_count.value(),
                                   height.value(),
                                   width.value(),
                                   mip_count.value(),
                                   dxgi_format.value(),
                                   entry.metadata.is_cubemap ? ba2_dx10_cubemap_raw : ba2_dx10_non_cubemap_raw,
                                   {}};
  std::vector<std::optional<ba2_dx10_prepared_chunk>> chunks_by_index(planned_chunks.value().size());
  auto work = [&](std::size_t index) -> result<void> {
    auto chunk = ba2_dx10_prepare_chunk(target, options, entry, planned_chunks.value()[index]);
    if (!chunk) {
      return chunk.error();
    }
    chunks_by_index[index] = std::move(chunk.value());
    return {};
  };
  auto prepared_chunks = detail::run_indexed_work(planned_chunks.value().size(), worker_count, work);
  if (!prepared_chunks) {
    return prepared_chunks.error();
  }
  prepared.chunks.reserve(planned_chunks.value().size());
  for (auto& chunk : chunks_by_index) {
    if (!chunk.has_value()) {
      return error{error_code::io_error, "BA2 DX10 worker did not prepare a texture chunk"};
    }
    prepared.chunks.push_back(std::move(chunk.value()));
  }
  return prepared;
}

} // namespace

result<std::vector<ba2_dx10_prepared_entry>> ba2_dx10_prepare_entries(ba2_dx10_target target,
                                                                      const ba2_dx10_writer_options& options,
                                                                      std::span<const ba2_dx10_writer_entry> entries,
                                                                      std::uint32_t worker_count) {
  std::vector<ba2_dx10_prepared_entry> prepared;
  prepared.reserve(entries.size());
  for (const auto& entry : entries) {
    auto next = prepare_entry(target, options, entry, worker_count);
    if (!next) {
      return next.error();
    }
    prepared.push_back(std::move(next.value()));
  }

  std::sort(prepared.begin(), prepared.end(), [](const ba2_dx10_prepared_entry& lhs,
                                                 const ba2_dx10_prepared_entry& rhs) {
    return lhs.archive_path_canonical < rhs.archive_path_canonical;
  });
  return prepared;
}

} // namespace libbsa::formats::ba2
