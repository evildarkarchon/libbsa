#include "formats/ba2/ba2_dx10_writer.hpp"

#include <detail/archive_path.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> ba2_dx10_writer::write_to(std::string_view host_path) const {
  return formats::ba2::write_ba2_dx10_archive(state_->target, state_->options, state_->entries, host_path);
}

} // namespace libbsa

namespace libbsa::formats::ba2 {

result<void> write_ba2_dx10_archive(ba2_dx10_target target,
                                    const ba2_dx10_writer_options& options,
                                    std::span<const ba2_dx10_writer_entry> entries,
                                    std::string_view output_host_path) {
  (void)target;
  (void)options;
  (void)entries;
  (void)output_host_path;
  return error{error_code::unsupported, "BA2 DX10 archive serialization is implemented in a later plan"};
}

} // namespace libbsa::formats::ba2
