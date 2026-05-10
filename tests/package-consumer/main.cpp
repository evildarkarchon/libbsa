#include <libbsa/libbsa.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

class byte_vector_sink final : public libbsa::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    return bytes.size();
  }

  const std::vector<std::byte>& bytes() const noexcept { return bytes_; }

 private:
  std::vector<std::byte> bytes_;
};

class byte_vector_sink_factory final : public libbsa::bulk_extract_sink_factory {
 public:
  libbsa::result<std::unique_ptr<libbsa::payload_sink>> create(std::string_view path,
                                                               const libbsa::entry_metadata& entry) override {
    // Bulk extraction may call the factory concurrently, so even example bookkeeping is protected.
    const std::lock_guard lock{mutex_};
    requested_paths_.push_back(std::string{path});
    observed_entries_.push_back(entry.path);
    std::unique_ptr<libbsa::payload_sink> sink = std::make_unique<byte_vector_sink>();
    return sink;
  }

  const std::vector<std::string>& requested_paths() const noexcept { return requested_paths_; }

  const std::vector<std::string>& observed_entries() const noexcept { return observed_entries_; }

 private:
  std::mutex mutex_;
  std::vector<std::string> requested_paths_;
  std::vector<std::string> observed_entries_;
};

libbsa::result<void> example_open_list_extract(std::string_view archive_host_path,
                                               std::string_view archive_virtual_path,
                                               libbsa::payload_sink& sink) {
  auto opened = libbsa::archive_reader::open(archive_host_path);
  if (!opened) {
    return opened.error();
  }

  auto reader = std::move(opened).value();
  auto metadata = reader.metadata();
  if (!metadata) {
    return metadata.error();
  }

  auto entries = reader.entries();
  if (!entries) {
    return entries.error();
  }

  auto found = reader.find(archive_virtual_path);
  if (!found) {
    return found.error();
  }
  if (!found.value()) {
    return libbsa::error{libbsa::error_code::not_found, "archive path is not present"};
  }

  return reader.extract(archive_virtual_path, sink);
}

libbsa::result<std::vector<libbsa::bulk_extract_entry_result>> example_bulk_extract(
    std::string_view archive_host_path,
    std::span<const std::string> archive_virtual_paths,
    libbsa::bulk_extract_sink_factory& sink_factory) {
  auto opened = libbsa::archive_reader::open(archive_host_path);
  if (!opened) {
    return opened.error();
  }

  std::vector<libbsa::bulk_extract_request> requests;
  requests.reserve(archive_virtual_paths.size());
  for (const auto& archive_virtual_path : archive_virtual_paths) {
    requests.push_back(libbsa::bulk_extract_request{archive_virtual_path});
  }

  libbsa::bulk_extract_options options;
  options.worker_count = 2U;

  auto reader = std::move(opened).value();
  return reader.extract_entries(requests, sink_factory, options);
}

libbsa::result<void> example_create_tes3_bsa(std::string_view source_host_path, std::string_view output_host_path) {
  libbsa::tes3_bsa_writer_options options;
  options.overwrite_existing = true;

  libbsa::tes3_bsa_writer writer{options};
  if (auto added = writer.add_file("book/readme.txt", source_host_path); !added) {
    return added.error();
  }

  libbsa::write_execution_options execution;
  execution.worker_count = 1U;
  return writer.write_to(output_host_path, execution);
}

libbsa::result<void> example_create_tes4_bsa(std::string_view source_host_path, std::string_view output_host_path) {
  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::target_default;
  options.overwrite_existing = true;

  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::skyrim_se, options};
  if (auto added = writer.add_file("meshes/example/example.nif",
                                   source_host_path,
                                   libbsa::entry_compression_policy::inherit);
      !added) {
    return added.error();
  }

  libbsa::write_execution_options execution;
  execution.worker_count = 2U;
  return writer.write_to(output_host_path, execution);
}

libbsa::result<void> example_create_ba2_gnrl(std::string_view source_host_path, std::string_view output_host_path) {
  libbsa::ba2_gnrl_writer_options options;
  options.compression = libbsa::archive_compression_policy::target_default;
  options.overwrite_existing = true;
  options.starfield_compression_method = 3U;

  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::starfield_v3, options};
  if (auto added = writer.add_file("scripts/example/example.pex",
                                   source_host_path,
                                   libbsa::entry_compression_policy::compressed);
      !added) {
    return added.error();
  }

  libbsa::write_execution_options execution;
  execution.worker_count = 2U;
  return writer.write_to(output_host_path, execution);
}

libbsa::result<void> example_create_ba2_dx10(std::string_view dds_host_path, std::string_view output_host_path) {
  libbsa::ba2_dx10_writer_options options;
  options.overwrite_existing = true;
  options.starfield_compression_method = 3U;

  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::starfield_v3, options};
  if (auto added = writer.add_file("textures/example/example_d.dds", dds_host_path); !added) {
    return added.error();
  }

  libbsa::write_execution_options execution;
  execution.worker_count = 2U;
  return writer.write_to(output_host_path, execution);
}

int example_handle_result_errors(std::string_view archive_host_path) {
  auto opened = libbsa::archive_reader::open(archive_host_path);
  if (opened) {
    return 0;
  }

  const auto code = opened.error().code;
  if (code == libbsa::error_code::io_error) {
    return 10;
  }
  if (code == libbsa::error_code::format_error) {
    return 20;
  }
  if (code == libbsa::error_code::unsupported) {
    return 30;
  }
  return 1;
}

libbsa::result<libbsa::validation_report> example_validate_archive(std::string_view archive_host_path) {
  libbsa::validation_options options;
  options.expected_type = libbsa::archive_type::bsa;
  options.validate_entry_extractability = true;

  auto report = libbsa::validate_archive(archive_host_path, options);
  if (!report) {
    return report.error();
  }

  for (const auto& warning : report.value().warnings) {
    if (warning.code == libbsa::compatibility_warning_code::compressed_sound_payload) {
      break;
    }
  }

  return report;
}

} // namespace

int main() {
  auto result = libbsa::validate_archive("consumer-smoke.bsa");
  if (result) {
    return 1;
  }

  return result.error().code == libbsa::error_code::io_error ? 0 : 1;
}
