#include "formats/bsa/tes3_bsa_writer.hpp"

#include <detail/archive_path.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace libbsa {

struct tes3_bsa_writer::state {
  tes3_bsa_writer_options options;
  std::vector<formats::bsa::tes3_writer_entry> entries;
};

namespace {

std::string preserved_archive_path(std::string_view archive_path) {
  std::string preserved{archive_path};
  // TES3 serializes a flat name table, but libbsa still normalizes separators so
  // callers get stable archive keys without losing the caller's path casing.
  std::replace(preserved.begin(), preserved.end(), '\\', '/');
  return preserved;
}

result<formats::bsa::tes3_writer_entry> make_entry(std::string_view archive_path) {
  auto canonical = detail::normalize_archive_path(archive_path);
  if (!canonical) {
    return canonical.error();
  }

  formats::bsa::tes3_writer_entry entry;
  entry.archive_path_original = preserved_archive_path(archive_path);
  entry.archive_path_canonical = std::move(canonical.value().value);
  return entry;
}

} // namespace

tes3_bsa_writer::tes3_bsa_writer() : tes3_bsa_writer(tes3_bsa_writer_options{}) {}

tes3_bsa_writer::tes3_bsa_writer(tes3_bsa_writer_options options)
    : state_(std::make_shared<state>(state{options, {}})) {}

const tes3_bsa_writer_options& tes3_bsa_writer::options() const noexcept { return state_->options; }

result<void> tes3_bsa_writer::add_file(std::string_view archive_path, std::string_view host_path) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "TES3 BSA disk source host path must not be empty"};
  }

  auto entry = make_entry(archive_path);
  if (!entry) {
    return entry.error();
  }

  entry.value().host_path = std::string{host_path};
  entry.value().from_memory = false;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> tes3_bsa_writer::add_bytes(std::string_view archive_path, std::span<const std::byte> bytes) {
  auto entry = make_entry(archive_path);
  if (!entry) {
    return entry.error();
  }

  entry.value().memory_bytes.assign(bytes.begin(), bytes.end());
  entry.value().from_memory = true;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> tes3_bsa_writer::write_to(std::string_view host_path) const {
  return formats::bsa::write_tes3_bsa_archive(state_->options, state_->entries, host_path);
}

} // namespace libbsa

namespace libbsa::formats::bsa {

result<void> write_tes3_bsa_archive(const tes3_bsa_writer_options&,
                                    std::span<const tes3_writer_entry> entries,
                                    std::string_view output_host_path) {
  if (output_host_path.empty()) {
    return error{error_code::invalid_argument, "TES3 BSA output host path must not be empty"};
  }
  if (entries.empty()) {
    return error{error_code::invalid_argument, "TES3 BSA writer requires at least one file entry"};
  }

  return error{error_code::unsupported, "TES3 BSA writer serialization is implemented by a later Phase 10 plan"};
}

} // namespace libbsa::formats::bsa
