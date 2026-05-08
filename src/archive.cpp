#include <libbsa/archive.hpp>

#include "formats/bsa/bsa_format_detector.hpp"
#include "formats/bsa/tes3_bsa_parser.hpp"
#include "formats/bsa/tes3_bsa_reader.hpp"
#include "formats/bsa/tes4_bsa_parser.hpp"
#include "formats/bsa/tes4_bsa_reader.hpp"

#include <cstddef>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace libbsa {

struct archive_reader::state {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
  std::string host_path;
};

archive_reader::archive_reader(archive_metadata metadata)
    : state_(std::make_shared<state>(state{metadata, {}, {}})) {}

namespace {

class vector_payload_sink final : public payload_sink {
 public:
  explicit vector_payload_sink(std::uint64_t expected_size) {
    if (expected_size <= static_cast<std::uint64_t>(std::vector<std::byte>{}.max_size())) {
      bytes_.reserve(static_cast<std::size_t>(expected_size));
    }
  }

  result<std::size_t> write(std::span<const std::byte> bytes) override {
    if (bytes.size() > bytes_.max_size() - bytes_.size()) {
      return error{error_code::format_error, "extracted payload exceeds platform vector limits"};
    }
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    return bytes.size();
  }

  [[nodiscard]] std::vector<std::byte> finish() && { return std::move(bytes_); }

 private:
  std::vector<std::byte> bytes_;
};

result<std::vector<std::byte>> read_detection_prefix(std::string_view host_path) {
  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path"};
  }

  std::vector<std::byte> bytes(8U);
  input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (input.bad()) {
    return error{error_code::io_error, "failed while reading archive host path"};
  }
  bytes.resize(static_cast<std::size_t>(input.gcount()));
  return bytes;
}

result<std::uint64_t> archive_file_size(std::string_view host_path) {
  std::ifstream input{std::string{host_path}, std::ios::binary | std::ios::ate};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path"};
  }
  const auto size = input.tellg();
  if (size < std::streampos{0}) {
    return error{error_code::io_error, "failed to determine archive host path size"};
  }
  return static_cast<std::uint64_t>(size);
}

result<std::vector<std::byte>> read_stored_payload(std::string_view host_path, const entry_metadata& entry) {
  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path for extraction"};
  }
  if (entry.payload_offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
    return error{error_code::format_error, "BSA payload offset exceeds stream limits"};
  }
  if (entry.stored_size > static_cast<std::uint64_t>(std::vector<std::byte>{}.max_size())) {
    return error{error_code::format_error, "BSA stored payload exceeds platform vector limits"};
  }
  if (entry.stored_size > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) {
    return error{error_code::format_error, "BSA stored payload exceeds stream limits"};
  }

  std::vector<std::byte> payload(static_cast<std::size_t>(entry.stored_size));
  input.seekg(static_cast<std::streamoff>(entry.payload_offset), std::ios::beg);
  if (!input) {
    return error{error_code::io_error, "failed to seek to archive payload"};
  }
  input.read(reinterpret_cast<char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
  if (input.bad()) {
    return error{error_code::io_error, "failed while reading archive payload"};
  }
  if (static_cast<std::size_t>(input.gcount()) != payload.size()) {
    return error{error_code::format_error, "BSA entry payload span is outside the archive"};
  }
  return payload;
}

} // namespace

result<archive_reader> archive_reader::open(std::string_view host_path) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }

  auto prefix = read_detection_prefix(host_path);
  if (!prefix) {
    return prefix.error();
  }

  auto detected = formats::bsa::detect_bsa_format(prefix.value());
  if (!detected) {
    return detected.error();
  }

  auto archive_size = archive_file_size(host_path);
  if (!archive_size) {
    return archive_size.error();
  }
  if (detected.value().variant == archive_variant::tes3) {
    auto tes3_archive = formats::bsa::parse_tes3_bsa_archive_file(host_path, archive_size.value(), detected.value());
    if (!tes3_archive) {
      return tes3_archive.error();
    }

    archive_reader reader{tes3_archive.value().metadata};
    reader.state_ = std::make_shared<state>(
        state{tes3_archive.value().metadata, std::move(tes3_archive.value().entries), std::string{host_path}});
    return reader;
  }

  auto tes4_archive = formats::bsa::parse_tes4_bsa_archive_file(host_path, archive_size.value(), detected.value());
  if (!tes4_archive) {
    return tes4_archive.error();
  }

  archive_reader reader{tes4_archive.value().metadata};
  reader.state_ = std::make_shared<state>(
      state{tes4_archive.value().metadata, std::move(tes4_archive.value().entries), std::string{host_path}});
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
  return formats::bsa::tes4_bsa_entries(state_->entries);
}

result<std::optional<entry_metadata>> archive_reader::find(std::string_view path) const {
  if (!state_) {
    return error{error_code::unsupported, "archive reader is not open"};
  }
  if (state_->metadata.variant == archive_variant::tes3) {
    return formats::bsa::find_tes3_bsa_entry(state_->entries, path);
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
  return formats::bsa::contains_tes4_bsa_entry(state_->entries, path);
}

result<void> archive_reader::extract(std::string_view path, payload_sink& sink) const {
  if (!state_) {
    return error{error_code::unsupported, "archive reader is not open"};
  }
  auto found = state_->metadata.variant == archive_variant::tes3
                   ? formats::bsa::find_tes3_bsa_entry(state_->entries, path)
                   : formats::bsa::find_tes4_bsa_entry(state_->entries, path);
  if (!found) {
    return found.error();
  }
  if (!found.value()) {
    return error{error_code::not_found, "archive path was not found"};
  }
  auto payload = read_stored_payload(state_->host_path, *found.value());
  if (!payload) {
    return payload.error();
  }
  if (state_->metadata.variant == archive_variant::tes3) {
    return formats::bsa::extract_tes3_bsa_payload(payload.value(), *found.value(), sink);
  }
  return formats::bsa::extract_tes4_bsa_payload(payload.value(), *found.value(), sink);
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

  // Keep the convenience API bounded by the parser-derived size for exactly one entry.
  vector_payload_sink sink{found.value()->raw_size};
  auto extracted = extract(path, sink);
  if (!extracted) {
    return extracted.error();
  }
  return std::move(sink).finish();
}

} // namespace libbsa
