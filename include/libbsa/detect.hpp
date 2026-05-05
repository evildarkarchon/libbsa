#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>

namespace libbsa {

/// Detects a supported Bethesda archive identity from bounded header bytes.
///
/// The detector reads only the fields needed to identify the archive variant and
/// expose safely available summary values. It returns structured failures for
/// unknown magic, unsupported versions/subtypes, and truncated required fields.
[[nodiscard]] result<archive_summary> detect_archive(const byte_source& source);

} // namespace libbsa
