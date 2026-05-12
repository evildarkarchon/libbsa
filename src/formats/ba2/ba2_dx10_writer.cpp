#include "formats/ba2/ba2_dx10_writer.hpp"

#include "formats/ba2/ba2_dx10_layout.hpp"
#include "formats/ba2/ba2_dx10_prepare.hpp"
#include "formats/ba2/ba2_dx10_serialize.hpp"

#include <detail/writer_publish.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa {

struct ba2_dx10_writer::state {
  ba2_dx10_target target;
  ba2_dx10_writer_options options;
  std::vector<formats::ba2::ba2_dx10_writer_entry> entries;
  std::filesystem::path snapshot_dir;

  state(ba2_dx10_target selected_target, ba2_dx10_writer_options selected_options)
      : target(selected_target), options(selected_options) {}

  ~state() {
    std::error_code fs_error;
    // Snapshot cleanup is best-effort; write_to returns the primary result before state teardown runs.
    if (!snapshot_dir.empty()) {
      std::filesystem::remove_all(snapshot_dir, fs_error);
    }
  }
};

ba2_dx10_writer::ba2_dx10_writer(ba2_dx10_target target) : ba2_dx10_writer(target, ba2_dx10_writer_options{}) {}

ba2_dx10_writer::~ba2_dx10_writer() = default;

ba2_dx10_writer::ba2_dx10_writer(ba2_dx10_writer&&) noexcept = default;

ba2_dx10_writer& ba2_dx10_writer::operator=(ba2_dx10_writer&&) noexcept = default;

ba2_dx10_writer::ba2_dx10_writer(ba2_dx10_target target, ba2_dx10_writer_options options)
    : state_(std::make_unique<state>(target, options)) {}

ba2_dx10_target ba2_dx10_writer::target() const noexcept { return state_->target; }

const ba2_dx10_writer_options& ba2_dx10_writer::options() const noexcept { return state_->options; }

result<void> ba2_dx10_writer::add_file(std::string_view archive_path, std::string_view dds_host_path) {
  if (dds_host_path.empty()) {
    return error{error_code::invalid_argument, "BA2 DX10 DDS source host path must not be empty"};
  }

  auto snapshot_dir = formats::ba2::ba2_dx10_ensure_snapshot_directory(state_->snapshot_dir);
  if (!snapshot_dir) {
    return snapshot_dir.error();
  }

  auto entry = formats::ba2::ba2_dx10_make_writer_entry(
      archive_path, dds_host_path, state_->target, state_->snapshot_dir, state_->entries.size());
  if (!entry) {
    return entry.error();
  }

  // D-17 keeps add-time ownership while avoiding a long-lived full DDS byte vector in writer state.
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> ba2_dx10_writer::write_to(std::string_view host_path) const {
  return write_to(host_path, write_execution_options{});
}

result<void> ba2_dx10_writer::write_to(std::string_view host_path, write_execution_options execution) const {
  if (execution.worker_count == 0U) {
    return error{error_code::invalid_argument, "BA2 DX10 writer worker_count must be positive"};
  }
  return formats::ba2::write_ba2_dx10_archive(
      state_->target, state_->options, state_->entries, host_path, execution.worker_count);
}

} // namespace libbsa

namespace libbsa::formats::ba2 {

result<void> write_ba2_dx10_archive(ba2_dx10_target target,
                                    const ba2_dx10_writer_options& options,
                                    std::span<const ba2_dx10_writer_entry> entries,
                                    std::string_view output_host_path,
                                    std::uint32_t worker_count) {
  if (output_host_path.empty()) {
    return error{error_code::invalid_argument, "BA2 DX10 output host path must not be empty"};
  }

  auto target_options = ba2_dx10_validate_target_options(target, options);
  if (!target_options) {
    return target_options.error();
  }

  const auto output_path = std::filesystem::path{output_host_path};

  auto validated = ba2_dx10_validate_entries(target, entries);
  if (!validated) {
    return validated.error();
  }

  const auto version = ba2_dx10_version_for(target);
  auto prepared = ba2_dx10_prepare_entries(target, options, entries, worker_count);
  if (!prepared) {
    return prepared.error();
  }

  std::uint64_t file_table_offset = 0;
  auto offsets =
      ba2_dx10_assign_payload_offsets(prepared.value(), version, options.deduplicate_payloads, file_table_offset);
  if (!offsets) {
    return offsets.error();
  }

  return detail::publish_writer_output(
      output_path, options.overwrite_existing, "BA2 DX10 writer",
      [&](const std::filesystem::path& temp_path) -> result<void> {
        return ba2_dx10_write_archive_bytes(options, prepared.value(), version, file_table_offset, temp_path);
      });
}

} // namespace libbsa::formats::ba2
