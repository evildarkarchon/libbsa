#pragma once

#include "formats/ba2/ba2_dx10_prepare.hpp"

#include "texture/dds_layout.hpp"

#include <cstdint>

namespace libbsa::formats::ba2 {

/// Assembles and compresses one BA2 DX10 texture chunk from writer-owned
/// snapshot files. Snapshot bytes are streamed into only the current chunk
/// buffer to preserve bounded-memory staging.
result<ba2_dx10_prepared_chunk> ba2_dx10_assemble_chunk(
    ba2_dx10_target target, const ba2_dx10_writer_options& options,
    const ba2_dx10_writer_entry& source, const texture::planned_texture_chunk& planned);

/// Plans BA2 DX10 chunks for one entry and prepares them with indexed
/// work-result placement.
result<ba2_dx10_prepared_entry> ba2_dx10_assemble_planned_entry(
    ba2_dx10_target target, const ba2_dx10_writer_options& options,
    const ba2_dx10_writer_entry& entry, std::uint32_t worker_count);

}  // namespace libbsa::formats::ba2
