#include <libbsa/archive.hpp>

namespace libbsa {

result<archive_reader> archive_reader::open(std::string_view host_path) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }

  return error{error_code::unsupported,
               "archive detection and parsing are not implemented in Phase 1"};
}

} // namespace libbsa
