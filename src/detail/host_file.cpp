#include <detail/host_file.hpp>

#include <detail/byte_vector.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

namespace libbsa::detail {

namespace {

error io_error(std::string_view message) { return error{error_code::io_error, std::string{message}}; }

result<std::size_t> checked_buffer_size(std::uint64_t size, const host_file_context& context) {
  if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return byte_vector_allocation_error(context.allocation_description);
  }
  return static_cast<std::size_t>(size);
}

result<void> validate_expected_host_file_size(std::string_view host_path,
                                              std::uint64_t expected_size,
                                              const host_file_context& context) {
  auto actual_size = inspect_host_file_size(host_path, context);
  if (!actual_size) {
    return actual_size.error();
  }
  if (actual_size.value() != expected_size) {
    // Size changes are surfaced through caller-supplied diagnostics so each archive flow keeps
    // its existing attribution when mutable host files are detected mid-operation.
    return io_error(context.changed_error);
  }
  return {};
}

result<void> read_exact_bytes(std::ifstream& input, std::span<std::byte> output, const host_file_context& context) {
  while (!output.empty()) {
    const auto requested = std::min<std::size_t>(
        output.size(), static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()));
    input.read(reinterpret_cast<char*>(output.data()), static_cast<std::streamsize>(requested));
    if (input.bad()) {
      return io_error(context.read_error);
    }
    if (input.gcount() != static_cast<std::streamsize>(requested)) {
      return io_error(context.changed_error);
    }
    output = output.subspan(requested);
  }
  return {};
}

result<void> reject_appended_host_file_byte(std::ifstream& input, const host_file_context& context) {
  char extra = '\0';
  if (input.get(extra)) {
    return io_error(context.changed_error);
  }
  if (input.bad()) {
    return io_error(context.read_error);
  }
  return {};
}

} // namespace

result<std::ifstream> open_host_file(std::string_view host_path, const host_file_context& context) {
  std::ifstream input{std::filesystem::path{std::string{host_path}}, std::ios::binary};
  if (!input) {
    return io_error(context.open_error);
  }
  return input;
}

result<std::uint64_t> inspect_host_file_size(std::string_view host_path, const host_file_context& context) {
  const auto path = std::filesystem::path{std::string{host_path}};
  std::error_code fs_error;
  const bool regular_file = std::filesystem::is_regular_file(path, fs_error);
  if (fs_error || !regular_file) {
    return io_error(context.inspect_error);
  }

  const auto size = std::filesystem::file_size(path, fs_error);
  if (fs_error) {
    return io_error(context.inspect_error);
  }
  return size;
}

result<std::vector<std::byte>> read_host_file_exact(std::string_view host_path,
                                                    std::uint64_t expected_size,
                                                    const host_file_context& context) {
  auto size_valid = validate_expected_host_file_size(host_path, expected_size, context);
  if (!size_valid) {
    return size_valid.error();
  }

  auto size = checked_buffer_size(expected_size, context);
  if (!size) {
    return size.error();
  }
  auto bytes = make_byte_vector(size.value(), context.allocation_description);
  if (!bytes) {
    return bytes.error();
  }

  auto input = open_host_file(host_path, context);
  if (!input) {
    return input.error();
  }
  auto read = read_exact_bytes(input.value(), bytes.value(), context);
  if (!read) {
    return read.error();
  }
  auto stable = reject_appended_host_file_byte(input.value(), context);
  if (!stable) {
    return stable.error();
  }
  return std::move(bytes).value();
}

result<std::vector<std::byte>> read_host_file_exact(std::string_view host_path, const host_file_context& context) {
  auto size = inspect_host_file_size(host_path, context);
  if (!size) {
    return size.error();
  }
  return read_host_file_exact(host_path, size.value(), context);
}

result<std::vector<std::byte>> read_host_file_prefix(std::string_view host_path,
                                                     std::size_t max_bytes,
                                                     const host_file_context& context) {
  auto input = open_host_file(host_path, context);
  if (!input) {
    return input.error();
  }

  auto bytes = make_byte_vector(max_bytes, context.allocation_description);
  if (!bytes) {
    return bytes.error();
  }

  std::size_t offset = 0;
  while (offset < bytes.value().size()) {
    const auto requested = std::min<std::size_t>(
        bytes.value().size() - offset, static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()));
    input.value().read(reinterpret_cast<char*>(bytes.value().data() + offset), static_cast<std::streamsize>(requested));
    if (input.value().bad()) {
      return io_error(context.read_error);
    }
    const auto count = input.value().gcount();
    offset += static_cast<std::size_t>(count);
    if (count != static_cast<std::streamsize>(requested)) {
      break;
    }
  }
  bytes.value().resize(offset);
  return std::move(bytes).value();
}

result<void> for_each_host_file_chunk(std::string_view host_path,
                                      std::uint64_t expected_size,
                                      const host_file_context& context,
                                      const std::function<result<void>(std::span<const std::byte>)>& callback,
                                      std::size_t chunk_size) {
  if (chunk_size == 0U) {
    return error{error_code::invalid_argument, "host file chunk size must be non-zero"};
  }

  auto size_valid = validate_expected_host_file_size(host_path, expected_size, context);
  if (!size_valid) {
    return size_valid.error();
  }

  auto input = open_host_file(host_path, context);
  if (!input) {
    return input.error();
  }

  auto scratch = make_byte_vector(static_cast<std::size_t>(std::min<std::uint64_t>(expected_size, chunk_size)),
                                  context.allocation_description);
  if (!scratch) {
    return scratch.error();
  }

  std::uint64_t remaining = expected_size;
  while (remaining > 0U) {
    const auto requested = static_cast<std::size_t>(
        std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(scratch.value().size())));
    auto read = read_exact_bytes(input.value(), std::span<std::byte>{scratch.value().data(), requested}, context);
    if (!read) {
      return read.error();
    }
    auto handled = callback(std::span<const std::byte>{scratch.value().data(), requested});
    if (!handled) {
      return handled.error();
    }
    remaining -= requested;
  }

  return reject_appended_host_file_byte(input.value(), context);
}

} // namespace libbsa::detail
