#include "formats/ba2/ba2_gnrl_serialize.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <fstream>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>

namespace libbsa::formats::ba2 {

namespace {

class stream_writer {
 public:
  explicit stream_writer(std::ostream& output) : output_(output) {}

  result<void> write_bytes(std::span<const std::byte> bytes) {
    if (bytes.empty()) {
      return {};
    }
    output_.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!output_) {
      return error{error_code::io_error, "BA2 GNRL writer failed while streaming archive bytes"};
    }
    return {};
  }

  result<void> write_u8(std::uint8_t value) {
    const std::byte byte{value};
    return write_bytes(std::span<const std::byte>{&byte, 1U});
  }

  result<void> write_u16_le(std::uint16_t value) {
    const std::array bytes{static_cast<std::byte>(value & 0xFFU), static_cast<std::byte>((value >> 8U) & 0xFFU)};
    return write_bytes(std::span<const std::byte>{bytes.data(), bytes.size()});
  }

  result<void> write_u32_le(std::uint32_t value) {
    const std::array bytes{static_cast<std::byte>(value & 0xFFU),
                           static_cast<std::byte>((value >> 8U) & 0xFFU),
                           static_cast<std::byte>((value >> 16U) & 0xFFU),
                           static_cast<std::byte>((value >> 24U) & 0xFFU)};
    return write_bytes(std::span<const std::byte>{bytes.data(), bytes.size()});
  }

  result<void> write_u64_le(std::uint64_t value) {
    std::array<std::byte, 8U> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index) {
      bytes[index] = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
    }
    return write_bytes(std::span<const std::byte>{bytes.data(), bytes.size()});
  }

 private:
  std::ostream& output_;
};

result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description) {
  if (value > std::numeric_limits<std::uint32_t>::max()) {
    return error{error_code::format_error, std::string{description} + " exceeds UInt32 range"};
  }
  return static_cast<std::uint32_t>(value);
}

result<std::uint16_t> checked_u16(std::uint64_t value, std::string_view description) {
  if (value > std::numeric_limits<std::uint16_t>::max()) {
    return error{error_code::format_error, std::string{description} + " exceeds UInt16 range"};
  }
  return static_cast<std::uint16_t>(value);
}

result<void> write_name(stream_writer& writer, std::string_view name) {
  auto length = checked_u16(name.size(), "BA2 GNRL filename-table entry length");
  if (!length) {
    return length.error();
  }
  auto written = writer.write_u16_le(length.value());
  if (!written) {
    return written.error();
  }
  for (const char ch : name) {
    if (!(written = writer.write_u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch))))) {
      return written.error();
    }
  }
  return {};
}

result<void> stream_disk_payload(const std::string& host_path, std::uint32_t expected_size, std::ostream& output) {
  std::ifstream input{host_path, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "BA2 GNRL writer failed to open disk source"};
  }

  std::array<char, 64U * 1024U> scratch{};
  std::uint64_t remaining = expected_size;
  while (remaining > 0U) {
    const auto requested = static_cast<std::size_t>(
        std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(scratch.size())));
    input.read(scratch.data(), static_cast<std::streamsize>(requested));
    if (input.gcount() != static_cast<std::streamsize>(requested)) {
      return error{error_code::io_error, "BA2 GNRL disk source changed during finalization"};
    }
    output.write(scratch.data(), static_cast<std::streamsize>(requested));
    if (!output) {
      return error{error_code::io_error, "BA2 GNRL writer failed while streaming disk source"};
    }
    remaining -= requested;
  }

  // Metadata offsets and FileTableOffset are fixed before streaming, so an appended byte must fail the write.
  char extra = '\0';
  if (input.get(extra)) {
    return error{error_code::io_error, "BA2 GNRL disk source changed during finalization"};
  }
  if (input.bad()) {
    return error{error_code::io_error, "BA2 GNRL writer failed while reading disk source"};
  }
  return {};
}

} // namespace

result<void> ba2_gnrl_write_archive_bytes(ba2_gnrl_target target,
                                          const ba2_gnrl_writer_options& options,
                                          std::span<const ba2_gnrl_prepared_entry> entries,
                                          std::uint32_t version,
                                          std::uint64_t file_table_offset,
                                          const std::filesystem::path& output_path) {
  std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
  if (!output) {
    return error{error_code::io_error, "BA2 GNRL writer failed to create temporary output"};
  }

  stream_writer writer{output};
  auto written = writer.write_u32_le(ba2_btdx_magic);
  if (!(written = writer.write_u32_le(version)) || !(written = writer.write_u32_le(ba2_gnrl_magic))) {
    return written.error();
  }
  auto file_count = checked_u32(entries.size(), "BA2 GNRL file count");
  if (!file_count) {
    return file_count.error();
  }
  if (!(written = writer.write_u32_le(file_count.value())) || !(written = writer.write_u64_le(file_table_offset))) {
    return written.error();
  }
  if (version >= ba2_starfield_v2_version) {
    // xEdit/BSArchPro initializes Starfield writer Unknown1/Unknown2 to 1/0; options can override these raw
    // compatibility fields while keeping them library-owned and version-gated in public metadata.
    if (!(written = writer.write_u32_le(options.starfield_unknown1)) ||
        !(written = writer.write_u32_le(options.starfield_unknown2))) {
      return written.error();
    }
  }
  if (version >= ba2_starfield_v3_version) {
    // Phase 8 treats v3 GNRL as a structurally supported profile. Method 3 remains the default raw-LZ4-block
    // method for later compression support; raw entries still serialize with PackedSize == 0 in this plan.
    if (!(written = writer.write_u32_le(options.starfield_compression_method))) {
      return written.error();
    }
  }

  for (const auto& entry : entries) {
    if (!(written = writer.write_u32_le(entry.name_hash)) || !(written = writer.write_bytes(entry.extension)) ||
        !(written = writer.write_u32_le(entry.directory_hash)) || !(written = writer.write_u32_le(entry.record_flags)) ||
        !(written = writer.write_u64_le(entry.payload_offset)) || !(written = writer.write_u32_le(entry.packed_size)) ||
        !(written = writer.write_u32_le(entry.raw_size)) || !(written = writer.write_u32_le(ba2_record_sentinel))) {
      return written.error();
    }
  }

  for (const auto& entry : entries) {
    if (!entry.owns_payload_bytes) {
      continue;
    }
    if (entry.stream_from_disk) {
      auto streamed = stream_disk_payload(entry.source_path, entry.raw_size, output);
      if (!streamed) {
        return streamed.error();
      }
    } else {
      if (!(written = writer.write_bytes(entry.stored_payload))) {
        return written.error();
      }
    }
  }

  for (const auto& entry : entries) {
    if (!(written = write_name(writer, entry.archive_path_original))) {
      return written.error();
    }
  }

  (void)target;
  if (!output) {
    return error{error_code::io_error, "BA2 GNRL writer failed while writing temporary output"};
  }
  return {};
}

} // namespace libbsa::formats::ba2
