#include "formats/ba2/ba2_gnrl_layout.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <detail/host_file.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::ba2
{

  namespace
  {

    struct payload_assignment
    {
      std::uint64_t offset{};
      std::uint32_t stored_size{};
      std::size_t entry_index{};
    };

    struct ba2_gnrl_final_stored_dedupe_key
    {
      std::uint32_t stored_size{};
      std::uint64_t final_stored_dedupe_hash{};

      bool operator<(const ba2_gnrl_final_stored_dedupe_key &other) const noexcept
      {
        if (stored_size != other.stored_size)
        {
          return stored_size < other.stored_size;
        }
        return final_stored_dedupe_hash < other.final_stored_dedupe_hash;
      }
    };

    ba2_gnrl_final_stored_dedupe_key make_ba2_gnrl_final_stored_dedupe_key(
        std::uint64_t final_stored_dedupe_hash,
        std::uint32_t stored_size) noexcept
    {
      return ba2_gnrl_final_stored_dedupe_key{stored_size, final_stored_dedupe_hash};
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

    result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description)
    {
      if (value > std::numeric_limits<std::uint32_t>::max())
      {
        return error{error_code::format_error, std::string{description} + " exceeds UInt32 range"};
      }
      return static_cast<std::uint32_t>(value);
    }

    constexpr detail::host_file_context ba2_gnrl_dedupe_source_context{
        "BA2 GNRL writer failed to open disk source",
        "BA2 GNRL writer failed to inspect disk source size",
        "BA2 GNRL writer failed while comparing disk source",
        "BA2 GNRL disk source changed during dedupe preparation",
        "BA2 GNRL disk source"};

    result<bool> compare_disk_payload_to_bytes(const detail::host_file_path &host_path,
                                               std::span<const std::byte> expected);

    result<bool> compare_disk_payloads(const detail::host_file_path &lhs_path,
                                       const detail::host_file_path &rhs_path,
                                       std::uint32_t expected_size);

  } // namespace

  std::uint32_t ba2_gnrl_version_for(ba2_gnrl_target target) noexcept
  {
    switch (target)
    {
    case ba2_gnrl_target::fallout4:
      return ba2_fallout4_version;
    case ba2_gnrl_target::starfield_v2:
      return ba2_starfield_v2_version;
    case ba2_gnrl_target::starfield_v3:
      return ba2_starfield_v3_version;
    }
    return 0U;
  }

  std::size_t ba2_gnrl_header_size_for(std::uint32_t version) noexcept
  {
    if (version >= ba2_starfield_v3_version)
    {
      return ba2_starfield_v3_header_size;
    }
    if (version >= ba2_starfield_v2_version)
    {
      return ba2_starfield_v2_header_size;
    }
    return ba2_common_header_size;
  }

  result<bool> ba2_gnrl_payloads_equal(const ba2_gnrl_prepared_entry &lhs, const ba2_gnrl_prepared_entry &rhs)
  {
    if (lhs.stream_from_disk && rhs.stream_from_disk)
    {
      return compare_disk_payloads(lhs.resolved_source_path, rhs.resolved_source_path, lhs.raw_size);
    }
    if (lhs.stream_from_disk)
    {
      return compare_disk_payload_to_bytes(lhs.resolved_source_path, rhs.stored_payload);
    }
    if (rhs.stream_from_disk)
    {
      return compare_disk_payload_to_bytes(rhs.resolved_source_path, lhs.stored_payload);
    }
    return lhs.stored_payload == rhs.stored_payload;
  }

  result<void> ba2_gnrl_assign_payload_offsets(std::span<ba2_gnrl_prepared_entry> entries,
                                               std::uint32_t version,
                                               bool deduplicate_payloads,
                                               std::uint64_t &file_table_offset)
  {
    std::uint64_t record_bytes = 0;
    if (entries.size() > std::numeric_limits<std::uint64_t>::max() / ba2_gnrl_record_size)
    {
      return error{error_code::format_error, "BA2 GNRL record table size overflows"};
    }
    record_bytes = static_cast<std::uint64_t>(entries.size()) * ba2_gnrl_record_size;

    std::uint64_t cursor = 0;
    if (!add_fits_u64(ba2_gnrl_header_size_for(version), record_bytes, cursor))
    {
      return error{error_code::format_error, "BA2 GNRL metadata size overflows"};
    }
    const auto first_payload_offset = cursor;

    std::map<ba2_gnrl_final_stored_dedupe_key, std::vector<payload_assignment>> deduplicated_payloads;
    for (std::size_t index = 0; index < entries.size(); ++index)
    {
      auto &entry = entries[index];
      auto stored_size = checked_u32(entry.stream_from_disk ? entry.raw_size : entry.stored_payload.size(),
                                     "BA2 GNRL stored payload size");
      if (!stored_size)
      {
        return stored_size.error();
      }
      if (deduplicate_payloads)
      {
        // D-23 requires dedupe after raw-vs-compressed routing, so this key is the exact byte span
        // the writer would store in the BA2 payload area rather than the caller's source bytes.
        const auto final_stored_dedupe_hash = entry.final_stored_dedupe_hash;
        const auto identity = make_ba2_gnrl_final_stored_dedupe_key(final_stored_dedupe_hash, stored_size.value());
        auto duplicate = deduplicated_payloads.find(identity);
        bool reused_payload = false;
        if (duplicate != deduplicated_payloads.end())
        {
          for (const auto &candidate : duplicate->second)
          {
            auto equal = ba2_gnrl_payloads_equal(entry, entries[candidate.entry_index]);
            if (!equal)
            {
              return equal.error();
            }
            if (equal.value())
            {
              entry.payload_offset = candidate.offset;
              entry.owns_payload_bytes = false;
              reused_payload = true;
              break;
            }
          }
        }
        if (reused_payload)
        {
          continue;
        }
      }

      // Empty BA2 GNRL entries do not own a physical payload span. Point them at the first payload byte so
      // FileTableOffset remains after every real payload while readers validate the zero-length span safely.
      entry.payload_offset = stored_size.value() == 0U ? first_payload_offset : cursor;
      entry.owns_payload_bytes = true;
      if (deduplicate_payloads)
      {
        const auto identity = make_ba2_gnrl_final_stored_dedupe_key(entry.final_stored_dedupe_hash, stored_size.value());
        deduplicated_payloads[identity].push_back(
            payload_assignment{entry.payload_offset, stored_size.value(), index});
      }
      if (!add_fits_u64(cursor, stored_size.value(), cursor))
      {
        return error{error_code::format_error, "BA2 GNRL payload span overflows"};
      }
    }

    file_table_offset = cursor;
    return {};
  }

  namespace
  {

    result<bool> compare_disk_payload_to_bytes(const detail::host_file_path &host_path,
                                               std::span<const std::byte> expected)
    {
      bool equal = true;
      std::size_t offset = 0;
      auto compared = detail::for_each_host_file_chunk(
          host_path,
          expected.size(),
          ba2_gnrl_dedupe_source_context,
          [&](std::span<const std::byte> chunk) -> result<void>
          {
            if (equal && !std::equal(chunk.begin(), chunk.end(), expected.begin() + static_cast<std::ptrdiff_t>(offset)))
            {
              equal = false;
            }
            offset += chunk.size();
            return {};
          });
      if (!compared)
      {
        return compared.error();
      }
      return equal;
    }

    result<bool> compare_disk_payloads(const detail::host_file_path &lhs_path,
                                       const detail::host_file_path &rhs_path,
                                       std::uint32_t expected_size)
    {
      auto lhs_size = detail::inspect_host_file_size(lhs_path, ba2_gnrl_dedupe_source_context);
      if (!lhs_size)
      {
        return lhs_size.error();
      }
      auto rhs_size = detail::inspect_host_file_size(rhs_path, ba2_gnrl_dedupe_source_context);
      if (!rhs_size)
      {
        return rhs_size.error();
      }
      if (lhs_size.value() != expected_size || rhs_size.value() != expected_size)
      {
        return error{error_code::io_error, "BA2 GNRL disk source changed during dedupe preparation"};
      }

      auto lhs = detail::open_host_file(lhs_path, ba2_gnrl_dedupe_source_context);
      if (!lhs)
      {
        return lhs.error();
      }
      auto rhs = detail::open_host_file(rhs_path, ba2_gnrl_dedupe_source_context);
      if (!rhs)
      {
        return rhs.error();
      }

      std::array<char, 64U * 1024U> lhs_scratch{};
      std::array<char, 64U * 1024U> rhs_scratch{};
      std::uint64_t remaining = expected_size;
      while (remaining > 0U)
      {
        const auto requested = static_cast<std::size_t>(
            std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(lhs_scratch.size())));
        lhs.value().read(lhs_scratch.data(), static_cast<std::streamsize>(requested));
        rhs.value().read(rhs_scratch.data(), static_cast<std::streamsize>(requested));
        if (lhs.value().gcount() != static_cast<std::streamsize>(requested) ||
            rhs.value().gcount() != static_cast<std::streamsize>(requested))
        {
          return error{error_code::io_error, "BA2 GNRL disk source changed during dedupe preparation"};
        }
        if (!std::equal(lhs_scratch.begin(), lhs_scratch.begin() + static_cast<std::ptrdiff_t>(requested), rhs_scratch.begin()))
        {
          return false;
        }
        remaining -= requested;
      }

      // A file that grew after preparation can otherwise compare equal for the prepared prefix and corrupt offsets.
      char lhs_extra = '\0';
      char rhs_extra = '\0';
      if (lhs.value().get(lhs_extra) || rhs.value().get(rhs_extra))
      {
        return error{error_code::io_error, "BA2 GNRL disk source changed during dedupe preparation"};
      }
      if (lhs.value().bad() || rhs.value().bad())
      {
        return error{error_code::io_error, "BA2 GNRL writer failed while comparing disk sources"};
      }
      return true;
    }

  } // namespace

} // namespace libbsa::formats::ba2
