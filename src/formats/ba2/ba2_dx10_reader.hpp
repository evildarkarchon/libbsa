#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <detail/host_file_path.hpp>

namespace libbsa::formats::ba2 {

/// Extracts one BA2 DX10 entry as a reconstructed DDS byte stream by reopening
/// the resolved host archive.
///
/// DDS extraction writes the reconstructed DXT10 header first, then each
/// decoded texture chunk in parser-validated DDS order. Chunk codec routing
/// comes only from parsed texture metadata and exact decoded-size validation is
/// applied to every compressed chunk.
result<void> extract_ba2_dx10_payload(const detail::host_file_path& host_path,
                                      const entry_metadata& entry, payload_sink& sink);

}  // namespace libbsa::formats::ba2
