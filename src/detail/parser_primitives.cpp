#include <detail/parser_primitives.hpp>

#include <detail/byte_vector.hpp>

#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>

namespace libbsa::detail
{

  result<void> validate_metadata_count(std::uint64_t count, std::uint64_t limit, std::string_view description)
  {
    if (count > limit)
    {
      return error{error_code::format_error, std::string{description} + " exceeds libbsa metadata count limit"};
    }
    return {};
  }

  bool multiply_fits(std::uint32_t count, std::size_t width, std::size_t &total) noexcept
  {
    if (width != 0U && count > std::numeric_limits<std::size_t>::max() / width)
    {
      return false;
    }
    total = static_cast<std::size_t>(count) * width;
    return true;
  }

  bool add_fits(std::size_t lhs, std::size_t rhs, std::size_t &total) noexcept
  {
    if (lhs > std::numeric_limits<std::size_t>::max() - rhs)
    {
      return false;
    }
    total = lhs + rhs;
    return true;
  }

  bool add_fits_u64(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t &total) noexcept
  {
    if (lhs > std::numeric_limits<std::uint64_t>::max() - rhs)
    {
      return false;
    }
    total = lhs + rhs;
    return true;
  }

  bool span_fits(std::size_t start, std::size_t length, std::size_t total) noexcept
  {
    return start <= total && length <= total - start;
  }

  bool span_fits_u64(std::uint64_t start, std::uint64_t length, std::uint64_t total) noexcept
  {
    return start <= total && length <= total - start;
  }

  result<std::vector<std::byte>> read_file_bytes_at(std::ifstream &input,
                                                    std::uint64_t offset,
                                                    std::size_t count,
                                                    std::string_view description)
  {
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()))
    {
      return error{error_code::format_error, std::string{description} + " offset exceeds stream limits"};
    }
    if (count > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
    {
      return error{error_code::format_error, std::string{description} + " size exceeds stream limits"};
    }

    auto bytes = make_byte_vector(count, description);
    if (!bytes)
    {
      return bytes.error();
    }
    input.clear();
    input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!input)
    {
      return error{error_code::io_error, std::string{"failed to seek while reading "} + std::string{description}};
    }
    input.read(reinterpret_cast<char *>(bytes.value().data()), static_cast<std::streamsize>(bytes.value().size()));
    if (input.bad())
    {
      return error{error_code::io_error, std::string{"failed while reading "} + std::string{description}};
    }
    if (static_cast<std::size_t>(input.gcount()) != bytes.value().size())
    {
      return error{error_code::format_error, std::string{description} + " is truncated"};
    }
    return std::move(bytes).value();
  }

  result<std::string> archive_string_from_bytes(std::span<const std::byte> bytes, std::string_view description)
  {
    std::string result;
    if (bytes.size() > result.max_size())
    {
      return error{error_code::format_error, std::string{description} + " exceeds platform string limits"};
    }

    try
    {
      result.reserve(bytes.size());
      for (const auto value : bytes)
      {
        result.push_back(static_cast<char>(std::to_integer<unsigned char>(value)));
      }
    }
    catch (const std::bad_alloc &)
    {
      return error{error_code::format_error, std::string{description} + " exceeds platform string limits"};
    }
    catch (const std::length_error &)
    {
      return error{error_code::format_error, std::string{description} + " exceeds platform string limits"};
    }
    return result;
  }

  void normalize_display_separators(std::string &value) noexcept
  {
    std::replace(value.begin(), value.end(), '\\', '/');
  }

} // namespace libbsa::detail
