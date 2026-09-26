#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace libbsa::detail {

/// Compresses a payload into an RFC1950 zlib-wrapped DEFLATE stream through the
/// private libdeflate adapter.
///
/// Bethesda archives store zlib-wrapped streams, never bare RFC1951 deflate:
/// `TwbBSArchive.CompressStream` routes `ctZlib` through Delphi's
/// `ZCompressStream`, and `ctZlib` is the compression type every BA2 and every
/// TES4-family BSA v103/v104 selects (`wbBSArchive.pas:1089`, `1301-1335`,
/// `1790-1795`). Emitting raw deflate here produces archives the games cannot
/// read, so libbsa deliberately has no raw-deflate route at all.
result<std::vector<std::byte>> compress_zlib(std::span<const std::byte> input,
                                             int compression_level = 6);

/// Decompresses an RFC1950 zlib-wrapped stream and requires exactly
/// `expected_size` bytes.
///
/// Malformed data or a mismatched output size returns `format_error`, matching
/// the archive metadata validation format parsers need.
///
/// Two shapes found in vanilla retail archives are tolerated, both of which
/// still honour the exact-size contract:
///
/// - An empty payload decoding to zero bytes. `Fallout - Misc.bsa` stores one
///   zero-length file whose compressed payload is zero bytes long, with no zlib
///   header at all.
/// - A stream whose DEFLATE data is complete and yields exactly
///   `expected_size` bytes but whose four-byte Adler-32 trailer is missing or
///   short. `Fallout - Misc.bsa` contains one such entry. This is the case the
///   reference implementation calls out by name: `TwbBSArchive.DecompressBuf`
///   swallows Delphi zlib's `Buffer error` because "it happens in vanilla
///   'Fallout - Misc.bsa'" (`wbBSArchive.pas:1800-1810`). A trailer that is
///   present but fails verification is still rejected, so genuine corruption
///   does not slip through.
result<std::vector<std::byte>> decompress_zlib_exact(std::span<const std::byte> compressed,
                                                     std::size_t expected_size);

}  // namespace libbsa::detail
