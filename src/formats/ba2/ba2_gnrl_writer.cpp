#include "formats/ba2/ba2_gnrl_writer.hpp"

#include "formats/ba2/ba2_gnrl_layout.hpp"
#include "formats/ba2/ba2_gnrl_prepare.hpp"
#include "formats/ba2/ba2_gnrl_serialize.hpp"

#include <detail/writer_publish.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa {

struct ba2_gnrl_writer::state {
  ba2_gnrl_target target;
  ba2_gnrl_writer_options options;
  std::vector<formats::ba2::ba2_gnrl_writer_entry> entries;
};

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

  auto entry = formats::ba2::ba2_gnrl_make_writer_entry(archive_path, options);
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
  auto entry = formats::ba2::ba2_gnrl_make_writer_entry(archive_path, options);
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
  return formats::ba2::write_ba2_gnrl_archive(
      state_->target, state_->options, state_->entries, host_path, execution.worker_count);
}

} // namespace libbsa

namespace libbsa::formats::ba2 {

result<void> write_ba2_gnrl_archive(ba2_gnrl_target target,
                                    const ba2_gnrl_writer_options& options,
                                    std::span<const ba2_gnrl_writer_entry> entries,
                                    std::string_view output_host_path,
                                    std::uint32_t worker_count) {
  if (output_host_path.empty()) {
    return error{error_code::invalid_argument, "BA2 GNRL output host path must not be empty"};
  }

  auto target_options = ba2_gnrl_validate_target_options(target, options);
  if (!target_options) {
    return target_options.error();
  }

  const auto output_path = std::filesystem::path{output_host_path};

  auto validated = ba2_gnrl_validate_entries(entries);
  if (!validated) {
    return validated.error();
  }

  const auto version = ba2_gnrl_version_for(target);
  auto prepared = ba2_gnrl_prepare_entries(target, options, entries, worker_count);
  if (!prepared) {
    return prepared.error();
  }

  std::uint64_t file_table_offset = 0;
  auto offsets = ba2_gnrl_assign_payload_offsets(prepared.value(),
                                                 version,
                                                 options.deduplicate_payloads,
                                                 file_table_offset);
  if (!offsets) {
    return offsets.error();
  }

  return detail::publish_writer_output(
      output_path, options.overwrite_existing, "BA2 GNRL writer",
      [&](const std::filesystem::path& temp_path) -> result<void> {
        return ba2_gnrl_write_archive_bytes(target, options, prepared.value(), version, file_table_offset, temp_path);
      });
}

} // namespace libbsa::formats::ba2
