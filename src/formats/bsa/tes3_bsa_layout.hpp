#pragma once

#include "formats/bsa/tes3_bsa_prepare.hpp"

#include <span>

namespace libbsa::formats::bsa {

/// Places every prepared TES3 payload and records where it went.
///
/// Placement runs through the shared Payload Placement module with a base offset
/// of zero and sharing permanently disabled, so every entry receives a distinct
/// data-section-relative location whatever its content. The assigned offsets are
/// 64-bit; the UInt32 a TES3 file record holds is narrowed by serialization,
/// when the record is written.
///
/// Returns a `format_error` only if the payload cursor would leave the 64-bit
/// range. A payload area too large for the UInt32 offset field is rejected at
/// serialization instead, which is the same set of archives the standalone
/// offset assignment refused.
result<void> tes3_place_payloads(std::span<tes3_prepared_entry> entries);

}  // namespace libbsa::formats::bsa
