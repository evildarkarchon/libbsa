#include "formats/bsa/tes3_bsa_layout.hpp"

#include <limits>
#include <string>
#include <string_view>

namespace libbsa::formats::bsa
{

  namespace
  {

    bool add_fits_u64(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t &total) noexcept
    {
      if (lhs > std::numeric_limits<std::uint64_t>::max() - rhs)
      {
        return false;
      }
      total = lhs + rhs;
      return true;
    }

    result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description)
    {
      if (value > std::numeric_limits<std::uint32_t>::max())
      {
        return error{error_code::format_error, std::string{description} + " exceeds uint32_t limits"};
      }
      return static_cast<std::uint32_t>(value);
    }

    result<std::uint32_t> checked_add_u32(std::uint32_t lhs, std::uint32_t rhs, std::string_view description)
    {
      return checked_u32(static_cast<std::uint64_t>(lhs) + rhs, description);
    }

    result<std::uint32_t> checked_mul_u32(std::uint32_t lhs, std::uint32_t rhs, std::string_view description)
    {
      return checked_u32(static_cast<std::uint64_t>(lhs) * rhs, description);
    }

  } // namespace

  result<void> tes3_assign_raw_offsets(std::span<tes3_prepared_entry> entries)
  {
    std::uint32_t cursor = 0;
    for (auto &entry : entries)
    {
      // On disk TES3 stores data-section-relative raw offsets; readers add the computed data section start back.
      entry.raw_offset = cursor;
      auto next = checked_add_u32(cursor, entry.payload_size, "TES3 BSA payload span");
      if (!next)
      {
        return next.error();
      }
      cursor = next.value();
    }
    return {};
  }

} // namespace libbsa::formats::bsa
