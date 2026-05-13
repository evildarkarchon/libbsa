#include <libbsa/archive.hpp>

#include "formats/ba2/ba2_format_detector.hpp"
#include "formats/ba2/ba2_dx10_parser.hpp"
#include "formats/ba2/ba2_dx10_reader.hpp"
#include "formats/ba2/ba2_gnrl_parser.hpp"
#include "formats/ba2/ba2_gnrl_reader.hpp"
#include "formats/bsa/bsa_format_detector.hpp"
#include "formats/bsa/tes3_bsa_parser.hpp"
#include "formats/bsa/tes3_bsa_reader.hpp"
#include "formats/bsa/tes4_bsa_parser.hpp"
#include "formats/bsa/tes4_bsa_reader.hpp"

#include <detail/byte_vector.hpp>
#include <detail/host_file.hpp>
#include <detail/host_file_path.hpp>
#include <detail/parallel_work.hpp>
#include <detail/payload_stream.hpp>

#include <cstddef>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace libbsa {

struct archive_reader::state {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
  /// Keeps caller UTF-8 text for diagnostics only; open-time and parser-time host-file I/O stay on the resolved path.
  detail::host_file_path host_path;
  bool is_ba2_dx10{false};
};

archive_reader::archive_reader(archive_metadata metadata)
    : state_(std::make_shared<state>(state{metadata, {}, {}})) {}

namespace {

class vector_payload_sink final : public payload_sink {
 public:
  explicit vector_payload_sink(std::uint64_t expected_size) {
    if (expected_size <= static_cast<std::uint64_t>(std::vector<std::byte>{}.max_size())) {
      auto reserved = detail::reserve_byte_vector(bytes_, static_cast<std::size_t>(expected_size), "extracted payload");
      if (!reserved) {
        allocation_error_ = reserved.error();
      }
    } else {
      allocation_error_ = detail::byte_vector_allocation_error("extracted payload");
    }
  }

  result<std::size_t> write(std::span<const std::byte> bytes) override {
    if (allocation_error_.has_value()) {
      return *allocation_error_;
    }
    auto appended = detail::append_byte_vector(bytes_, bytes, "extracted payload");
    if (!appended) {
      return appended.error();
    }
    return bytes.size();
  }

  [[nodiscard]] std::vector<std::byte> finish() && { return std::move(bytes_); }

 private:
  std::vector<std::byte> bytes_;
  std::optional<error> allocation_error_;
};

detail::host_file_context archive_open_host_context() noexcept {
  return detail::host_file_context{"failed to open archive host path",
                                   "failed to determine archive host path size",
                                   "failed while reading archive host path",
                                   "archive host path changed while reading",
                                   "archive host path bytes"};
}

result<std::vector<std::byte>> read_detection_prefix(const detail::host_file_path& host_path) {
  return detail::read_host_file_prefix(host_path, 36U, archive_open_host_context());
}

result<std::uint64_t> archive_file_size(const detail::host_file_path& host_path) {
  return detail::inspect_host_file_size(host_path, archive_open_host_context());
}

// Bulk extraction resolves metadata once per unique request, so payload dispatch stays separate from lookup.
result<void> extract_entry_payload(const archive_metadata& metadata,
                                   bool is_ba2_dx10,
                                   const detail::host_file_path& host_path,
                                   const entry_metadata& entry,
                                   payload_sink& sink) {
  if (metadata.variant == archive_variant::tes3) {
    return formats::bsa::extract_tes3_bsa_payload(host_path, entry, sink);
  }
  if (metadata.type == archive_type::ba2) {
    return is_ba2_dx10 ? formats::ba2::extract_ba2_dx10_payload(host_path, entry, sink)
                       : formats::ba2::extract_ba2_gnrl_payload(host_path, entry, sink);
  }
  return formats::bsa::extract_tes4_bsa_payload_from_file(host_path, entry, sink);
}

struct bulk_request_group {
  std::string path;
  std::vector<std::size_t> result_indices;
};

} // namespace

