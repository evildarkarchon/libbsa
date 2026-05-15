#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace
{

  std::filesystem::path writer_execution_test_dir()
  {
    auto path = std::filesystem::temp_directory_path() / "libbsa_writer_execution_options_tests";
    std::filesystem::create_directories(path);
    return path;
  }

  std::filesystem::path output_path(std::string_view name)
  {
    static std::uint32_t counter = 0;
    auto path = writer_execution_test_dir() / (std::to_string(++counter) + "-" + std::string{name});
    std::error_code fs_error;
    std::filesystem::remove(path, fs_error);
    return path;
  }

  std::filesystem::path generated_dx10_source_path()
  {
    return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "source" /
           "ba2_dx10_multi_mip_bc7_unorm.dds";
  }

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

  std::vector<std::byte> read_binary_file(const std::filesystem::path &path)
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

  void require_entry_bytes(const std::filesystem::path &archive_path,
                           std::string_view entry_path,
                           std::span<const std::byte> expected)
  {
    auto opened = libbsa::archive_reader::open(archive_path.string());
    REQUIRE(opened.has_value());
    auto extracted = opened.value().extract_bytes(entry_path);
    REQUIRE(extracted.has_value());
    CHECK(extracted.value() == std::vector<std::byte>{expected.begin(), expected.end()});
  }

  void require_matching_archive_entries(const std::filesystem::path &lhs, const std::filesystem::path &rhs)
  {
    auto lhs_reader = libbsa::archive_reader::open(lhs.string());
    auto rhs_reader = libbsa::archive_reader::open(rhs.string());
    REQUIRE(lhs_reader.has_value());
    REQUIRE(rhs_reader.has_value());

    auto lhs_entries = lhs_reader.value().entries();
    auto rhs_entries = rhs_reader.value().entries();
    REQUIRE(lhs_entries.has_value());
    REQUIRE(rhs_entries.has_value());
    REQUIRE(lhs_entries.value().size() == rhs_entries.value().size());

    for (const auto &entry : lhs_entries.value())
    {
      auto lhs_bytes = lhs_reader.value().extract_bytes(entry.original_path);
      auto rhs_bytes = rhs_reader.value().extract_bytes(entry.original_path);
      REQUIRE(lhs_bytes.has_value());
      REQUIRE(rhs_bytes.has_value());
      CHECK(lhs_bytes.value() == rhs_bytes.value());
    }
  }

} // namespace

TEST_CASE("writer_execution_options defaults to serial worker count", "[unit][writer_execution_options]")
{
  libbsa::write_execution_options execution;

  CHECK(execution.worker_count == 1U);
}

TEST_CASE("writer_execution_options rejects zero worker count before creating output",
          "[unit][writer_execution_options]")
{
  libbsa::write_execution_options execution;
  execution.worker_count = 0U;

  SECTION("TES3 writer")
  {
    libbsa::tes3_bsa_writer writer;
    REQUIRE(writer.add_file("Meshes/Missing.NIF", output_path("missing-tes3-source.nif").string()).has_value());
    const auto output = output_path("zero-worker-tes3.bsa");

    auto written = writer.write_to(output.string(), execution);

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::invalid_argument);
    CHECK_FALSE(std::filesystem::exists(output));
  }

  SECTION("TES4-family writer")
  {
    libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3};
    REQUIRE(writer.add_file("Meshes/Missing.NIF", output_path("missing-tes4-source.nif").string()).has_value());
    const auto output = output_path("zero-worker-tes4.bsa");

    auto written = writer.write_to(output.string(), execution);

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::invalid_argument);
    CHECK_FALSE(std::filesystem::exists(output));
  }

  SECTION("BA2 GNRL writer")
  {
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};
    REQUIRE(writer.add_file("Meshes/Missing.nif", output_path("missing-gnrl-source.nif").string()).has_value());
    const auto output = output_path("zero-worker-gnrl.ba2");

    auto written = writer.write_to(output.string(), execution);

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::invalid_argument);
    CHECK_FALSE(std::filesystem::exists(output));
  }

  SECTION("BA2 DX10 writer")
  {
    libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};
    const auto output = output_path("zero-worker-dx10.ba2");

    auto written = writer.write_to(output.string(), execution);

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::invalid_argument);
    CHECK_FALSE(std::filesystem::exists(output));
  }
}

