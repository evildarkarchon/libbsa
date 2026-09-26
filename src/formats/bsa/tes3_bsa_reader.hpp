#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <detail/host_file_path.hpp>

namespace libbsa::formats::bsa {

/// Streams one validated TES3 raw payload by reopening the already-resolved
/// host archive path.
result<void> extract_tes3_bsa_payload(const detail::host_file_path& host_path,
                                      const entry_metadata& entry, payload_sink& sink);

}  // namespace libbsa::formats::bsa