result<archive_reader> archive_reader::open(std::string_view host_path) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }

  auto resolved_host_path = detail::resolve_host_file_path(host_path);
  if (!resolved_host_path) {
    return resolved_host_path.error();
  }
  // Once the public UTF-8 text resolves successfully, this shared path object is the only open/parser I/O route.

  auto prefix = read_detection_prefix(resolved_host_path.value());
  if (!prefix) {
    return prefix.error();
  }

  if (prefix.value().size() >= 4U && prefix.value()[0] == static_cast<std::byte>(static_cast<unsigned char>('B')) &&
      prefix.value()[1] == static_cast<std::byte>(static_cast<unsigned char>('T')) &&
      prefix.value()[2] == static_cast<std::byte>(static_cast<unsigned char>('D')) &&
      prefix.value()[3] == static_cast<std::byte>(static_cast<unsigned char>('X'))) {
    auto detected_ba2 = formats::ba2::detect_ba2_format(prefix.value());
    if (!detected_ba2) {
      return detected_ba2.error();
    }

    auto archive_size = archive_file_size(resolved_host_path.value());
    if (!archive_size) {
      return archive_size.error();
    }
    if (detected_ba2.value().is_dx10) {
      auto ba2_archive = formats::ba2::parse_ba2_dx10_archive_file(resolved_host_path.value(),
                                                                    archive_size.value(),
                                                                    detected_ba2.value());
      if (!ba2_archive) {
        return ba2_archive.error();
      }

      archive_reader reader{ba2_archive.value().metadata};
      reader.state_ = std::make_shared<state>(state{ba2_archive.value().metadata,
                                                    std::move(ba2_archive.value().entries),
                                                    std::move(resolved_host_path).value(),
                                                    true});
      return reader;
    }

    auto ba2_archive = formats::ba2::parse_ba2_gnrl_archive_file(resolved_host_path.value(),
                                                                  archive_size.value(),
                                                                  detected_ba2.value());
    if (!ba2_archive) {
      return ba2_archive.error();
    }
    archive_reader reader{ba2_archive.value().metadata};
    reader.state_ = std::make_shared<state>(state{ba2_archive.value().metadata,
                                                  std::move(ba2_archive.value().entries),
                                                  std::move(resolved_host_path).value(),
                                                  false});
    return reader;
  }

  auto detected = formats::bsa::detect_bsa_format(prefix.value());
  if (!detected) {
    return detected.error();
  }

  auto archive_size = archive_file_size(resolved_host_path.value());
  if (!archive_size) {
    return archive_size.error();
  }
  if (detected.value().variant == archive_variant::tes3) {
    auto tes3_archive =
        formats::bsa::parse_tes3_bsa_archive_file(resolved_host_path.value(), archive_size.value(), detected.value());
    if (!tes3_archive) {
      return tes3_archive.error();
    }

    archive_reader reader{tes3_archive.value().metadata};
    reader.state_ = std::make_shared<state>(state{tes3_archive.value().metadata,
                                                  std::move(tes3_archive.value().entries),
                                                  std::move(resolved_host_path).value()});
    return reader;
  }

  auto tes4_archive =
      formats::bsa::parse_tes4_bsa_archive_file(resolved_host_path.value(), archive_size.value(), detected.value());
  if (!tes4_archive) {
    return tes4_archive.error();
  }

  archive_reader reader{tes4_archive.value().metadata};
  reader.state_ = std::make_shared<state>(state{tes4_archive.value().metadata,
                                                std::move(tes4_archive.value().entries),
                                                std::move(resolved_host_path).value()});
  return reader;
}

result<archive_metadata> archive_reader::metadata() const {
  if (!state_) {
    return error{error_code::unsupported, "archive reader is not open"};
  }
  return state_->metadata;
}

result<std::vector<entry_metadata>> archive_reader::entries() const {
  if (!state_) {
    return error{error_code::unsupported, "archive reader is not open"};
  }
  if (state_->metadata.variant == archive_variant::tes3) {
    return formats::bsa::tes3_bsa_entries(state_->entries);
  }
  if (state_->metadata.type == archive_type::ba2) {
    return state_->is_ba2_dx10 ? formats::ba2::ba2_dx10_entries(state_->entries) : formats::ba2::ba2_gnrl_entries(state_->entries);
  }
  return formats::bsa::tes4_bsa_entries(state_->entries);
}

result<std::optional<entry_metadata>> archive_reader::find(std::string_view path) const {
  if (!state_) {
    return error{error_code::unsupported, "archive reader is not open"};
  }
  if (state_->metadata.variant == archive_variant::tes3) {
    return formats::bsa::find_tes3_bsa_entry(state_->entries, path);
  }
  if (state_->metadata.type == archive_type::ba2) {
    return state_->is_ba2_dx10 ? formats::ba2::find_ba2_dx10_entry(state_->entries, path)
                               : formats::ba2::find_ba2_gnrl_entry(state_->entries, path);
  }
  return formats::bsa::find_tes4_bsa_entry(state_->entries, path);
}

result<bool> archive_reader::contains(std::string_view path) const {
  if (!state_) {
    return error{error_code::unsupported, "archive reader is not open"};
  }
  if (state_->metadata.variant == archive_variant::tes3) {
    return formats::bsa::contains_tes3_bsa_entry(state_->entries, path);
  }
  if (state_->metadata.type == archive_type::ba2) {
    return state_->is_ba2_dx10 ? formats::ba2::contains_ba2_dx10_entry(state_->entries, path)
                               : formats::ba2::contains_ba2_gnrl_entry(state_->entries, path);
  }
  return formats::bsa::contains_tes4_bsa_entry(state_->entries, path);
}

