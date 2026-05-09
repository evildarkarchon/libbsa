#include "formats/ba2/ba2_gnrl_writer.hpp"

#include <detail/archive_path.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

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
  return formats::ba2::write_ba2_gnrl_archive(state_->target, state_->options, state_->entries, host_path);
}

} // namespace libbsa

namespace libbsa::formats::ba2 {

namespace {

constexpr std::uint32_t starfield_deflate_method = 0U;
constexpr std::uint32_t starfield_lz4_block_method = 3U;

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

result<void> write_marker_archive(const std::filesystem::path& output_path) {
  // Plan 08-03 validates writer-owned state only; Plan 08-04 replaces this marker
  // with reader-reopenable BA2 GNRL header, record, payload, and filename tables.
  std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
  if (!output) {
    return error{error_code::io_error, "BA2 GNRL writer failed to create temporary output"};
  }
  constexpr char marker[] = {'B', 'T', 'D', 'X'};
  output.write(marker, sizeof(marker));
  if (!output) {
    return error{error_code::io_error, "BA2 GNRL writer failed while writing temporary output"};
  }
  return {};
}

} // namespace

result<void> write_ba2_gnrl_archive(ba2_gnrl_target target,
                                    const ba2_gnrl_writer_options& options,
                                    std::span<const ba2_gnrl_writer_entry> entries,
                                    std::string_view output_host_path) {
  if (output_host_path.empty()) {
    return error{error_code::invalid_argument, "BA2 GNRL output host path must not be empty"};
  }

  auto target_options = validate_target_options(target, options);
  if (!target_options) {
    return target_options.error();
  }

  const auto output_path = std::filesystem::path{output_host_path};
  if (!options.overwrite_existing && std::filesystem::exists(output_path)) {
    return error{error_code::io_error, "BA2 GNRL output host path already exists"};
  }

  auto validated = validate_entries(entries);
  if (!validated) {
    return validated.error();
  }

  auto temp_path = output_path;
  temp_path += ".tmp";
  std::error_code fs_error;
  std::filesystem::remove(temp_path, fs_error);

  auto written = write_marker_archive(temp_path);
  if (!written) {
    std::filesystem::remove(temp_path, fs_error);
    return written.error();
  }

  if (options.overwrite_existing) {
    std::filesystem::remove(output_path, fs_error);
    if (fs_error) {
      std::filesystem::remove(temp_path, fs_error);
      return error{error_code::io_error, "BA2 GNRL writer failed to replace output host path"};
    }
  }
  std::filesystem::rename(temp_path, output_path, fs_error);
  if (fs_error) {
    std::filesystem::remove(temp_path, fs_error);
    return error{error_code::io_error, "BA2 GNRL writer failed to publish output host path"};
  }
  return {};
}

} // namespace libbsa::formats::ba2
