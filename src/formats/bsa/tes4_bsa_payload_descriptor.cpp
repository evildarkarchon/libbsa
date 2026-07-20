#include "formats/bsa/tes4_bsa_payload_descriptor.hpp"

namespace libbsa::formats::bsa {
namespace {

bool non_empty_span_intersects_prefix(std::size_t start, std::size_t length,
                                      std::size_t prefix_size) noexcept {
    return length != 0U && start < prefix_size;
}

}  // namespace

result<std::uint32_t> tes4_bsa_stored_payload_size(const tes4_bsa_file_record& record,
                                                   std::size_t archive_size,
                                                   std::size_t metadata_size) {
    const auto stored_size = record.size_flags & ~tes4_bsa_file_size_compression_toggle;
    if (!detail::span_fits(record.offset, stored_size, archive_size)) {
        return error{error_code::format_error,
                     "TES4 BSA entry payload span is outside the archive"};
    }
    // Payload offsets are archive-controlled; non-empty file bytes must not point
    // back into the header or name tables.
    if (non_empty_span_intersects_prefix(record.offset, stored_size, metadata_size)) {
        return error{error_code::format_error, "TES4 BSA entry payload span overlaps metadata"};
    }
    return stored_size;
}

}  // namespace libbsa::formats::bsa
