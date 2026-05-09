#include "formats/ba2/ba2_dx10_writer.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>
#include <detail/compression_router.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "texture/dds_layout.hpp"

namespace libbsa {

struct ba2_dx10_writer::state {
  ba2_dx10_target target;
  ba2_dx10_writer_options options;
  std::vector<formats::ba2::ba2_dx10_writer_entry> entries;
};

namespace {

std::string preserved_archive_path(std::string_view archive_path) {
  std::string preserved{archive_path};
  std::replace(preserved.begin(), preserved.end(), '\\', '/');
  return preserved;
}

result<std::vector<std::byte>> read_dds_file(std::string_view dds_host_path) {
  std::ifstream input{std::filesystem::path{dds_host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "BA2 DX10 writer failed to open DDS source"};
  }

  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  if (input.bad()) {
    return error{error_code::io_error, "BA2 DX10 writer failed while reading DDS source"};
  }
  return bytes;
}

result<formats::ba2::ba2_dx10_writer_entry> make_entry(std::string_view archive_path,
                                                        std::string_view dds_host_path) {
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

  formats::ba2::ba2_dx10_writer_entry entry;
  entry.archive_path_original = preserved_archive_path(archive_path);
  entry.archive_path_canonical = std::move(canonical.value().value);
  entry.dds_bytes = std::move(dds_bytes.value());
  entry.source = std::move(source.value());
  return entry;
}

} // namespace

ba2_dx10_writer::ba2_dx10_writer(ba2_dx10_target target) : ba2_dx10_writer(target, ba2_dx10_writer_options{}) {}

ba2_dx10_writer::ba2_dx10_writer(ba2_dx10_target target, ba2_dx10_writer_options options)
    : state_(std::make_shared<state>(state{target, options, {}})) {}

ba2_dx10_target ba2_dx10_writer::target() const noexcept { return state_->target; }

const ba2_dx10_writer_options& ba2_dx10_writer::options() const noexcept { return state_->options; }

result<void> ba2_dx10_writer::add_file(std::string_view archive_path, std::string_view dds_host_path) {
  if (dds_host_path.empty()) {
    return error{error_code::invalid_argument, "BA2 DX10 DDS source host path must not be empty"};
  }

  auto entry = make_entry(archive_path, dds_host_path);
  if (!entry) {
    return entry.error();
  }

  // D-03 requires add-time ownership so later source file changes or deletion cannot affect output.
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> ba2_dx10_writer::write_to(std::string_view host_path) const {
  return formats::ba2::write_ba2_dx10_archive(state_->target, state_->options, state_->entries, host_path);
}

} // namespace libbsa

namespace libbsa::formats::ba2 {

namespace {

constexpr std::uint32_t fallout4_version = 1U;
constexpr std::uint32_t starfield_v3_version = 3U;
constexpr std::uint32_t starfield_deflate_method = 0U;
constexpr std::uint32_t starfield_lz4_block_method = 3U;
constexpr std::uint32_t ba2_btdx_magic = 0x5844'5442U;
constexpr std::uint32_t ba2_dx10_magic = 0x3031'5844U;
constexpr std::uint32_t ba2_record_sentinel = 0xBAAD'F00DU;
constexpr std::uint16_t ba2_dx10_chunk_header_size = 24U;
constexpr std::uint16_t ba2_dx10_non_cubemap_raw = 2048U;
constexpr std::uint16_t ba2_dx10_cubemap_raw = 2049U;
constexpr std::size_t common_header_size = 24U;
constexpr std::size_t starfield_v3_header_size = 36U;
constexpr std::size_t dx10_record_size = 24U;
// Reference-derived writer-owned texture byte. It is intentionally not a public option because
// D-09 keeps unknown_tex library-owned until stronger compatibility evidence requires otherwise.
constexpr std::uint8_t ba2_dx10_unknown_tex_default = 0U;

struct prepared_chunk {
  std::uint64_t payload_offset{};
  std::uint32_t packed_size{};
  std::uint32_t raw_size{};
  std::uint16_t start_mip{};
  std::uint16_t end_mip{};
  detail::compression_method compression{};
  bool owns_payload_bytes{true};
  std::vector<std::byte> stored_payload;
};

struct prepared_entry {
  std::string archive_path_original;
  std::string archive_path_canonical;
  std::array<std::byte, 4> extension{};
  std::uint32_t name_hash{};
  std::uint32_t directory_hash{};
  std::uint8_t unknown_tex{ba2_dx10_unknown_tex_default};
  std::uint8_t chunk_count{};
  std::uint16_t height{};
  std::uint16_t width{};
  std::uint8_t mip_count{};
  std::uint8_t dxgi_format{};
  std::uint16_t cube_maps_raw{ba2_dx10_non_cubemap_raw};
  std::vector<prepared_chunk> chunks;
};

struct payload_assignment {
  std::uint64_t offset{};
};

struct dedupe_key {
  std::vector<std::byte> stored_payload;
  std::uint32_t raw_size{};
  std::uint32_t packed_size{};
  detail::compression_method compression{};