TEST_CASE("writer_execution_options rejects unsupported large worker counts before creating output",
          "[unit][writer_execution_options]")
{
  libbsa::write_execution_options execution;
  execution.worker_count = 1025U;

  SECTION("TES4-family writer")
  {
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
    REQUIRE(writer.add_bytes("Meshes/LargeWorker.NIF", bytes_from_text("tes4 payload")).has_value());
    const auto output = output_path("large-worker-tes4.bsa");

    auto written = writer.write_to(output.string(), execution);

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::invalid_argument);
    CHECK_FALSE(std::filesystem::exists(output));
  }

  SECTION("BA2 GNRL writer")
  {
    libbsa::ba2_gnrl_writer_options options;
    options.compression = libbsa::archive_compression_policy::all_raw;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
    REQUIRE(writer.add_bytes("Meshes/LargeWorker.nif", bytes_from_text("ba2 gnrl payload")).has_value());
    const auto output = output_path("large-worker-gnrl.ba2");

    auto written = writer.write_to(output.string(), execution);

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::invalid_argument);
    CHECK_FALSE(std::filesystem::exists(output));
  }
}

TEST_CASE("writer_execution_options worker count one preserves serial writer behavior",
          "[unit][writer_execution_options]")
{
  libbsa::write_execution_options execution;
  execution.worker_count = 1U;

  SECTION("TES3 writer")
  {
    const auto payload = bytes_from_text("tes3 serial payload");
    libbsa::tes3_bsa_writer serial_writer;
    libbsa::tes3_bsa_writer execution_writer;
    REQUIRE(serial_writer.add_bytes("Meshes/Serial.NIF", payload).has_value());
    REQUIRE(execution_writer.add_bytes("Meshes/Serial.NIF", payload).has_value());
    const auto serial_output = output_path("serial-default-tes3.bsa");
    const auto execution_output = output_path("execution-default-tes3.bsa");

    REQUIRE(serial_writer.write_to(serial_output.string()).has_value());
    REQUIRE(execution_writer.write_to(execution_output.string(), execution).has_value());

    CHECK(read_binary_file(serial_output) == read_binary_file(execution_output));
    require_entry_bytes(execution_output, "Meshes/Serial.NIF", payload);
  }

  SECTION("TES4-family writer")
  {
    const auto payload = bytes_from_text("tes4 serial payload");
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    libbsa::tes4_bsa_writer serial_writer{libbsa::tes4_bsa_target::fallout3, options};
    libbsa::tes4_bsa_writer execution_writer{libbsa::tes4_bsa_target::fallout3, options};
    REQUIRE(serial_writer.add_bytes("Meshes/Serial.NIF", payload).has_value());
    REQUIRE(execution_writer.add_bytes("Meshes/Serial.NIF", payload).has_value());
    const auto serial_output = output_path("serial-default-tes4.bsa");
    const auto execution_output = output_path("execution-default-tes4.bsa");

    REQUIRE(serial_writer.write_to(serial_output.string()).has_value());
    REQUIRE(execution_writer.write_to(execution_output.string(), execution).has_value());

    CHECK(read_binary_file(serial_output) == read_binary_file(execution_output));
    require_entry_bytes(execution_output, "Meshes/Serial.NIF", payload);
  }

  SECTION("BA2 GNRL writer")
  {
    const auto payload = bytes_from_text("ba2 gnrl serial payload");
    libbsa::ba2_gnrl_writer_options options;
    options.compression = libbsa::archive_compression_policy::all_raw;
    libbsa::ba2_gnrl_writer serial_writer{libbsa::ba2_gnrl_target::fallout4, options};
    libbsa::ba2_gnrl_writer execution_writer{libbsa::ba2_gnrl_target::fallout4, options};
    REQUIRE(serial_writer.add_bytes("Meshes/Serial.nif", payload).has_value());
    REQUIRE(execution_writer.add_bytes("Meshes/Serial.nif", payload).has_value());
    const auto serial_output = output_path("serial-default-gnrl.ba2");
    const auto execution_output = output_path("execution-default-gnrl.ba2");

    REQUIRE(serial_writer.write_to(serial_output.string()).has_value());
    REQUIRE(execution_writer.write_to(execution_output.string(), execution).has_value());

    CHECK(read_binary_file(serial_output) == read_binary_file(execution_output));
    require_entry_bytes(execution_output, "Meshes/Serial.nif", payload);
  }

  SECTION("BA2 DX10 writer")
  {
    REQUIRE(std::filesystem::exists(generated_dx10_source_path()));
    libbsa::ba2_dx10_writer serial_writer{libbsa::ba2_dx10_target::fallout4};
    libbsa::ba2_dx10_writer execution_writer{libbsa::ba2_dx10_target::fallout4};
    REQUIRE(serial_writer.add_file("textures/serial/texture.dds", generated_dx10_source_path().string()).has_value());
    REQUIRE(execution_writer.add_file("textures/serial/texture.dds", generated_dx10_source_path().string()).has_value());
    const auto serial_output = output_path("serial-default-dx10.ba2");
    const auto execution_output = output_path("execution-default-dx10.ba2");

    REQUIRE(serial_writer.write_to(serial_output.string()).has_value());
    REQUIRE(execution_writer.write_to(execution_output.string(), execution).has_value());

    require_matching_archive_entries(serial_output, execution_output);
  }
}
