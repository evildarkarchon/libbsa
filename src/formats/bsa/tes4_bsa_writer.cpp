#include "formats/bsa/tes4_bsa_writer.hpp"

#include "formats/bsa/tes4_bsa_layout.hpp"
#include "formats/bsa/tes4_bsa_prepare.hpp"
#include "formats/bsa/tes4_bsa_serialize.hpp"

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

struct tes4_bsa_writer::state {
  tes4_bsa_target target;
  tes4_bsa_writer_options options;
  std::vector<formats::bsa::tes4_writer_entry> entries;
};

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

  auto entry = formats::bsa::tes4_make_writer_entry(archive_path, compression);
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
  auto entry = formats::bsa::tes4_make_writer_entry(archive_path, compression);
  if (!entry) {
    return entry.error();
  }

  entry.value().memory_bytes.assign(bytes.begin(), bytes.end());
  entry.value().from_memory = true;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> tes4_bsa_writer::write_to(std::string_view host_path) const {
  return write_to(host_path, write_execution_options{});
}

result<void> tes4_bsa_writer::write_to(std::string_view host_path, write_execution_options execution) const {
  if (execution.worker_count == 0U) {
    return error{error_code::invalid_argument, "TES4 BSA writer worker_count must be positive"};
  }
  return formats::bsa::write_tes4_bsa_archive(
      state_->target, state_->options, state_->entries, host_path, execution.worker_count);
}

} // namespace libbsa

namespace libbsa::formats::bsa {

result<void> write_tes4_bsa_archive(tes4_bsa_target target,
                                    const tes4_bsa_writer_options& options,
                                    std::span<const tes4_writer_entry> entries,
                                    std::string_view output_host_path,
                                    std::uint32_t worker_count) {
  if (output_host_path.empty()) {
    return error{error_code::invalid_argument, "TES4 BSA output host path must not be empty"};
  }
  if (worker_count == 0U) {
    return error{error_code::invalid_argument, "TES4 BSA writer worker_count must be positive"};
  }

  const auto output_path = std::filesystem::path{output_host_path};

  auto validated = tes4_validate_entries(entries);
  if (!validated) {
    return validated.error();
  }

  auto version = tes4_version_for(target);
  if (!version) {
    return version.error();
  }

  const bool archive_default_is_compressed = tes4_archive_default_compressed(target, options.compression_policy);
  const bool emit_embedded_names = tes4_should_emit_embedded_names(options, version.value());

  std::uint32_t file_flags = 0U;
  auto folders = tes4_prepare_folders(entries,
                                     target,
                                     archive_default_is_compressed,
                                     emit_embedded_names,
                                     version.value(),
                                     worker_count,
                                     file_flags);
  if (!folders) {
    return folders.error();
  }

  auto layout = tes4_assign_offsets(folders.value(), version.value(), options.deduplicate_payloads);
  if (!layout) {
    return layout.error();
  }

  return detail::publish_writer_output(
      output_path, options.overwrite_existing, "TES4 BSA writer",
      [&](const std::filesystem::path& temp_path) -> result<void> {
        return tes4_write_archive_bytes(folders.value(),
                                        version.value(),
                                        archive_default_is_compressed,
                                        emit_embedded_names,
                                        file_flags,
                                        layout.value(),
                                        temp_path);
      });
}

} // namespace libbsa::formats::bsa
