#include "formats/ba2/ba2_gnrl_layout.hpp"
#include "formats/ba2/ba2_gnrl_prepare.hpp"
#include "formats/ba2/ba2_dx10_layout.hpp"
#include "formats/ba2/ba2_dx10_prepare.hpp"
#include "formats/bsa/tes3_bsa_layout.hpp"
#include "formats/bsa/tes3_bsa_prepare.hpp"
#include "formats/bsa/tes4_bsa_layout.hpp"
#include "formats/bsa/tes4_bsa_prepare.hpp"
#include "formats/bsa/tes4_bsa_serialize.hpp"
#include "texture/dds_layout.hpp"

#include <detail/host_file_path.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace
{

  std::vector<std::byte> bytes_from_text(std::string_view text)
  {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char ch : text)
    {
      bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
  }

  std::filesystem::path stage_test_dir()
  {
    auto path = std::filesystem::temp_directory_path() / "libbsa_writer_stage_tests";
    std::filesystem::create_directories(path);
    return path;
  }

  std::filesystem::path stage_output_path(std::string name) { return stage_test_dir() / std::move(name); }

  void write_stage_binary_file(const std::filesystem::path &path, std::span<const std::byte> bytes)
  {
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    REQUIRE(output.good());
    output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
  }

  std::vector<std::byte> read_stage_binary_file(const std::filesystem::path &path)
  {
    std::ifstream input{path, std::ios::binary};
    REQUIRE(input.good());

    std::vector<std::byte> bytes;
    for (char ch = 0; input.get(ch);)
    {
      bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    REQUIRE_FALSE(input.bad());
    return bytes;
  }

  libbsa::formats::ba2::ba2_gnrl_prepared_entry ba2_gnrl_memory_stage_entry(std::vector<std::byte> bytes,
                                                                            std::uint64_t hash)
  {
    libbsa::formats::ba2::ba2_gnrl_prepared_entry entry;
    entry.archive_path_original = "Meshes/Stage.bin";
    entry.archive_path_canonical = "meshes/stage.bin";
    entry.extension = {std::byte{0x62}, std::byte{0x69}, std::byte{0x6E}, std::byte{0x00}};
    entry.raw_size = static_cast<std::uint32_t>(bytes.size());
    entry.payload_hash = hash;
    entry.stored_payload = std::move(bytes);
    return entry;
  }

  libbsa::formats::ba2::ba2_gnrl_prepared_entry ba2_gnrl_disk_stage_entry(const std::filesystem::path &path,
                                                                          std::uint32_t raw_size)
  {
    libbsa::formats::ba2::ba2_gnrl_prepared_entry entry;
    entry.archive_path_original = "Meshes/Stage.bin";
    entry.archive_path_canonical = "meshes/stage.bin";
    entry.source_path = path.string();
    auto resolved = libbsa::detail::resolve_host_file_path(entry.source_path);
    REQUIRE(resolved.has_value());
    entry.resolved_source_path = std::move(resolved).value();
    entry.extension = {std::byte{0x62}, std::byte{0x69}, std::byte{0x6E}, std::byte{0x00}};
    entry.raw_size = raw_size;
    entry.stream_from_disk = true;
    return entry;
  }

  libbsa::formats::bsa::tes4_prepared_entry tes4_memory_stage_entry(std::string file_name,
                                                                    std::vector<std::byte> bytes)
  {
    libbsa::formats::bsa::tes4_prepared_entry entry;
    entry.folder = "Meshes";
    entry.canonical_folder = "meshes";
    entry.file_name = std::move(file_name);
    entry.file_hash = static_cast<std::uint64_t>(entry.file_name.size());
    entry.stored_size = static_cast<std::uint32_t>(bytes.size());
    entry.stored_payload = std::move(bytes);
    return entry;
  }

  libbsa::formats::bsa::tes4_prepared_entry tes4_disk_stage_entry(const std::filesystem::path &path,
                                                                  std::uint32_t raw_size)
  {
    libbsa::formats::bsa::tes4_prepared_entry entry;
    entry.folder = "Meshes";
    entry.canonical_folder = "meshes";
    entry.file_name = "Disk.nif";
    entry.file_hash = 0xD15CU;
    entry.stored_size = raw_size;
    entry.raw_disk_size = raw_size;
    entry.raw_disk_host_path = path.string();
    entry.stream_raw_disk = true;
    return entry;
  }

  std::vector<std::byte> repeated_bytes(std::size_t size, std::uint8_t seed)
  {
    std::vector<std::byte> bytes(size);
    for (std::size_t index = 0; index < bytes.size(); ++index)
    {
      bytes[index] = static_cast<std::byte>(seed + static_cast<std::uint8_t>(index % 17U));
    }
    return bytes;
  }

  libbsa::formats::ba2::ba2_dx10_writer_entry ba2_dx10_stage_entry(std::string archive_path,
                                                                   const libbsa::texture::dds_texture_layout &layout,
                                                                   std::string snapshot_prefix)
  {
    libbsa::formats::ba2::ba2_dx10_writer_entry entry;
    entry.archive_path_original = archive_path;
    entry.archive_path_canonical = archive_path;
    entry.metadata = libbsa::texture_metadata{layout.width,
                                              layout.height,
                                              layout.mip_count,
                                              layout.dxgi_format,
                                              layout.array_size,
                                              layout.is_cubemap,
                                              0U,
                                              layout.is_cubemap ? 2049U : 2048U,
                                              {}};

    const std::uint32_t face_count = layout.is_cubemap ? 6U : 1U;
    for (std::uint32_t array_index = 0; array_index < layout.array_size; ++array_index)
    {
      for (std::uint32_t face_index = 0; face_index < face_count; ++face_index)
      {
        for (std::uint32_t mip = 0; mip < layout.mip_count; ++mip)
        {
          auto mip_size = libbsa::texture::mip_size_for_format(layout, mip);
          REQUIRE(mip_size.has_value());
          auto bytes = repeated_bytes(static_cast<std::size_t>(mip_size.value()),
                                      static_cast<std::uint8_t>(array_index + face_index + mip + 1U));
          const auto snapshot = stage_output_path(snapshot_prefix + "-" + std::to_string(array_index) + "-" +
                                                  std::to_string(face_index) + "-" + std::to_string(mip) + ".bin");
          write_stage_binary_file(snapshot, bytes);
          entry.subresources.push_back(libbsa::formats::ba2::ba2_dx10_subresource_snapshot{
              array_index, face_index, mip, bytes.size(), snapshot});
        }
      }
    }
    return entry;
  }

  libbsa::formats::ba2::ba2_dx10_prepared_entry ba2_dx10_prepared_stage_entry(std::string path,
                                                                              std::vector<std::byte> payload)
  {
    libbsa::formats::ba2::ba2_dx10_prepared_entry entry;
    entry.archive_path_original = std::move(path);
    entry.archive_path_canonical = entry.archive_path_original;
    entry.chunk_count = 2U;
    auto first = libbsa::formats::ba2::ba2_dx10_prepared_chunk{};
    first.raw_size = static_cast<std::uint32_t>(payload.size());
    first.packed_size = static_cast<std::uint32_t>(payload.size());
    first.compression = libbsa::detail::compression_method::deflate;
    first.stored_payload = payload;
    auto second = first;
    entry.chunks = {std::move(first), std::move(second)};
    return entry;
  }

} // namespace

TEST_CASE("tes3 writer preparation stage prepares and sorts minimal memory entries",
          "[unit][writer-stage][tes3_bsa_writer]")
{
  auto entry = libbsa::formats::bsa::tes3_make_writer_entry("Textures\\Stage\\Probe.dds");
  REQUIRE(entry.has_value());
  entry.value().memory_bytes = bytes_from_text("tes3-stage");
  entry.value().from_memory = true;

  auto prepared = libbsa::formats::bsa::tes3_prepare_entries(std::span<const libbsa::formats::bsa::tes3_writer_entry>{
      &entry.value(), 1U});

  REQUIRE(prepared.has_value());
  REQUIRE(prepared.value().size() == 1U);
  CHECK(prepared.value()[0].archive_path_original == "Textures/Stage/Probe.dds");
  CHECK(prepared.value()[0].payload_size == 10U);
  CHECK(prepared.value()[0].from_memory);
}

TEST_CASE("tes3 writer preparation stage reports malformed disk entries",
          "[unit][writer-stage][tes3_bsa_writer]")
{
  auto entry = libbsa::formats::bsa::tes3_make_writer_entry("meshes/stage/missing.nif");
  REQUIRE(entry.has_value());
  entry.value().host_path = "Z:/definitely/missing/libbsa-stage-source.nif";
  entry.value().from_memory = false;

  auto prepared = libbsa::formats::bsa::tes3_prepare_entries(std::span<const libbsa::formats::bsa::tes3_writer_entry>{
      &entry.value(), 1U});

  REQUIRE_FALSE(prepared.has_value());
  CHECK(prepared.error().code == libbsa::error_code::io_error);
}

TEST_CASE("tes3 writer layout stage assigns raw offsets and rejects oversized spans",
          "[unit][writer-stage][tes3_bsa_writer]")
{
  std::vector<libbsa::formats::bsa::tes3_prepared_entry> entries(2U);
  entries[0].payload_size = 4U;
  entries[1].payload_size = 8U;

  auto assigned = libbsa::formats::bsa::tes3_assign_raw_offsets(entries);

  REQUIRE(assigned.has_value());
  CHECK(entries[0].raw_offset == 0U);
  CHECK(entries[1].raw_offset == 4U);

  entries[0].payload_size = std::numeric_limits<std::uint32_t>::max();
  entries[1].payload_size = 1U;
  auto oversized = libbsa::formats::bsa::tes3_assign_raw_offsets(entries);

  REQUIRE_FALSE(oversized.has_value());
  CHECK(oversized.error().code == libbsa::error_code::format_error);
}

TEST_CASE("ba2 gnrl writer preparation stage prepares minimal memory entries",
          "[unit][writer-stage][ba2_gnrl_writer]")
{
  auto entry = libbsa::formats::ba2::ba2_gnrl_make_writer_entry("Meshes\\Stage\\Probe.bin", {});
  REQUIRE(entry.has_value());
  entry.value().memory_bytes = bytes_from_text("ba2-stage");
  entry.value().from_memory = true;

  libbsa::ba2_gnrl_writer_options options;
  options.compression = libbsa::archive_compression_policy::all_raw;
  auto prepared = libbsa::formats::ba2::ba2_gnrl_prepare_entries(
      libbsa::ba2_gnrl_target::fallout4,
      options,
      std::span<const libbsa::formats::ba2::ba2_gnrl_writer_entry>{&entry.value(), 1U},
      1U);

  REQUIRE(prepared.has_value());
  REQUIRE(prepared.value().size() == 1U);
  CHECK(prepared.value()[0].archive_path_original == "Meshes/Stage/Probe.bin");
  CHECK(prepared.value()[0].raw_size == 9U);
  CHECK(prepared.value()[0].stored_payload == bytes_from_text("ba2-stage"));
  CHECK(prepared.value()[0].extension[0] == std::byte{0x62});
}

TEST_CASE("ba2 gnrl writer layout stage toggles duplicate payload reuse",
          "[unit][writer-stage][ba2_gnrl_writer]")
{
  const auto payload = bytes_from_text("shared");
  const auto version = libbsa::formats::ba2::ba2_gnrl_version_for(libbsa::ba2_gnrl_target::fallout4);

  auto distinct = std::vector{ba2_gnrl_memory_stage_entry(payload, 0xA11CEU),
                              ba2_gnrl_memory_stage_entry(payload, 0xA11CEU)};
  std::uint64_t distinct_file_table_offset = 0;
  auto assigned_distinct =
      libbsa::formats::ba2::ba2_gnrl_assign_payload_offsets(distinct, version, false, distinct_file_table_offset);

  REQUIRE(assigned_distinct.has_value());
  CHECK(distinct[0].payload_offset != distinct[1].payload_offset);
  CHECK(distinct[0].owns_payload_bytes);
  CHECK(distinct[1].owns_payload_bytes);

  auto deduped = std::vector{ba2_gnrl_memory_stage_entry(payload, 0xA11CEU),
                             ba2_gnrl_memory_stage_entry(payload, 0xA11CEU)};
  std::uint64_t deduped_file_table_offset = 0;
  auto assigned_deduped =
      libbsa::formats::ba2::ba2_gnrl_assign_payload_offsets(deduped, version, true, deduped_file_table_offset);

  REQUIRE(assigned_deduped.has_value());
  CHECK(deduped[0].payload_offset == deduped[1].payload_offset);
  CHECK(deduped[0].owns_payload_bytes);
  CHECK_FALSE(deduped[1].owns_payload_bytes);
  CHECK(deduped_file_table_offset < distinct_file_table_offset);
}

TEST_CASE("ba2 gnrl writer layout stage compares prepared payload bytes",
          "[unit][writer-stage][ba2_gnrl_writer]")
{
  const auto payload = bytes_from_text("equal");
  auto memory_equal = libbsa::formats::ba2::ba2_gnrl_payloads_equal(
      ba2_gnrl_memory_stage_entry(payload, 0xBEEFU), ba2_gnrl_memory_stage_entry(payload, 0xBEEFU));

  REQUIRE(memory_equal.has_value());
  CHECK(memory_equal.value());

  const auto source = stage_output_path("ba2-gnrl-payload-equal.bin");
  write_stage_binary_file(source, payload);
  auto disk_equal = libbsa::formats::ba2::ba2_gnrl_payloads_equal(
      ba2_gnrl_disk_stage_entry(source, static_cast<std::uint32_t>(payload.size())),
      ba2_gnrl_memory_stage_entry(payload, 0xBEEFU));

  REQUIRE(disk_equal.has_value());
  CHECK(disk_equal.value());

  auto mismatch = libbsa::formats::ba2::ba2_gnrl_payloads_equal(
      ba2_gnrl_memory_stage_entry(payload, 0xBEEFU), ba2_gnrl_memory_stage_entry(bytes_from_text("other"), 0xBEEFU));

  REQUIRE(mismatch.has_value());
  CHECK_FALSE(mismatch.value());
}

TEST_CASE("tes4 writer preparation stage prepares minimal memory folders",
          "[unit][writer-stage][tes4_bsa_writer]")
{
  auto entry = libbsa::formats::bsa::tes4_make_writer_entry("Meshes\\Stage\\Probe.nif",
                                                            libbsa::entry_compression_policy::raw);
  REQUIRE(entry.has_value());
  entry.value().memory_bytes = bytes_from_text("tes4-stage");
  entry.value().from_memory = true;

  auto version = libbsa::formats::bsa::tes4_version_for(libbsa::tes4_bsa_target::fallout3);
  REQUIRE(version.has_value());
  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::all_raw;
  const bool archive_default = libbsa::formats::bsa::tes4_archive_default_compressed(
      libbsa::tes4_bsa_target::fallout3, options.compression_policy);
  const bool emit_embedded_names = libbsa::formats::bsa::tes4_should_emit_embedded_names(options, version.value());
  std::uint32_t file_flags = 0;

  auto folders = libbsa::formats::bsa::tes4_prepare_folders(
      std::span<const libbsa::formats::bsa::tes4_writer_entry>{&entry.value(), 1U},
      libbsa::tes4_bsa_target::fallout3,
      archive_default,
      emit_embedded_names,
      version.value(),
      1U,
      file_flags);

  REQUIRE(folders.has_value());
  REQUIRE(folders.value().size() == 1U);
  REQUIRE(folders.value()[0].entries.size() == 1U);
  CHECK(folders.value()[0].name == "Meshes\\Stage");
  CHECK(folders.value()[0].entries[0].file_name == "Probe.nif");
  CHECK(folders.value()[0].entries[0].stored_payload == bytes_from_text("tes4-stage"));
  CHECK(file_flags != 0U);
}

TEST_CASE("tes4 writer layout stage toggles duplicate payload reuse",
          "[unit][writer-stage][tes4_bsa_writer]")
{
  const auto payload = bytes_from_text("shared");
  auto version = libbsa::formats::bsa::tes4_version_for(libbsa::tes4_bsa_target::fallout3);
  REQUIRE(version.has_value());

  std::vector<libbsa::formats::bsa::tes4_prepared_folder> distinct{
      libbsa::formats::bsa::tes4_prepared_folder{
          "Meshes",
          1U,
          0U,
          {tes4_memory_stage_entry("A.nif", payload), tes4_memory_stage_entry("B.nif", payload)}}};
  auto distinct_layout = libbsa::formats::bsa::tes4_assign_offsets(distinct, version.value(), false);

  REQUIRE(distinct_layout.has_value());
  CHECK(distinct[0].entries[0].payload_offset != distinct[0].entries[1].payload_offset);
  CHECK(distinct[0].entries[0].owns_payload_bytes);
  CHECK(distinct[0].entries[1].owns_payload_bytes);

  std::vector<libbsa::formats::bsa::tes4_prepared_folder> deduped{
      libbsa::formats::bsa::tes4_prepared_folder{
          "Meshes",
          1U,
          0U,
          {tes4_memory_stage_entry("A.nif", payload), tes4_memory_stage_entry("B.nif", payload)}}};
  auto deduped_layout = libbsa::formats::bsa::tes4_assign_offsets(deduped, version.value(), true);

  REQUIRE(deduped_layout.has_value());
  CHECK(deduped[0].entries[0].payload_offset == deduped[0].entries[1].payload_offset);
  CHECK(deduped[0].entries[0].owns_payload_bytes);
  CHECK_FALSE(deduped[0].entries[1].owns_payload_bytes);
  CHECK(deduped_layout.value().file_count == 2U);
}

TEST_CASE("tes4 writer layout stage compares raw disk and memory payloads",
          "[unit][writer-stage][tes4_bsa_writer]")
{
  const auto payload = bytes_from_text("disk");
  const auto source = stage_output_path("tes4-payload-equal.bin");
  write_stage_binary_file(source, payload);

  auto equal = libbsa::formats::bsa::tes4_stored_payloads_equal(
      tes4_disk_stage_entry(source, static_cast<std::uint32_t>(payload.size())),
      tes4_memory_stage_entry("Memory.nif", payload));

  REQUIRE(equal.has_value());
  CHECK(equal.value());

  auto mismatch = libbsa::formats::bsa::tes4_stored_payloads_equal(
      tes4_disk_stage_entry(source, static_cast<std::uint32_t>(payload.size())),
      tes4_memory_stage_entry("Memory.nif", bytes_from_text("dusk")));

  REQUIRE(mismatch.has_value());
  CHECK_FALSE(mismatch.value());
}

TEST_CASE("tes4 writer serialization stage rejects raw disk source size changes",
          "[unit][writer-stage][tes4_bsa_writer][writer-source-io]")
{
  const auto payload = bytes_from_text("tes4 raw streaming payload");
  auto version = libbsa::formats::bsa::tes4_version_for(libbsa::tes4_bsa_target::fallout3);
  REQUIRE(version.has_value());

  SECTION("source grows after layout")
  {
    const auto source = stage_output_path("tes4-stream-grew.bin");
    write_stage_binary_file(source, payload);
    std::vector<libbsa::formats::bsa::tes4_prepared_folder> folders{
        libbsa::formats::bsa::tes4_prepared_folder{
            "Meshes", 1U, 0U, {tes4_disk_stage_entry(source, static_cast<std::uint32_t>(payload.size()))}}};
    auto layout = libbsa::formats::bsa::tes4_assign_offsets(folders, version.value(), false);
    REQUIRE(layout.has_value());

    auto grown = payload;
    grown.push_back(std::byte{0x21});
    write_stage_binary_file(source, grown);
    auto written = libbsa::formats::bsa::tes4_write_archive_bytes(folders,
                                                                  version.value(),
                                                                  false,
                                                                  false,
                                                                  0U,
                                                                  layout.value(),
                                                                  stage_output_path("tes4-stream-grew.bsa"));

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::io_error);
  }

  SECTION("source shrinks after layout")
  {
    const auto source = stage_output_path("tes4-stream-shrank.bin");
    write_stage_binary_file(source, payload);
    std::vector<libbsa::formats::bsa::tes4_prepared_folder> folders{
        libbsa::formats::bsa::tes4_prepared_folder{
            "Meshes", 1U, 0U, {tes4_disk_stage_entry(source, static_cast<std::uint32_t>(payload.size()))}}};
    auto layout = libbsa::formats::bsa::tes4_assign_offsets(folders, version.value(), false);
    REQUIRE(layout.has_value());

    write_stage_binary_file(source, std::span<const std::byte>{payload.data(), payload.size() - 1U});
    auto written = libbsa::formats::bsa::tes4_write_archive_bytes(folders,
                                                                  version.value(),
                                                                  false,
                                                                  false,
                                                                  0U,
                                                                  layout.value(),
                                                                  stage_output_path("tes4-stream-shrank.bsa"));

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::io_error);
  }
}

