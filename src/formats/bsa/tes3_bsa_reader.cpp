#include "formats/bsa/tes3_bsa_reader.hpp"

#include <detail/host_file.hpp>
#include <detail/payload_stream.hpp>

#include <cstddef>
#include <limits>

namespace libbsa::formats::bsa {
namespace {

constexpr std::size_t extraction_chunk_size = 64U * 1024U;

detail::host_file_context tes3_extraction_host_context() noexcept {
    return detail::host_file_context{"failed to open TES3 archive host path for extraction",
                                     "failed to inspect TES3 archive host path for extraction",
                                     "failed while reading TES3 archive payload",
                                     "TES3 archive host path changed while reading payload",
                                     "TES3 archive payload bytes"};
}

result<void> stream_from_host(const detail::host_file_path& host_path, const entry_metadata& entry,
                              payload_sink& sink) {
    if (entry.payload_offset >
        static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
        return error{error_code::format_error, "TES3 BSA payload offset exceeds stream limits"};
    }
    if (entry.stored_size >
        static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) {
        return error{error_code::format_error, "TES3 BSA stored payload exceeds stream limits"};
    }

    auto input = detail::open_host_file(host_path, tes3_extraction_host_context());
    if (!input) {
        return input.error();
    }

    return detail::stream_payload_range(input.value(), entry.payload_offset, entry.stored_size,
                                        sink, extraction_chunk_size, "TES3 archive payload");
}

}  // namespace

result<void> extract_tes3_bsa_payload(const detail::host_file_path& host_path,
                                      const entry_metadata& entry, payload_sink& sink) {
    if (entry.compression != entry_compression::none) {
        return error{error_code::format_error,
                     "TES3 BSA entries must be stored without compression"};
    }
    if (entry.stored_size == 0U) {
        return {};
    }
    return stream_from_host(host_path, entry, sink);
}

}  // namespace libbsa::formats::bsa
