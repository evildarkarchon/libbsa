#include "formats/bsa/bsa_archive_opening.hpp"

#include "formats/bsa/bsa_archive_source.hpp"
#include "formats/bsa/tes3_bsa_parser.hpp"
#include "formats/bsa/tes4_bsa_constants.hpp"
#include "formats/bsa/tes4_bsa_parser.hpp"
#include "formats/bsa/tes4_bsa_profile.hpp"

#include <detail/binary_io.hpp>

#include <algorithm>
#include <string>
#include <utility>

namespace libbsa::formats::bsa {
namespace {

constexpr std::uint32_t tes3_magic_version = 0x00000100U;

detail::host_file_context bsa_archive_session_context() noexcept {
    return {"failed to open BSA archive read session",
            "failed to determine BSA archive read session size",
            "failed while reading BSA archive metadata",
            "BSA archive changed during metadata reading", "BSA archive metadata"};
}

}  // namespace

result<bsa_archive_source> bsa_archive_source::open(const detail::host_file_path& host_path) {
    // The reference observes metadata through one fmShareDenyWrite stream
    // (wbBSArchive.pas:1077). Our shared session also denies delete access;
    // ownership ends at materialization, rather than the public reader lifetime.
    auto session = detail::stable_host_file_session::open(host_path, bsa_archive_session_context());
    if (!session) {
        return session.error();
    }
    return bsa_archive_source{std::move(session).value()};
}

bsa_archive_source::bsa_archive_source(detail::stable_host_file_session session) noexcept
    : session_{std::move(session)} {}

result<std::vector<std::byte>> bsa_archive_source::read_exact(std::uint64_t offset,
                                                              std::size_t count,
                                                              std::string_view description) const {
    // A declared range outside this stable observation is malformed archive
    // data, not a host change. Preserve the parsers' format_error classification
    // before the lower-level session's io_error check can run.
    if (offset > size() || static_cast<std::uint64_t>(count) > size() - offset) {
        return error{error_code::format_error, std::string{description} + " is truncated"};
    }
    return session_.read_exact_at(offset, count, description);
}

result<opened_bsa_archive> open_bsa_archive(const detail::host_file_path& host_path) {
    auto source = bsa_archive_source::open(host_path);
    if (!source) {
        return source.error();
    }

    // The earlier BSA/BA2 probe only selects a candidate family. Resolve the
    // authoritative magic/version here, from the same observation as all tables.
    auto prefix = source.value().read_exact(
        0U, static_cast<std::size_t>(std::min<std::uint64_t>(source.value().size(), 8U)),
        "BSA fixed header");
    if (!prefix) {
        return prefix.error();
    }
    detail::binary_reader reader{prefix.value()};
    auto magic = reader.read_u32_le();
    if (!magic) {
        return magic.error();
    }
    if (magic.value() == tes3_magic_version) {
        return materialize_tes3_bsa_archive(source.value());
    }
    if (magic.value() != tes4_bsa_magic) {
        return error{error_code::unsupported, "archive bytes do not start with BSA magic"};
    }
    auto version = reader.read_u32_le();
    if (!version) {
        return version.error();
    }
    // Unsupported versions must still win over truncation of the remaining
    // TES4 fixed header, matching the previous file-parser ordering.
    auto profile = make_tes4_bsa_profile_from_header(version.value());
    if (!profile) {
        return profile.error();
    }
    return materialize_tes4_bsa_archive(source.value(), profile.value());
}

}  // namespace libbsa::formats::bsa