TEST_CASE("ba2 dx10 writer preparation stage prepares a single-mip chunk",
          "[unit][writer-stage][ba2_dx10_writer]")
{
  const libbsa::texture::dds_texture_layout layout{4U, 4U, 1U, 28U, 1U, false};
  auto source = ba2_dx10_stage_entry("Textures/Stage/Single.dds", layout, "dx10-single");
  auto planned = libbsa::texture::plan_dx10_chunks(layout, 0U);
  REQUIRE(planned.has_value());
  REQUIRE(planned.value().size() == 1U);

  auto chunk = libbsa::formats::ba2::ba2_dx10_prepare_chunk(libbsa::ba2_dx10_target::fallout4,
                                                            libbsa::ba2_dx10_writer_options{},
                                                            source,
                                                            planned.value()[0]);

  REQUIRE(chunk.has_value());
  CHECK(chunk.value().raw_size == planned.value()[0].raw_size);
  CHECK(chunk.value().packed_size > 0U);
  CHECK(chunk.value().start_mip == 0U);
  CHECK(chunk.value().end_mip == 0U);
}

TEST_CASE("ba2 dx10 writer preparation stage preserves multi-mip snapshot chunk order",
          "[unit][writer-stage][ba2_dx10_writer]")
{
  const libbsa::texture::dds_texture_layout layout{4U, 4U, 3U, 28U, 1U, false};
  auto source = ba2_dx10_stage_entry("Textures/Stage/MultiChunk.dds", layout, "dx10-multi-chunk");
  auto planned = libbsa::texture::plan_dx10_chunks(layout, 0U);
  REQUIRE(planned.has_value());
  REQUIRE(planned.value().size() == 1U);
  REQUIRE(planned.value()[0].start_mip < planned.value()[0].end_mip);

  auto chunk = libbsa::formats::ba2::ba2_dx10_prepare_chunk(libbsa::ba2_dx10_target::fallout4,
                                                            libbsa::ba2_dx10_writer_options{},
                                                            source,
                                                            planned.value()[0]);

  REQUIRE(chunk.has_value());
  CHECK(chunk.value().raw_size == planned.value()[0].raw_size);
  CHECK(chunk.value().packed_size > 0U);
  CHECK(chunk.value().start_mip == planned.value()[0].start_mip);
  CHECK(chunk.value().end_mip == planned.value()[0].end_mip);

  auto decoded = libbsa::detail::decompress_payload_exact(chunk.value().compression,
                                                          chunk.value().stored_payload,
                                                          static_cast<std::size_t>(planned.value()[0].raw_size));
  REQUIRE(decoded.has_value());
  std::vector<std::byte> expected;
  expected.reserve(static_cast<std::size_t>(planned.value()[0].raw_size));
  for (std::uint32_t mip = planned.value()[0].start_mip; mip <= planned.value()[0].end_mip; ++mip)
  {
    auto mip_size = libbsa::texture::mip_size_for_format(layout, mip);
    REQUIRE(mip_size.has_value());
    auto bytes = repeated_bytes(static_cast<std::size_t>(mip_size.value()), static_cast<std::uint8_t>(mip + 1U));
    expected.insert(expected.end(), bytes.begin(), bytes.end());
  }
  CHECK(decoded.value() == expected);
}

