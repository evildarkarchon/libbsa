#include "formats/ba2/ba2_dx10_snapshot_builder.hpp"

#include <detail/archive_path.hpp>
#include <detail/host_file.hpp>

#include "texture/directxtex_analyzer.hpp"

// BCrypt depends on Windows base types; keep both headers private and prevent min/max macros.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <bcrypt.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>

namespace libbsa::formats::ba2
{

  namespace
  {

    /// Returns true for DDS DXGI formats accepted by the Starfield DX10 profile but not Fallout 4.
    bool is_starfield_only_dx10_format(std::uint32_t dxgi_format) noexcept
    {
      switch (dxgi_format)
      {
      case 29U: // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB.
      case 31U: // DXGI_FORMAT_R8G8B8A8_SNORM.
      case 72U: // DXGI_FORMAT_BC1_UNORM_SRGB.
      case 84U: // DXGI_FORMAT_BC5_SNORM.
      case 95U: // DXGI_FORMAT_BC6H_UF16.
      case 96U: // DXGI_FORMAT_BC6H_SF16.
      case 99U: // DXGI_FORMAT_BC7_UNORM_SRGB.
        return true;
      default:
        return false;
      }
    }

    std::string preserved_archive_path(std::string_view archive_path)
    {
      std::string preserved{archive_path};
      std::replace(preserved.begin(), preserved.end(), '\\', '/');
      return preserved;
    }

    constexpr detail::host_file_context ba2_dx10_dds_source_context{
        "BA2 DX10 writer failed to open DDS source",
        "BA2 DX10 writer failed to inspect DDS source",
        "BA2 DX10 writer failed while reading DDS source",
        "BA2 DX10 DDS source changed during analysis",
        "BA2 DX10 DDS source"};

    constexpr std::size_t snapshot_random_suffix_bytes = 16U;

    result<std::vector<std::byte>> read_dds_file(std::string_view dds_host_path)
    {
      auto source_path = detail::resolve_host_file_path(dds_host_path);
      if (!source_path)
      {
        return source_path.error();
      }
      return detail::read_host_file_exact(source_path.value(), ba2_dx10_dds_source_context);
    }

    /// Generates a 128-bit lowercase hex suffix using the Windows system-preferred RNG.
    /// Returns `io_error` if Windows cannot provide fresh cryptographic randomness for a candidate.
    result<std::string> make_snapshot_random_suffix()
    {
      std::array<std::byte, snapshot_random_suffix_bytes> random_bytes{};
      const auto status = BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(random_bytes.data()),
                                          static_cast<ULONG>(random_bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
      if (status < 0)
      {
        return error{error_code::io_error, "BA2 DX10 writer failed to generate snapshot temp directory name"};
      }

      constexpr char hex_digits[] = "0123456789abcdef";
      std::string suffix;
      suffix.resize(random_bytes.size() * 2U);
      for (std::size_t index = 0; index < random_bytes.size(); ++index)
      {
        const auto value = static_cast<unsigned char>(random_bytes[index]);
        suffix[index * 2U] = hex_digits[(value >> 4U) & 0x0FU];
        suffix[index * 2U + 1U] = hex_digits[value & 0x0FU];
      }
      return suffix;
    }

    result<std::filesystem::path> make_unique_snapshot_directory()
    {
      std::error_code fs_error;
      const auto root = std::filesystem::temp_directory_path(fs_error);
      if (fs_error)
      {
        return error{error_code::io_error, "BA2 DX10 writer failed to locate snapshot temp root"};
      }

      for (std::uint32_t attempt = 0; attempt < 1024U; ++attempt)
      {
        auto suffix = make_snapshot_random_suffix();
        if (!suffix)
        {
          return suffix.error();
        }
        const auto candidate = root / ("libbsa-dx10-snapshot-" + suffix.value());
        fs_error.clear();
        // Directory creation remains the atomic reservation boundary; existing paths are collisions.
        if (std::filesystem::create_directory(candidate, fs_error))
        {
          return candidate;
        }
        if (fs_error)
        {
          return error{error_code::io_error, "BA2 DX10 writer failed to reserve snapshot temp directory"};
        }
      }
      return error{error_code::io_error, "BA2 DX10 writer exhausted snapshot temp directory names"};
    }

    result<void> write_snapshot_file(const std::filesystem::path &snapshot_path, std::span<const std::byte> bytes)
    {
      std::ofstream output{snapshot_path, std::ios::binary | std::ios::trunc};
      if (!output)
      {
        return error{error_code::io_error, "BA2 DX10 writer failed to create snapshot temp file"};
      }
      output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
      if (!output)
      {
        return error{error_code::io_error, "BA2 DX10 writer failed while writing snapshot temp file"};
      }
      return {};
    }

  } // namespace

  result<void> ba2_dx10_validate_texture_format_for_target(ba2_dx10_target target, std::uint32_t dxgi_format)
  {
    switch (target)
    {
    case ba2_dx10_target::fallout4:
      if (is_starfield_only_dx10_format(dxgi_format))
      {
        return error{error_code::format_error,
                     "BA2 DX10 Fallout 4 target does not support BC6, SRGB, or SNORM DDS formats"};
      }
      return {};
    case ba2_dx10_target::starfield_v3:
      return {};
    }
    return error{error_code::invalid_argument, "BA2 DX10 writer target profile is not supported"};
  }

  result<void> ba2_dx10_ensure_snapshot_directory(std::filesystem::path &snapshot_dir_path)
  {
    if (!snapshot_dir_path.empty())
    {
      return {};
    }
    auto snapshot_dir = make_unique_snapshot_directory();
    if (!snapshot_dir)
    {
      return snapshot_dir.error();
    }
    snapshot_dir_path = std::move(snapshot_dir.value());
    return {};
  }

  result<ba2_dx10_writer_entry> ba2_dx10_build_writer_entry_snapshot(std::string_view archive_path,
                                                                     std::string_view dds_host_path,
                                                                     ba2_dx10_target target,
                                                                     const std::filesystem::path &snapshot_dir,
                                                                     std::size_t entry_index)
  {
    auto canonical = detail::normalize_archive_path(archive_path);
    if (!canonical)
    {
      return canonical.error();
    }

    auto dds_bytes = read_dds_file(dds_host_path);
    if (!dds_bytes)
    {
      return dds_bytes.error();
    }

    auto source = texture::analyze_dds_source(dds_bytes.value());
    if (!source)
    {
      return source.error();
    }
    auto target_format = ba2_dx10_validate_texture_format_for_target(target, source.value().metadata.dxgi_format);
    if (!target_format)
    {
      return target_format.error();
    }

    ba2_dx10_writer_entry entry;
    entry.archive_path_original = preserved_archive_path(archive_path);
    entry.archive_path_canonical = std::move(canonical.value().value);
    entry.metadata = source.value().metadata;
    entry.subresources.reserve(source.value().subresources.size());
    for (std::size_t index = 0; index < source.value().subresources.size(); ++index)
    {
      const auto &subresource = source.value().subresources[index];
      auto snapshot_path = snapshot_dir / ("entry-" + std::to_string(entry_index) + "-subresource-" +
                                           std::to_string(index) + ".bin");
      auto written = write_snapshot_file(snapshot_path, subresource.bytes);
      if (!written)
      {
        return written.error();
      }
      entry.subresources.push_back(ba2_dx10_subresource_snapshot{
          subresource.array_index, subresource.face_index, subresource.mip, subresource.bytes.size(), std::move(snapshot_path)});
    }
    return entry;
  }

} // namespace libbsa::formats::ba2
