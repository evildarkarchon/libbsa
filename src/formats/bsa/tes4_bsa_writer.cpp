#include "formats/bsa/tes4_bsa_writer.hpp"

#include <detail/archive_path.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace libbsa {

struct tes4_bsa_writer::state {
  tes4_bsa_target target;
  tes4_bsa_writer_options options;
  std::vector<formats::bsa::tes4_writer_entry> entries;
};

namespace {

std::string preserved_archive_path(std::string_view archive_path) {
  std::string preserved{archive_path};
  std::replace(preserved.begin(), preserved.end(), '\\', '/');
  return preserved;
}

result<formats::bsa::tes4_writer_entry> make_entry(std::string_view archive_path,
                                                   entry_compression_policy compression) {
  auto canonical = detail::normalize_archive_path(archive_path);
  if (!canonical) {
    return canonical.error();
  }

  formats::bsa::tes4_writer_entry entry;
  entry.archive_path_original = preserved_archive_path(archive_path);
  entry.archive_path_canonical = std::move(canonical.value().value);
  entry.compression = compression;
  return entry;
}

} // namespace

tes4_bsa_writer::tes4_bsa_writer(tes4_bsa_target target)
    : tes4_bsa_writer(target, tes4_bsa_writer_options{}) {}

tes4_bsa_writer::tes4_bsa_writer(tes4_bsa_target target, tes4_bsa_writer_options options)
    : state_(std::make_shared<state>(state{target, options, {}})) {}

tes4_bsa_target tes4_bsa_writer::target() const noexcept { return state_->target; }

const tes4_bsa_writer_options& tes4_bsa_writer::options() const noexcept { return state_->options; }

result<void> tes4_bsa_writer::add_file(std::string_view archive_path,
                                       std::string_view host_path,
                                       entry_compression_policy compression) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "TES4 BSA disk source host path must not be empty"};
  }

  auto entry = make_entry(archive_path, compression);
  if (!entry) {
    return entry.error();
  }

  entry.value().host_path = std::string{host_path};
  entry.value().from_memory = false;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> tes4_bsa_writer::add_bytes(std::string_view archive_path,
                                        std::span<const std::byte> bytes,
                                        entry_compression_policy compression) {
  auto entry = make_entry(archive_path, compression);
  if (!entry) {
    return entry.error();
  }

  entry.value().memory_bytes.assign(bytes.begin(), bytes.end());
  entry.value().from_memory = true;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> tes4_bsa_writer::write_to(std::string_view host_path) const {
  return formats::bsa::write_tes4_bsa_archive(state_->target, state_->options, state_->entries, host_path);
}

} // namespace libbsa

namespace libbsa::formats::bsa {

namespace {

result<void> validate_entries(std::span<const tes4_writer_entry> entries) {
  if (entries.empty()) {
    return error{error_code::invalid_argument, "TES4 BSA writer requires at least one file entry"};
  }

  std::unordered_set<std::string> canonical_paths;
  for (const auto& entry : entries) {
    if (!canonical_paths.insert(entry.archive_path_canonical).second) {
      return error{error_code::format_error, "TES4 BSA writer has duplicate canonical archive paths"};
    }

    if (!entry.from_memory) {
      std::ifstream input{entry.host_path, std::ios::binary};
      if (!input) {
        return error{error_code::io_error, "TES4 BSA writer failed to open disk source"};
      }
    }
  }

  return {};
}

} // namespace

result<void> write_tes4_bsa_archive(tes4_bsa_target,
                                    const tes4_bsa_writer_options& options,
                                    std::span<const tes4_writer_entry> entries,
                                    std::string_view output_host_path) {
  if (output_host_path.empty()) {
    return error{error_code::invalid_argument, "TES4 BSA output host path must not be empty"};
  }

  if (!options.overwrite_existing && std::filesystem::exists(std::filesystem::path{output_host_path})) {
    return error{error_code::io_error, "TES4 BSA output host path already exists"};
  }

  auto validated = validate_entries(entries);
  if (!validated) {
    return validated.error();
  }

  // Plan 02 establishes state and validation only; Plan 03 replaces this marker
  // with reader-reopenable TES4-family table and payload serialization.
  std::ofstream output{std::string{output_host_path}, std::ios::binary | std::ios::trunc};
  if (!output) {
    return error{error_code::io_error, "TES4 BSA writer failed to create output host path"};
  }
  output.write("BSA\0", 4);
  if (!output) {
    return error{error_code::io_error, "TES4 BSA writer failed while writing output host path"};
  }
  return {};
}

} // namespace libbsa::formats::bsa