TEST_CASE("ba2 dx10 writer preparation stage rejects truncated snapshots before publishing output",
          "[unit][writer-stage][ba2_dx10_writer][publish]")
{
  const libbsa::texture::dds_texture_layout layout{4U, 4U, 2U, 28U, 1U, false};
  auto source = ba2_dx10_stage_entry("Textures/Stage/Truncated.dds", layout, "dx10-truncated");
  REQUIRE_FALSE(source.subresources.empty());
  write_stage_binary_file(source.subresources[0].snapshot_path, bytes_from_text("short"));

  const auto output_path = stage_output_path("dx10-truncated-snapshot.ba2");
  const auto sentinel = bytes_from_text("existing BA2 DX10 sentinel");
  write_stage_binary_file(output_path, sentinel);
  libbsa::ba2_dx10_writer_options options;
  options.overwrite_existing = true;

  auto written = libbsa::formats::ba2::write_ba2_dx10_archive(libbsa::ba2_dx10_target::fallout4,
                                                              options,
                                                              std::span{&source, 1U},
                                                              output_path.string(),
                                                              1U);

  REQUIRE_FALSE(written.has_value());
  CHECK(written.error().code == libbsa::error_code::io_error);
  CHECK(read_stage_binary_file(output_path) == sentinel);
}

