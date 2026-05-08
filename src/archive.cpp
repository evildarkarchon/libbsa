#include <libbsa/archive.hpp>

#include "formats/bsa/bsa_format_detector.hpp"
#include "formats/bsa/tes4_bsa_parser.hpp"
#include "formats/bsa/tes4_bsa_reader.hpp"

#include <cstddef>
#include <fstream>
#include <iterator>
#include <vector>

namespace libbsa {

struct archive_reader::state {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
  std::vector<std::byte> archive_bytes;
};

archive_reader::archive_reader(archive_metadata metadata)
    : state_(std::make_shared<state>(state{metadata, {}, {}})) {}

namespace {

result<std::vector<std::byte>> read_archive_bytes(std::string_view host_path) {
  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path"};
  }

  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  if (input.bad()) {
    return error{error_code::io_error, "failed while reading archive host path"};
  }
  return bytes;
}

} // namespace

result<archive_reader> archive_reader::open(std::string_view host_path) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }

  auto bytes = read_archive_bytes(host_path);
  if (!bytes) {
    return bytes.error();
  }

  auto detected = formats::bsa::detect_bsa_format(bytes.value());
  if (!detected) {
    return detected.error();
  }

  auto archive = formats::bsa::parse_tes4_bsa_archive(bytes.value(), detected.value());
  if (!archive) {
    return archive.error();
  }

  archive_reader reader{archive.value().metadata};
  reader.state_ = std::make_shared<state>(state{archive.value().metadata, std::move(archive.value().entries), std::move(bytes.value())});
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
  return formats::bsa::tes4_bsa_entries(state_->entries);
}

result<std::optional<entry_metadata>> archive_reader::find(std::string_view path) const {
  if (!state_) {
    return error{error_code::unsupported, "archive reader is not open"};
  }
  return formats::bsa::find_tes4_bsa_entry(state_->entries, path);
}

result<bool> archive_reader::contains(std::string_view path) const {
  if (!state_) {
    return error{error_code::unsupported, "archive reader is not open"};
  }
  return formats::bsa::contains_tes4_bsa_entry(state_->entries, path);
}

result<void> archive_reader::extract(std::string_view path, payload_sink& sink) const {
  if (!state_) {
    return error{error_code::unsupported, "archive reader is not open"};
  }
  return formats::bsa::extract_tes4_bsa_entry(state_->archive_bytes, state_->entries, path, sink);
}

result<std::vector<std::byte>> archive_reader::extract_bytes(std::string_view path) const {
  if (path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }

  return error{error_code::unsupported, "archive byte extraction is not implemented until Phase 3 reader state exists"};
}

} // namespace libbsa
