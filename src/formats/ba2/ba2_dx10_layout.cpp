#include "formats/ba2/ba2_dx10_layout.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <limits>
#include <map>
#include <utility>
#include <vector>

namespace libbsa::formats::ba2
{

  namespace
  {

    struct payload_assignment
    {
      std::uint64_t offset{};
    };

    struct dedupe_key
    {
      std::vector<std::byte> stored_payload;
      std::uint32_t raw_size{};
      std::uint32_t packed_size{};
      detail::compression_method compression{};

      bool operator<(const dedupe_key &other) const noexcept
      {
        if (stored_payload != other.stored_payload)
        {
          return stored_payload < other.stored_payload;
        }
        if (raw_size != other.raw_size)
        {
          return raw_size < other.raw_size;
        }
        if (packed_size != other.packed_size)
        {
          return packed_size < other.packed_size;
        }
        return static_cast<int>(compression) < static_cast<int>(other.compression);
      }
    };

    bool add_fits_u64(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t &total) noexcept
    {
      if (lhs > std::numeric_limits<std::uint64_t>::max() - rhs)
      {
        return false;
      }
      total = lhs + rhs;
      return true;
    }

  } // namespace

  result<void> ba2_dx10_assign_payload_offsets(std::span<ba2_dx10_prepared_entry> entries,
                                               std::uint32_t version,
                                               bool deduplicate_payloads,
                                               std::uint64_t &file_table_offset)
  {
    std::uint64_t record_bytes = 0;
    for (const auto &entry : entries)
    {
      std::uint64_t entry_record_bytes = 0;
      if (!add_fits_u64(ba2_dx10_record_size, static_cast<std::uint64_t>(entry.chunks.size()) * ba2_dx10_chunk_header_size,
                        entry_record_bytes) ||
          !add_fits_u64(record_bytes, entry_record_bytes, record_bytes))
      {
        return error{error_code::format_error, "BA2 DX10 record table size overflows"};
      }
    }

    if (!add_fits_u64(ba2_dx10_header_size_for(version), record_bytes, file_table_offset))
    {
      return error{error_code::format_error, "BA2 DX10 metadata size overflows"};
    }

    std::uint64_t cursor = file_table_offset;
    for (const auto &entry : entries)
    {
      std::uint64_t name_bytes = 0;
      if (!add_fits_u64(2U, entry.archive_path_original.size(), name_bytes) || !add_fits_u64(cursor, name_bytes, cursor))
      {
        return error{error_code::format_error, "BA2 DX10 filename table size overflows"};
      }
    }

    std::map<dedupe_key, payload_assignment> deduplicated_payloads;
    for (auto &entry : entries)
    {
      for (auto &chunk : entry.chunks)
      {
        // D-20 requires DX10 dedupe to prove both byte identity and chunk metadata identity. Two
        // chunks only share storage when the final stored bytes, raw size, packed size, and explicit
        // compression route all match; texture dimensions and mip identity remain separate records.
        dedupe_key key{chunk.stored_payload, chunk.raw_size, chunk.packed_size, chunk.compression};
        if (deduplicate_payloads)
        {
          const auto duplicate = deduplicated_payloads.find(key);
          if (duplicate != deduplicated_payloads.end())
          {
            chunk.payload_offset = duplicate->second.offset;
            chunk.owns_payload_bytes = false;
            continue;
          }
        }
        chunk.payload_offset = cursor;
        chunk.owns_payload_bytes = true;
        if (deduplicate_payloads)
        {
          deduplicated_payloads.emplace(std::move(key), payload_assignment{chunk.payload_offset});
        }
        if (!add_fits_u64(cursor, chunk.stored_payload.size(), cursor))
        {
          return error{error_code::format_error, "BA2 DX10 payload span overflows"};
        }
      }
    }
    return {};
  }

} // namespace libbsa::formats::ba2