TEST_CASE("ba2 dx10 writer preparation stage prepares multi-mip and cubemap entries",
          "[unit][writer-stage][ba2_dx10_writer]")
{
  const libbsa::texture::dds_texture_layout multi_mip{4U, 4U, 2U, 28U, 1U, false};
  auto multi_entry = ba2_dx10_stage_entry("Textures/Stage/Multi.dds", multi_mip, "dx10-multi");
  auto multi_prepared = libbsa::formats::ba2::ba2_dx10_prepare_entries(libbsa::ba2_dx10_target::fallout4,
                                                                       libbsa::ba2_dx10_writer_options{},
                                                                       std::span{&multi_entry, 1U},
                                                                       1U);

  REQUIRE(multi_prepared.has_value());
  REQUIRE(multi_prepared.value().size() == 1U);
  CHECK(multi_prepared.value()[0].mip_count == 2U);
  CHECK_FALSE(multi_prepared.value()[0].chunks.empty());

  const libbsa::texture::dds_texture_layout cubemap{4U, 4U, 1U, 28U, 1U, true};
  auto cubemap_entry = ba2_dx10_stage_entry("Textures/Stage/Cube.dds", cubemap, "dx10-cube");
  auto cubemap_prepared = libbsa::formats::ba2::ba2_dx10_prepare_entries(libbsa::ba2_dx10_target::fallout4,
                                                                         libbsa::ba2_dx10_writer_options{},
                                                                         std::span{&cubemap_entry, 1U},
                                                                         1U);

  REQUIRE(cubemap_prepared.has_value());
  REQUIRE(cubemap_prepared.value().size() == 1U);
  CHECK(cubemap_prepared.value()[0].cube_maps_raw == 2049U);
  CHECK(cubemap_prepared.value()[0].chunks.size() == 6U);
}

