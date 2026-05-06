#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstdint>
#include <optional>

namespace libbsa {

/// Identifies the implementation codec required for an archive payload.
///
/// This type is deliberately libbsa-owned so public callers do not depend on
/// third-party codec headers or path/extension-based codec inference.
enum class compression_algorithm {
    none,
    deflate,
    lz4_frame,
    lz4_block,
};

/// Describes how writer code should choose an entry's archive compression state.
enum class compression_policy {
    archive_default,
    force_compressed,
    force_raw,
};

/// Carries all archive metadata needed to route a packed payload to a codec.
///
/// Starfield BA2 v3 stores raw LZ4 block selection in `compression_method`, so
/// callers must pass the detected header value instead of guessing from paths.
struct payload_codec_request {
    archive_format format{};
    compression_state entry_state{compression_state::archive_default};
    std::optional<std::uint32_t> compression_method;
};

/// Resolves archive format and entry metadata into the exact codec implementation.
///
/// Unsupported combinations return `error_code::unsupported_format` rather than
/// falling back to another codec, preventing frame/block or deflate confusion.
[[nodiscard]] result<compression_algorithm> resolve_payload_codec(const payload_codec_request& request);

/// Resolves a writer policy into the archive-native compression state to emit.
///
/// `archive_default_compressed` is the caller's known archive default flag; it is
/// only used when the policy asks to preserve the archive default behavior.
[[nodiscard]] result<compression_state> resolve_write_compression(archive_format format,
                                                                  compression_policy policy,
                                                                  bool archive_default_compressed);

} // namespace libbsa
