#pragma once

#include <libbsa/writer.hpp>

#include "texture/directxtex_analyzer.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::ba2 {

struct ba2_dx10_writer_entry {
  std::string archive_path_original;
  std::string archive_path_canonical;
  std::vector<std::byte> dds_bytes;
  texture::dds_source_analysis source;
};

result<void> write_ba2_dx10_archive(ba2_dx10_target target,
                                    const ba2_dx10_writer_options& options,
                                    std::span<const ba2_dx10_writer_entry> entries,
                                    std::string_view output_host_path);

} // namespace libbsa::formats::ba2
