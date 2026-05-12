#include "formats/bsa/tes3_bsa_writer.hpp"

#include "formats/bsa/tes3_bsa_layout.hpp"
#include "formats/bsa/tes3_bsa_prepare.hpp"
#include "formats/bsa/tes3_bsa_serialize.hpp"

#include <detail/writer_publish.hpp>

#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa {

struct tes3_bsa_writer::state {
  tes3_bsa_writer_options options;
  std::vector<formats::bsa::tes3_writer_entry> entries;
};

tes3_bsa_writer::tes3_bsa_writer() : tes3_bsa_writer(tes3_bsa_writer_options{}) {}

tes3_bsa_writer::~tes3_bsa_writer() = default;

tes3_bsa_writer::tes3_bsa_writer(tes3_bsa_writer&&) noexcept = default;

tes3_bsa_writer& tes3_bsa_writer::operator=(tes3_bsa_writer&&) noexcept = default;

tes3_bsa_writer::tes3_bsa_writer(tes3_bsa_writer_options options)
    : state_(std::make_unique<state>(state{options, {}})) {}

const tes3_bsa_writer_options& tes3_bsa_writer::options() const noexcept { return state_->options; }

result<void> tes3_bsa_writer::add_file(std::string_view archive_path, std::string_view host_path) {
  auto validated_host_path = formats::bsa::tes3_validate_host_path(host_path, "TES3 BSA disk source host path");
  if (!validated_host_path) {
    return validated_host_path.error();
  }

  auto entry = formats::bsa::tes3_make_writer_entry(archive_path);
  if (!entry) {
    return entry.error();
  }

  entry.value().host_path = std::string{host_path};
  entry.value().from_memory = false;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> tes3_bsa_writer::add_bytes(std::string_view archive_path, std::span<const std::byte> bytes) {
  auto entry = formats::bsa::tes3_make_writer_entry(archive_path);
  if (!entry) {
    return entry.error();
  }

  entry.value().memory_bytes.assign(bytes.begin(), bytes.end());
  entry.value().from_memory = true;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> tes3_bsa_writer::write_to(std::string_view host_path) const {
  return write_to(host_path, write_execution_options{});
}

result<void> tes3_bsa_writer::write_to(std::string_view host_path, write_execution_options execution) const {
  if (execution.worker_count == 0U) {
    return error{error_code::invalid_argument, "TES3 BSA writer worker_count must be positive"};
  }
  return formats::bsa::write_tes3_bsa_archive(state_->options, state_->entries, host_path);
}

} // namespace libbsa

namespace libbsa::formats::bsa {

result<void> write_tes3_bsa_archive(const tes3_bsa_writer_options& options,
                                    std::span<const tes3_writer_entry> entries,
                                    std::string_view output_host_path) {
  auto validated_host_path = tes3_validate_host_path(output_host_path, "TES3 BSA output host path");
  if (!validated_host_path) {
    return validated_host_path.error();
  }

  const auto output_path = std::filesystem::path{output_host_path};

  auto validated = tes3_validate_entries(entries);
  if (!validated) {
    return validated.error();
  }

  auto prepared = tes3_prepare_entries(entries);
  if (!prepared) {
    return prepared.error();
  }
  auto offsets = tes3_assign_raw_offsets(prepared.value());
  if (!offsets) {
    return offsets.error();
  }

  // D-16 keeps disk-backed source bytes path-backed until this point, but sizes
  // and source readability are validated before the shared helper reserves a publish path.
  return detail::publish_writer_output(
      output_path, options.overwrite_existing, "TES3 BSA writer",
      [&](const std::filesystem::path& temp_path) -> result<void> {
        return tes3_write_archive_bytes(prepared.value(), temp_path);
      });
}

} // namespace libbsa::formats::bsa
