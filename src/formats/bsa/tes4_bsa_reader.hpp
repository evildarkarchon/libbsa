#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <detail/host_file_path.hpp>

namespace libbsa::formats::bsa {

/// Extracts one TES4-family entry by reopening the already-resolved host
/// archive into a caller-owned sink.
result<void> extract_tes4_bsa_payload_from_file(const detail::host_file_path& host_path,
                                                const entry_metadata& entry, payload_sink& sink);

}  // namespace libbsa::formats::bsa