result<void> archive_reader::extract(std::string_view path, payload_sink& sink) const {
  if (!state_) {
    return error{error_code::unsupported, "archive reader is not open"};
  }
  auto found = state_->metadata.variant == archive_variant::tes3
                    ? formats::bsa::find_tes3_bsa_entry(state_->entries, path)
                    : state_->metadata.type == archive_type::ba2
                          ? (state_->is_ba2_dx10 ? formats::ba2::find_ba2_dx10_entry(state_->entries, path)
                                                 : formats::ba2::find_ba2_gnrl_entry(state_->entries, path))
                          : formats::bsa::find_tes4_bsa_entry(state_->entries, path);
  if (!found) {
    return found.error();
  }
  if (!found.value()) {
    return error{error_code::not_found, "archive path was not found"};
  }
  return extract_entry_payload(state_->metadata, state_->is_ba2_dx10, state_->host_path, *found.value(), sink);
}

result<std::vector<std::byte>> archive_reader::extract_bytes(std::string_view path) const {
  if (!state_) {
    return error{error_code::unsupported, "archive reader is not open"};
  }

  auto found = find(path);
  if (!found) {
    return found.error();
  }
  if (!found.value()) {
    return error{error_code::not_found, "archive path was not found"};
  }

  auto materialized_size = detail::checked_materialized_payload_size(found.value()->raw_size, "extracted payload");
  if (!materialized_size) {
    return materialized_size.error();
  }

  // Keep the convenience API bounded by the parser-derived size for exactly one entry.
  vector_payload_sink sink{found.value()->raw_size};
  auto extracted =
      extract_entry_payload(state_->metadata, state_->is_ba2_dx10, state_->host_path, *found.value(), sink);
  if (!extracted) {
    return extracted.error();
  }
  return std::move(sink).finish();
}

result<std::vector<bulk_extract_entry_result>> archive_reader::extract_entries(
    std::span<const bulk_extract_request> requests,
    bulk_extract_sink_factory& sink_factory,
    bulk_extract_options options) const {
  if (!state_) {
    return error{error_code::unsupported, "archive reader is not open"};
  }
  if (options.worker_count == 0U) {
    return error{error_code::invalid_argument, "worker_count must be greater than zero"};
  }

  std::vector<bulk_extract_entry_result> results(requests.size());
  std::vector<bulk_request_group> groups;
  groups.reserve(requests.size());
  std::map<std::string, std::size_t> group_by_path;
  for (std::size_t index = 0; index < requests.size(); ++index) {
    const auto& request = requests[index];
    // Coalesce only caller-supplied exact strings; archive-specific normalization remains inside find().
    const auto [group, inserted] = group_by_path.emplace(request.path, groups.size());
    if (inserted) {
      groups.push_back(bulk_request_group{request.path, std::vector<std::size_t>{index}});
      continue;
    }
    groups[group->second].result_indices.push_back(index);
  }

  const auto copy_group_result = [&](const bulk_request_group& group, const bulk_extract_entry_result& record) {
    for (const auto result_index : group.result_indices) {
      results[result_index] = record;
      results[result_index].path = requests[result_index].path;
    }
  };

  const auto work = [&](std::size_t group_index) -> result<void> {
    const auto& group = groups[group_index];
    bulk_extract_entry_result record;
    record.path = group.path;

    auto found = find(group.path);
    if (!found) {
      record.failure = found.error();
      copy_group_result(group, record);
      return {};
    }
    if (!found.value()) {
      record.failure = error{error_code::not_found, "archive path was not found"};
      copy_group_result(group, record);
      return {};
    }

    record.entry = *found.value();
    auto sink = sink_factory.create(group.path, *record.entry);
    if (!sink) {
      record.failure = sink.error();
      copy_group_result(group, record);
      return {};
    }
    if (!sink.value()) {
      record.failure = error{error_code::invalid_argument, "bulk extraction sink factory returned no sink"};
      copy_group_result(group, record);
      return {};
    }

    auto extracted = extract_entry_payload(state_->metadata,
                                           state_->is_ba2_dx10,
                                           state_->host_path,
                                           *record.entry,
                                           *sink.value());
    if (!extracted) {
      record.failure = extracted.error();
    }
    copy_group_result(group, record);
    return {};
  };

  auto worked = detail::run_indexed_work(groups.size(), options.worker_count, work);
  if (!worked) {
    return worked.error();
  }
  return results;
}

} // namespace libbsa
