#include <libbsa/archive.hpp>

namespace libbsa {

result<archive_reader> archive_reader::open(std::string_view host_path) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }

  return error{error_code::unsupported,
               "archive detection and parsing are not implemented in Phase 1"};
}

result<archive_metadata> archive_reader::metadata() const {
  return error{error_code::unsupported, "archive metadata is not implemented until Phase 3 reader state exists"};
}

result<std::vector<entry_metadata>> archive_reader::entries() const {
  return error{error_code::unsupported, "archive entries are not implemented until Phase 3 reader state exists"};
}

result<std::optional<entry_metadata>> archive_reader::find(std::string_view path) const {
  if (path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }

  return error{error_code::unsupported, "archive lookup is not implemented until Phase 3 reader state exists"};
}

result<bool> archive_reader::contains(std::string_view path) const {
  if (path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }

  return error{error_code::unsupported, "archive contains lookup is not implemented until Phase 3 reader state exists"};
}

result<void> archive_reader::extract(std::string_view path, payload_sink& sink) const {
  (void)sink;
  if (path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }

  return error{error_code::unsupported, "archive extraction is not implemented until Phase 3 reader state exists"};
}

result<std::vector<std::byte>> archive_reader::extract_bytes(std::string_view path) const {
  if (path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }

  return error{error_code::unsupported, "archive byte extraction is not implemented until Phase 3 reader state exists"};
}

} // namespace libbsa