TEST_CASE("ba2 dx10 writer layout stage toggles duplicate chunk reuse",
          "[unit][writer-stage][ba2_dx10_writer]")
{
  const auto payload = bytes_from_text("dx10-shared");
  const auto version = libbsa::formats::ba2::ba2_dx10_version_for(libbsa::ba2_dx10_target::fallout4);

  auto distinct = std::vector{ba2_dx10_prepared_stage_entry("Textures/Stage/Distinct.dds", payload)};
  std::uint64_t distinct_file_table_offset = 0;
  auto assigned_distinct =
      libbsa::formats::ba2::ba2_dx10_assign_payload_offsets(distinct, version, false, distinct_file_table_offset);

  REQUIRE(assigned_distinct.has_value());
  REQUIRE(distinct[0].chunks.size() == 2U);
  CHECK(distinct[0].chunks[0].payload_offset != distinct[0].chunks[1].payload_offset);
  CHECK(distinct[0].chunks[0].owns_payload_bytes);
  CHECK(distinct[0].chunks[1].owns_payload_bytes);

  auto deduped = std::vector{ba2_dx10_prepared_stage_entry("Textures/Stage/Deduped.dds", payload)};
  std::uint64_t deduped_file_table_offset = 0;
  auto assigned_deduped =
      libbsa::formats::ba2::ba2_dx10_assign_payload_offsets(deduped, version, true, deduped_file_table_offset);

  REQUIRE(assigned_deduped.has_value());
  REQUIRE(deduped[0].chunks.size() == 2U);
  CHECK(deduped[0].chunks[0].payload_offset == deduped[0].chunks[1].payload_offset);
  CHECK(deduped[0].chunks[0].owns_payload_bytes);
  CHECK_FALSE(deduped[0].chunks[1].owns_payload_bytes);
}
