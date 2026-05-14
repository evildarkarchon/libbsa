#include "formats/ba2/ba2_dx10_prepare.hpp"

#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_dx10_chunk_assembler.hpp"
#include "formats/ba2/ba2_dx10_snapshot_builder.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace libbsa::formats::ba2 {

namespace {

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

} // namespace

result<ba2_dx10_writer_entry> ba2_dx10_make_writer_entry(std::string_view archive_path,
                                                         std::string_view dds_host_path,
                                                         ba2_dx10_target target,
                                                         const std::filesystem::path& snapshot_dir,
                                                         std::size_t entry_index) {
  // Add-time snapshots become the source of truth for later writes; source DDS files may change or disappear.
  return ba2_dx10_build_writer_entry_snapshot(archive_path, dds_host_path, target, snapshot_dir, entry_index);
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
  return ba2_dx10_assemble_chunk(target, options, source, planned);
}

result<std::vector<ba2_dx10_prepared_entry>> ba2_dx10_prepare_entries(ba2_dx10_target target,
                                                                      const ba2_dx10_writer_options& options,
                                                                      std::span<const ba2_dx10_writer_entry> entries,
                                                                      std::uint32_t worker_count) {
  std::vector<ba2_dx10_prepared_entry> prepared;
  prepared.reserve(entries.size());
  for (const auto& entry : entries) {
    // The chunk seam still owns detail::run_indexed_work over planned chunk indices; this coordinator
    // only serializes entry preparation and final canonical-path sorting.
    auto next = ba2_dx10_assemble_planned_entry(target, options, entry, worker_count);
    if (!next) {
      return next.error();
    }
    prepared.push_back(std::move(next.value()));
  }

  // Sorting happens only after each entry has been fully prepared, so indexed chunk work remains entry-local.
  std::sort(prepared.begin(), prepared.end(), [](const ba2_dx10_prepared_entry& lhs,
                                                 const ba2_dx10_prepared_entry& rhs) {
    return lhs.archive_path_canonical < rhs.archive_path_canonical;
  });
  return prepared;
}

} // namespace libbsa::formats::ba2
