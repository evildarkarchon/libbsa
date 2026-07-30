#include "formats/ba2/ba2_dx10_chunk_assembler.hpp"

#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_record_identity.hpp"

#include <detail/byte_vector.hpp>
#include <detail/host_file.hpp>
#include <detail/parallel_work.hpp>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa::formats::ba2 {

namespace {

constexpr detail::host_file_context ba2_dx10_snapshot_source_context{
    "BA2 DX10 writer failed to open snapshot temp file",
    "BA2 DX10 writer failed to inspect snapshot temp file",
    "BA2 DX10 writer failed while reading snapshot temp file",
    "BA2 DX10 writer failed while reading snapshot temp file", "BA2 DX10 snapshot temp file"};

result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description) {
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        return error{error_code::format_error, std::string{description} + " exceeds UInt32 range"};
    }
    return static_cast<std::uint32_t>(value);
}

result<std::uint16_t> checked_u16(std::uint64_t value, std::string_view description) {
    if (value > std::numeric_limits<std::uint16_t>::max()) {
        return error{error_code::format_error, std::string{description} + " exceeds UInt16 range"};
    }
    return static_cast<std::uint16_t>(value);
}

result<std::uint8_t> checked_u8(std::uint64_t value, std::string_view description) {
    if (value > std::numeric_limits<std::uint8_t>::max()) {
        return error{error_code::format_error, std::string{description} + " exceeds UInt8 range"};
    }
    return static_cast<std::uint8_t>(value);
}

result<std::size_t> checked_size_t(std::uint64_t value, std::string_view description) {
    if (value > std::numeric_limits<std::size_t>::max()) {
        return error{error_code::format_error,
                     std::string{description} + " exceeds platform size range"};
    }
    return static_cast<std::size_t>(value);
}

result<void> append_snapshot_bytes(std::vector<std::byte>& bytes,
                                   const ba2_dx10_subresource_snapshot& snapshot) {
    return detail::for_each_host_file_chunk(
        snapshot.snapshot_path, snapshot.size, ba2_dx10_snapshot_source_context,
        [&](std::span<const std::byte> chunk) -> result<void> {
            return detail::append_byte_vector(bytes, chunk, "BA2 DX10 raw texture chunk bytes");
        });
}

struct ba2_dx10_chunk_snapshot_batch {
    std::vector<const ba2_dx10_subresource_snapshot*> snapshots;
    std::uint64_t aggregate_size{};
};

/// Collects snapshot subresources for one planned BA2 DX10 chunk in archive mip
/// order.
result<ba2_dx10_chunk_snapshot_batch> collect_chunk_snapshots(
    const ba2_dx10_writer_entry& source, const texture::planned_texture_chunk& chunk) {
    if (chunk.start_mip > chunk.end_mip) {
        return error{error_code::format_error, "BA2 DX10 planned chunk mip range is invalid"};
    }

    ba2_dx10_chunk_snapshot_batch batch;
    batch.snapshots.reserve(static_cast<std::size_t>(chunk.end_mip - chunk.start_mip) + 1U);

    // The writer does not transform DDS image data: it copies existing
    // subresource bytes in the BA2-required chunk layout without resizing,
    // transcoding, mip generation, repair, or reordering.
    for (std::uint32_t mip = chunk.start_mip;; ++mip) {
        const auto found = std::ranges::find_if(
            source.subresources, [&](const ba2_dx10_subresource_snapshot& subresource) {
                return subresource.array_index == chunk.array_index &&
                       subresource.face_index == chunk.face_index && subresource.mip == mip;
            });
        if (found == source.subresources.end()) {
            return error{error_code::format_error,
                         "BA2 DX10 source DDS is missing a planned subresource"};
        }
        if (found->size > std::numeric_limits<std::uint64_t>::max() - batch.aggregate_size) {
            return error{error_code::format_error,
                         "BA2 DX10 planned chunk snapshot size overflows"};
        }
        batch.aggregate_size += found->size;
        batch.snapshots.push_back(&*found);
        if (mip == chunk.end_mip) {
            break;
        }
    }
    return batch;
}

}  // namespace

