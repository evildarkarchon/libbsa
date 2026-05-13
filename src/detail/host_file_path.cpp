#include <detail/host_file_path.hpp>

namespace libbsa::detail {

result<host_file_path> resolve_host_file_path(std::string_view host_path) {
  return host_file_path{std::string{host_path}, std::filesystem::path{std::string{host_path}}};
}

} // namespace libbsa::detail
