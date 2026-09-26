#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <detail/host_file_path.hpp>

namespace libbsa::formats::ba2 {

/// Extracts one BA2 GNRL entry by reopening the already-resolved host archive
/// into a caller-owned sink.
///
/// The entry's parsed compression metadata selects raw, deflate, or raw
/// LZ4-block handling; BA2 GNRL extraction deliberately never infers codec
/// behavior from names or extensions.
result<void> extract_ba2_gnrl_payload(const detail::host_file_path& host_path,
                                      const entry_metadata& entry, payload_sink& sink);

}  // namespace libbsa::formats::ba2