result<ba2_dx10_prepared_chunk> ba2_dx10_assemble_chunk(
    const ba2_profile& profile, const ba2_dx10_writer_options& options,
    const ba2_dx10_writer_entry& source, const texture::planned_texture_chunk& planned) {
    if (!profile.is_dx10()) {
        return error{error_code::invalid_argument, "BA2 DX10 chunk profile is not DX10"};
    }

    auto snapshots = collect_chunk_snapshots(source, planned);
    if (!snapshots) {
        return snapshots.error();
    }
    if (snapshots.value().aggregate_size != planned.raw_size) {
        return error{error_code::format_error,
                     "BA2 DX10 planned chunk size does not match source DDS bytes"};
    }
    auto raw_capacity = checked_size_t(planned.raw_size, "BA2 DX10 raw chunk size");
    if (!raw_capacity) {
        return raw_capacity.error();
    }

    std::vector<std::byte> raw_bytes;
    auto reserved = detail::reserve_byte_vector(raw_bytes, raw_capacity.value(),
                                                "BA2 DX10 raw texture chunk bytes");
    if (!reserved) {
        return reserved.error();
    }
    // Snapshot files are the source of truth after add_file, and bounded chunk
    // streaming catches missing/truncated temp data before publish can replace
    // caller-visible output.
    for (const auto* snapshot : snapshots.value().snapshots) {
        auto appended = append_snapshot_bytes(raw_bytes, *snapshot);
        if (!appended) {
            return appended.error();
        }
    }
    if (raw_bytes.empty()) {
        return error{error_code::format_error,
                     "BA2 DX10 writer refuses to serialize an empty texture chunk"};
    }
    auto raw_size = checked_u32(raw_bytes.size(), "BA2 DX10 raw chunk size");
    if (!raw_size) {
        return raw_size.error();
    }
    if (raw_size.value() != planned.raw_size) {
        return error{error_code::format_error,
                     "BA2 DX10 planned chunk size does not match source DDS bytes"};
    }

    (void)options;
    const auto method = profile.compressed_payload_method();
    auto compressed = detail::compress_payload(method, raw_bytes);
    if (!compressed) {
        return compressed.error();
    }
    auto packed_size = checked_u32(compressed.value().size(), "BA2 DX10 packed chunk size");
    if (!packed_size) {
        return packed_size.error();
    }
    // The compressed Stored Payload is the only final-byte owner after this
    // point; release assembled DDS bytes before carrying preparation forward.
    std::vector<std::byte>{}.swap(raw_bytes);
    auto payload = detail::stored_payload::from_owned_bytes(std::move(compressed).value());
    auto start_mip = checked_u16(planned.start_mip, "BA2 DX10 chunk start mip");
    auto end_mip = checked_u16(planned.end_mip, "BA2 DX10 chunk end mip");
    if (!start_mip || !end_mip) {
        return !start_mip ? start_mip.error() : end_mip.error();
    }
    return ba2_dx10_prepared_chunk{
        0U,   packed_size.value(), raw_size.value(), start_mip.value(), end_mip.value(), method,
        true, std::move(payload)};
}

result<ba2_dx10_prepared_entry> ba2_dx10_assemble_planned_entry(
    const ba2_profile& profile, const ba2_dx10_writer_options& options,
    const ba2_dx10_writer_entry& entry, std::uint32_t worker_count) {
    texture::dds_texture_layout layout{entry.metadata.width,      entry.metadata.height,
                                       entry.metadata.mip_count,  entry.metadata.dxgi_format,
                                       entry.metadata.array_size, entry.metadata.is_cubemap};
    auto planned_chunks = texture::plan_dx10_chunks(layout, options.max_decoded_chunk_bytes);
    if (!planned_chunks) {
        return planned_chunks.error();
    }
    if (planned_chunks.value().empty()) {
        return error{error_code::format_error, "BA2 DX10 writer planned no chunks for texture"};
    }

    auto identity = make_ba2_record_identity(
        ba2_subtype::dx10,
        ba2_record_path{entry.archive_path_original, entry.archive_path_canonical},
        ba2_record_identity_source::writer_entry);
    if (!identity) {
        return identity.error();
    }

    auto chunk_count = checked_u8(planned_chunks.value().size(), "BA2 DX10 chunk count");
    auto height = checked_u16(entry.metadata.height, "BA2 DX10 texture height");
    auto width = checked_u16(entry.metadata.width, "BA2 DX10 texture width");
    auto mip_count = checked_u8(entry.metadata.mip_count, "BA2 DX10 mip count");
    auto dxgi_format = checked_u8(entry.metadata.dxgi_format, "BA2 DX10 DXGI format");
    if (!chunk_count || !height || !width || !mip_count || !dxgi_format) {
        return !chunk_count
                   ? chunk_count.error()
                   : (!height ? height.error()
                              : (!width ? width.error()
                                        : (!mip_count ? mip_count.error() : dxgi_format.error())));
    }

    ba2_dx10_prepared_entry prepared{
        identity.value().display_path,
        identity.value().canonical_path,
        identity.value().extension,
        identity.value().name_hash,
        identity.value().directory_hash,
        ba2_dx10_unknown_tex_default,
        chunk_count.value(),
        height.value(),
        width.value(),
        mip_count.value(),
        dxgi_format.value(),
        entry.metadata.is_cubemap ? ba2_dx10_cubemap_raw : ba2_dx10_non_cubemap_raw,
        {}};
    std::vector<std::optional<ba2_dx10_prepared_chunk>> chunks_by_index(
        planned_chunks.value().size());
    auto work = [&](std::size_t index) -> result<void> {
        auto chunk =
            ba2_dx10_assemble_chunk(profile, options, entry, planned_chunks.value()[index]);
        if (!chunk) {
            return chunk.error();
        }
        chunks_by_index[index] = std::move(chunk.value());
        return {};
    };
    // Indexed work preserves planned texture order even when worker_count enables
    // parallel assembly.
    auto prepared_chunks =
        detail::run_indexed_work(planned_chunks.value().size(), worker_count, work);
    if (!prepared_chunks) {
        return prepared_chunks.error();
    }
    prepared.chunks.reserve(planned_chunks.value().size());
    for (auto& chunk : chunks_by_index) {
        if (!chunk.has_value()) {
            return error{error_code::io_error, "BA2 DX10 worker did not prepare a texture chunk"};
        }
        prepared.chunks.push_back(std::move(chunk.value()));
    }
    return prepared;
}

}  // namespace libbsa::formats::ba2