  bool operator<(const dedupe_key& other) const noexcept {
    if (stored_payload != other.stored_payload) {
      return stored_payload < other.stored_payload;
    }
    if (raw_size != other.raw_size) {
      return raw_size < other.raw_size;
    }
    if (packed_size != other.packed_size) {
      return packed_size < other.packed_size;
    }
    return static_cast<int>(compression) < static_cast<int>(other.compression);
  }
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

result<std::uint8_t> checked_u8(std::uint64_t value, std::string_view description) {
  if (value > std::numeric_limits<std::uint8_t>::max()) {
    return error{error_code::format_error, std::string{description} + " exceeds UInt8 range"};
  }
  return static_cast<std::uint8_t>(value);
}

std::uint32_t version_for(ba2_dx10_target target) noexcept {
  switch (target) {
  case ba2_dx10_target::fallout4:
    return fallout4_version;
  case ba2_dx10_target::starfield_v3:
    return starfield_v3_version;
  }
  return 0U;
}

std::size_t header_size_for(std::uint32_t version) noexcept {
  return version >= starfield_v3_version ? starfield_v3_header_size : common_header_size;
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
    if (starfield_method == starfield_deflate_method) {
      return detail::compression_method::deflate;
    }
    if (starfield_method == starfield_lz4_block_method) {
      return detail::compression_method::lz4_block;
    }
    return error{error_code::unsupported, "BA2 DX10 Starfield v3 compression method is unsupported"};
  }
  return error{error_code::invalid_argument, "BA2 DX10 writer target profile is not supported"};
}

result<void> append_subresource_bytes(std::vector<std::byte>& bytes,
                                      const texture::dds_source_analysis& source,
                                      const texture::planned_texture_chunk& chunk) {
  // The writer does not transform DDS image data: it copies existing subresource bytes in the
  // BA2-required chunk layout without resizing, transcoding, mip generation, repair, or reordering.
  for (std::uint32_t mip = chunk.start_mip; mip <= chunk.end_mip; ++mip) {
    const auto found = std::ranges::find_if(source.subresources, [&](const texture::dds_source_subresource& subresource) {
      return subresource.array_index == chunk.array_index && subresource.face_index == chunk.face_index &&
             subresource.mip == mip;
    });
    if (found == source.subresources.end()) {
      return error{error_code::format_error, "BA2 DX10 source DDS is missing a planned subresource"};
    }
    bytes.insert(bytes.end(), found->bytes.begin(), found->bytes.end());
  }
  return {};
}

result<prepared_chunk> prepare_chunk(ba2_dx10_target target,
                                     const ba2_dx10_writer_options& options,
                                     const texture::dds_source_analysis& source,
                                     const texture::planned_texture_chunk& planned) {
  std::vector<std::byte> raw_bytes;
  auto appended = append_subresource_bytes(raw_bytes, source, planned);
  if (!appended) {
    return appended.error();
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
  return prepared_chunk{0U,
                        packed_size.value(),
                        raw_size.value(),
                        start_mip.value(),
                        end_mip.value(),
                        method.value(),
                        true,
                        std::move(compressed.value())};
}

result<prepared_entry> prepare_entry(ba2_dx10_target target,
                                     const ba2_dx10_writer_options& options,
                                     const ba2_dx10_writer_entry& entry) {
  texture::dds_texture_layout layout{entry.source.metadata.width,
                                     entry.source.metadata.height,
                                     entry.source.metadata.mip_count,
                                     entry.source.metadata.dxgi_format,
                                     entry.source.metadata.array_size,
                                     entry.source.metadata.is_cubemap};
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
  auto height = checked_u16(entry.source.metadata.height, "BA2 DX10 texture height");
  auto width = checked_u16(entry.source.metadata.width, "BA2 DX10 texture width");
  auto mip_count = checked_u8(entry.source.metadata.mip_count, "BA2 DX10 mip count");
  auto dxgi_format = checked_u8(entry.source.metadata.dxgi_format, "BA2 DX10 DXGI format");
  if (!chunk_count || !height || !width || !mip_count || !dxgi_format) {
    return !chunk_count ? chunk_count.error()
                        : (!height ? height.error() : (!width ? width.error() : (!mip_count ? mip_count.error() : dxgi_format.error())));
  }

  prepared_entry prepared{entry.archive_path_original,
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
                          entry.source.metadata.is_cubemap ? ba2_dx10_cubemap_raw : ba2_dx10_non_cubemap_raw,
                          {}};
  prepared.chunks.reserve(planned_chunks.value().size());
  for (const auto& planned : planned_chunks.value()) {
    auto chunk = prepare_chunk(target, options, entry.source, planned);
    if (!chunk) {
      return chunk.error();
    }
    prepared.chunks.push_back(std::move(chunk.value()));
  }
  return prepared;
}

result<std::vector<prepared_entry>> prepare_entries(ba2_dx10_target target,
                                                    const ba2_dx10_writer_options& options,
                                                    std::span<const ba2_dx10_writer_entry> entries) {
  std::vector<prepared_entry> prepared;
  prepared.reserve(entries.size());
  for (const auto& entry : entries) {
    auto next = prepare_entry(target, options, entry);
    if (!next) {
      return next.error();
    }
    prepared.push_back(std::move(next.value()));
  }

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
  for (const auto& entry : entries) {
    std::uint64_t entry_record_bytes = 0;
    if (!add_fits_u64(dx10_record_size, static_cast<std::uint64_t>(entry.chunks.size()) * ba2_dx10_chunk_header_size,
                      entry_record_bytes) ||
        !add_fits_u64(record_bytes, entry_record_bytes, record_bytes)) {
      return error{error_code::format_error, "BA2 DX10 record table size overflows"};
    }
  }

  if (!add_fits_u64(header_size_for(version), record_bytes, file_table_offset)) {
    return error{error_code::format_error, "BA2 DX10 metadata size overflows"};
  }

  std::uint64_t cursor = file_table_offset;
  for (const auto& entry : entries) {
    std::uint64_t name_bytes = 0;
    if (!add_fits_u64(2U, entry.archive_path_original.size(), name_bytes) || !add_fits_u64(cursor, name_bytes, cursor)) {
      return error{error_code::format_error, "BA2 DX10 filename table size overflows"};
    }
  }

  std::map<dedupe_key, payload_assignment> deduplicated_payloads;
  for (auto& entry : entries) {
    for (auto& chunk : entry.chunks) {
      // D-20 requires DX10 dedupe to prove both byte identity and chunk metadata identity. Two
      // chunks only share storage when the final stored bytes, raw size, packed size, and explicit
      // compression route all match; texture dimensions and mip identity remain separate records.
      dedupe_key key{chunk.stored_payload, chunk.raw_size, chunk.packed_size, chunk.compression};
      if (deduplicate_payloads) {
        const auto duplicate = deduplicated_payloads.find(key);
        if (duplicate != deduplicated_payloads.end()) {
          chunk.payload_offset = duplicate->second.offset;
          chunk.owns_payload_bytes = false;
          continue;
        }
      }
      chunk.payload_offset = cursor;
      chunk.owns_payload_bytes = true;
      if (deduplicate_payloads) {
        deduplicated_payloads.emplace(std::move(key), payload_assignment{chunk.payload_offset});
      }
      if (!add_fits_u64(cursor, chunk.stored_payload.size(), cursor)) {
        return error{error_code::format_error, "BA2 DX10 payload span overflows"};
      }
    }
  }
  return {};
}

result<void> write_name(detail::binary_writer& writer, std::string_view name) {
  auto length = checked_u16(name.size(), "BA2 DX10 filename-table entry length");
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

result<void> write_archive_bytes(const ba2_dx10_writer_options& options,
                                 std::span<const prepared_entry> entries,
                                 std::uint32_t version,
                                 std::uint64_t file_table_offset,
                                 const std::filesystem::path& output_path) {
  detail::binary_writer writer;
  auto written = writer.write_u32_le(ba2_btdx_magic);
  if (!(written = writer.write_u32_le(version)) || !(written = writer.write_u32_le(ba2_dx10_magic))) {
    return written.error();
  }
  auto file_count = checked_u32(entries.size(), "BA2 DX10 file count");
  if (!file_count) {
    return file_count.error();
  }
  if (!(written = writer.write_u32_le(file_count.value())) || !(written = writer.write_u64_le(file_table_offset))) {
    return written.error();
  }
  if (version >= starfield_v3_version) {
    if (!(written = writer.write_u32_le(options.starfield_unknown1)) ||
        !(written = writer.write_u32_le(options.starfield_unknown2)) ||
        !(written = writer.write_u32_le(options.starfield_compression_method))) {
      return written.error();
    }
  }

  for (const auto& entry : entries) {
    if (!(written = writer.write_u32_le(entry.name_hash)) || !(written = writer.write_bytes(entry.extension)) ||
        !(written = writer.write_u32_le(entry.directory_hash)) || !(written = writer.write_u8(entry.unknown_tex)) ||
        !(written = writer.write_u8(entry.chunk_count)) || !(written = writer.write_u16_le(ba2_dx10_chunk_header_size)) ||
        !(written = writer.write_u16_le(entry.height)) || !(written = writer.write_u16_le(entry.width)) ||
        !(written = writer.write_u8(entry.mip_count)) || !(written = writer.write_u8(entry.dxgi_format)) ||
        !(written = writer.write_u16_le(entry.cube_maps_raw))) {
      return written.error();
    }
    for (const auto& chunk : entry.chunks) {
      if (!(written = writer.write_u64_le(chunk.payload_offset)) || !(written = writer.write_u32_le(chunk.packed_size)) ||
          !(written = writer.write_u32_le(chunk.raw_size)) || !(written = writer.write_u16_le(chunk.start_mip)) ||
          !(written = writer.write_u16_le(chunk.end_mip)) || !(written = writer.write_u32_le(ba2_record_sentinel))) {
        return written.error();
      }
    }
  }

  for (const auto& entry : entries) {
    if (!(written = write_name(writer, entry.archive_path_original))) {
      return written.error();
    }
  }

  for (const auto& entry : entries) {
    for (const auto& chunk : entry.chunks) {
      if (chunk.owns_payload_bytes && !(written = writer.write_bytes(chunk.stored_payload))) {
        return written.error();
      }
    }
  }

  std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
  if (!output) {
    return error{error_code::io_error, "BA2 DX10 writer failed to create temporary output"};
  }
  const auto bytes = writer.bytes();
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!output) {
    return error{error_code::io_error, "BA2 DX10 writer failed while writing temporary output"};
  }
  return {};
}

result<bool> path_exists_noexcept(const std::filesystem::path& path) {
  std::error_code fs_error;
  const bool exists = std::filesystem::exists(path, fs_error);
  if (fs_error) {
    return error{error_code::io_error, "BA2 DX10 writer failed to inspect output host path"};
  }
  return exists;
}

result<std::filesystem::path> make_unique_publish_directory(const std::filesystem::path& output_path) {
  const auto parent = output_path.parent_path();
  const auto filename = output_path.filename();
  for (std::uint32_t counter = 0; counter < 64U; ++counter) {
    auto candidate_name = filename;
    candidate_name += ".libbsa-tmp-" + std::to_string(counter);
    const auto candidate = parent.empty() ? candidate_name : parent / candidate_name;
    std::error_code fs_error;
    // A unique directory avoids deterministic `<archive>.tmp` cleanup, which could delete a
    // caller-owned sibling that merely looks like a temporary file.
    if (std::filesystem::create_directory(candidate, fs_error)) {
      return candidate;
    }
    if (fs_error) {
      return error{error_code::io_error, "BA2 DX10 writer failed to reserve temporary output directory"};
    }
  }
  return error{error_code::io_error, "BA2 DX10 writer exhausted temporary output directory names"};
}

result<std::filesystem::path> reserve_backup_path(const std::filesystem::path& output_path) {
  const auto parent = output_path.parent_path();
  const auto filename = output_path.filename();
  for (std::uint32_t counter = 0; counter < 64U; ++counter) {
    auto candidate_name = filename;
    candidate_name += ".libbsa-bak-" + std::to_string(counter);
    const auto candidate = parent.empty() ? candidate_name : parent / candidate_name;
    auto exists = path_exists_noexcept(candidate);
    if (!exists) {
      return exists.error();
    }
    if (!exists.value()) {
      return candidate;
    }
  }
  return error{error_code::io_error, "BA2 DX10 writer exhausted backup output path names"};
}

void cleanup_publish_directory(const std::filesystem::path& temp_dir) noexcept {
  std::error_code fs_error;
  // Cleanup is best-effort because callers should receive the primary write/publish failure, not cleanup noise.
  std::filesystem::remove_all(temp_dir, fs_error);
}

bool should_fail_after_backup_for_test(const std::filesystem::path& output_path) {
  // Tests use this path-scoped hook to exercise the rollback branch that normally requires
  // a narrow filesystem race after the original archive has already been moved aside.
#ifdef _WIN32
  char* requested_path = nullptr;
  std::size_t requested_path_size = 0;
  if (_dupenv_s(&requested_path, &requested_path_size, "LIBBSA_TEST_FAIL_BA2_DX10_PUBLISH_AFTER_BACKUP") != 0 ||
      requested_path == nullptr) {
    return false;
  }
  const std::filesystem::path requested{requested_path};
  std::free(requested_path);
  return output_path == requested;
#else
  const char* requested_path = std::getenv("LIBBSA_TEST_FAIL_BA2_DX10_PUBLISH_AFTER_BACKUP");
  return requested_path != nullptr && output_path == std::filesystem::path{requested_path};
#endif
}

result<void> validate_target_options(ba2_dx10_target target, const ba2_dx10_writer_options& options) {
  switch (target) {
  case ba2_dx10_target::fallout4:
    return {};
  case ba2_dx10_target::starfield_v3:
    if (options.starfield_compression_method == starfield_deflate_method ||
        options.starfield_compression_method == starfield_lz4_block_method) {
      return {};
    }
    return error{error_code::unsupported, "BA2 DX10 Starfield v3 compression method is unsupported"};
  }
  return error{error_code::invalid_argument, "BA2 DX10 writer target profile is not supported"};
}

result<void> validate_entries(std::span<const ba2_dx10_writer_entry> entries) {
  if (entries.empty()) {
    return error{error_code::invalid_argument, "BA2 DX10 writer requires at least one texture entry"};
  }

  std::unordered_set<std::string> canonical_paths;
  for (const auto& entry : entries) {
    if (!canonical_paths.insert(entry.archive_path_canonical).second) {
      return error{error_code::format_error, "BA2 DX10 writer has duplicate canonical archive paths"};
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
    if (entry.source.subresources.empty() || entry.source.image_payload_bytes.empty()) {
      return error{error_code::format_error, "BA2 DX10 writer entry has no DDS image payload"};
    }
  }
  return {};
}

} // namespace

result<void> write_ba2_dx10_archive(ba2_dx10_target target,
                                    const ba2_dx10_writer_options& options,
                                    std::span<const ba2_dx10_writer_entry> entries,
                                    std::string_view output_host_path) {
  if (output_host_path.empty()) {
    return error{error_code::invalid_argument, "BA2 DX10 output host path must not be empty"};
  }

  auto target_options = validate_target_options(target, options);
  if (!target_options) {
    return target_options.error();
  }

  const auto output_path = std::filesystem::path{output_host_path};
  auto output_exists = path_exists_noexcept(output_path);
  if (!output_exists) {
    return output_exists.error();
  }
  if (!options.overwrite_existing && output_exists.value()) {
    return error{error_code::io_error, "BA2 DX10 output host path already exists"};
  }

  auto validated = validate_entries(entries);
  if (!validated) {
    return validated.error();
  }

  const auto version = version_for(target);
  auto prepared = prepare_entries(target, options, entries);
  if (!prepared) {
    return prepared.error();
  }

  std::uint64_t file_table_offset = 0;
  auto offsets = assign_payload_offsets(prepared.value(), version, options.deduplicate_payloads, file_table_offset);
  if (!offsets) {
    return offsets.error();
  }

  auto temp_dir = make_unique_publish_directory(output_path);
  if (!temp_dir) {
    return temp_dir.error();
  }
  const auto temp_path = temp_dir.value() / output_path.filename();
  auto written = write_archive_bytes(options, prepared.value(), version, file_table_offset, temp_path);
  if (!written) {
    cleanup_publish_directory(temp_dir.value());
    return written.error();
  }

  std::error_code fs_error;
  if (options.overwrite_existing) {
    output_exists = path_exists_noexcept(output_path);
    if (!output_exists) {
      cleanup_publish_directory(temp_dir.value());
      return output_exists.error();
    }
    if (output_exists.value()) {
      const bool is_regular = std::filesystem::is_regular_file(output_path, fs_error);
      if (fs_error || !is_regular) {
        cleanup_publish_directory(temp_dir.value());
        return error{error_code::io_error, "BA2 DX10 writer refuses to replace non-regular output host path"};
      }

      auto backup_path = reserve_backup_path(output_path);
      if (!backup_path) {
        cleanup_publish_directory(temp_dir.value());
        return backup_path.error();
      }

      // Move the old archive aside before publish so a failed replacement can roll back to the last good file.
      std::filesystem::rename(output_path, backup_path.value(), fs_error);
      if (fs_error) {
        cleanup_publish_directory(temp_dir.value());
        return error{error_code::io_error, "BA2 DX10 writer failed to reserve output backup"};
      }

      if (should_fail_after_backup_for_test(output_path)) {
        std::error_code rollback_error;
        std::filesystem::rename(backup_path.value(), output_path, rollback_error);
        cleanup_publish_directory(temp_dir.value());
        return error{error_code::io_error, "BA2 DX10 writer failed to publish output host path"};
      }

      std::filesystem::rename(temp_path, output_path, fs_error);
      if (fs_error) {
        std::error_code rollback_error;
        std::filesystem::rename(backup_path.value(), output_path, rollback_error);
        cleanup_publish_directory(temp_dir.value());
        return error{error_code::io_error, "BA2 DX10 writer failed to publish output host path"};
      }

      std::filesystem::remove(backup_path.value(), fs_error);
      cleanup_publish_directory(temp_dir.value());
      return {};
    }
  }

  if (!options.overwrite_existing) {
    // POSIX rename would replace a destination created after the initial existence check;
    // copy_options::none preserves the caller's no-overwrite contract at the cost of atomic publish.
    std::filesystem::copy_file(temp_path, output_path, std::filesystem::copy_options::none, fs_error);
    if (fs_error) {
      cleanup_publish_directory(temp_dir.value());
      return error{error_code::io_error, "BA2 DX10 writer failed to publish output host path without overwrite"};
    }
    cleanup_publish_directory(temp_dir.value());
    return {};
  }

  std::filesystem::rename(temp_path, output_path, fs_error);
  if (fs_error) {
    cleanup_publish_directory(temp_dir.value());
    return error{error_code::io_error, "BA2 DX10 writer failed to publish output host path"};
  }
  cleanup_publish_directory(temp_dir.value());
  return {};
}

} // namespace libbsa::formats::ba2
