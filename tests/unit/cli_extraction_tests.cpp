#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "tools/cli/extraction.hpp"

#include <libbsa/writer.hpp>

#include <filesystem>
#include <fstream>
#include <future>
#include <iterator>
#include <latch>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {

using entry_results = std::vector<libbsa::bulk_extract_entry_result>;

/// Gives each invocation its own directory within the test process's private temp root.
std::filesystem::path extraction_test_root() {
    static unsigned next_id = 0;
    auto root =
        std::filesystem::temp_directory_path() / "cli_extraction" / std::to_string(next_id++);
    std::filesystem::create_directories(root);
    return root;
}

/// Encodes a host path as the UTF-8 text accepted at the CLI Extraction boundary.
std::string utf8_path(const std::filesystem::path& path) {
    const auto value = path.u8string();
    return {reinterpret_cast<const char*>(value.data()), value.size()};
}

std::filesystem::path fixture_path(std::string_view name) {
    return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests/fixtures/generated/archives" / name;
}

/// Creates options against a checked-in synthetic archive without copying or rewriting it.
libbsa::cli::extraction_options options_for(const std::filesystem::path& output,
                                            std::string_view fixture = "tes3_success.bsa") {
    return {utf8_path(fixture_path(fixture)), utf8_path(output)};
}

/// Reads the real published file, failing clearly when extraction did not create it.
std::string read_text(const std::filesystem::path& path) {
    std::ifstream stream{path, std::ios::binary};
    REQUIRE(stream.good());
    return {std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
}

/// Creates a sentinel destination whose bytes must survive refused or failed overwrites.
void write_text(const std::filesystem::path& path, std::string_view text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream{path, std::ios::binary};
    REQUIRE(stream.good());
    stream << text;
    REQUIRE(stream.good());
}

/// Captures actual on-disk spelling, which Windows existence checks alone cannot verify.
std::set<std::wstring> child_names(const std::filesystem::path& path) {
    std::set<std::wstring> names;
    for (const auto& child : std::filesystem::directory_iterator{path}) {
        names.insert(child.path().filename().wstring());
    }
    return names;
}

}  // namespace

TEST_CASE("CLI Extraction opens the archive before preparing the output root",
          "[unit][cli][fixture]") {
    const auto root = extraction_test_root();
    auto options = options_for(root / "output", "malformed_non_bsa_bytes.bsa");
    auto expected_error = libbsa::error_code::unsupported;
    SECTION("unrecognized archive bytes") {}
    SECTION("truncated recognized header") {
        options.archive_path = utf8_path(fixture_path("tes3_truncated_header.bsa"));
        expected_error = libbsa::error_code::format_error;
    }
    SECTION("missing archive") {
        options.archive_path = utf8_path(root / "missing.bsa");
        expected_error = libbsa::error_code::io_error;
    }

    const auto outcome = libbsa::cli::extract(options);

    REQUIRE(std::holds_alternative<libbsa::cli::extraction_failure>(outcome));
    const auto& failure = std::get<libbsa::cli::extraction_failure>(outcome);
    CHECK(failure.failure.code == expected_error);
    CHECK(failure.context == options.archive_path);
    CHECK_FALSE(std::filesystem::exists(root / "output"));

    // An invalid output string must not replace an earlier archive-open failure.
    options.output_directory = std::string(1, static_cast<char>(0xff));
    const auto invalid_output = libbsa::cli::extract(options);
    REQUIRE(std::holds_alternative<libbsa::cli::extraction_failure>(invalid_output));
    CHECK(std::get<libbsa::cli::extraction_failure>(invalid_output).failure.code == expected_error);
    CHECK(std::get<libbsa::cli::extraction_failure>(invalid_output).context ==
          options.archive_path);
}

TEST_CASE("CLI Extraction rejects invalid worker counts before host work", "[unit][cli]") {
    const auto workers = GENERATE(0U, 1025U);
    const auto output = extraction_test_root() / "output";
    auto options = options_for(output);
    options.worker_count = workers;

    REQUIRE_THROWS_AS(libbsa::cli::extract(options), std::invalid_argument);
    CHECK_FALSE(std::filesystem::exists(output));
}

TEST_CASE("CLI Extraction preserves extraction of long destination leaf names",
          "[unit][cli][roundtrip]") {
    const auto root = extraction_test_root();
    const auto archive = root / "long-leaf.bsa";
    const std::string leaf = std::string(226, 'x') + ".txt";
    const std::vector<std::byte> payload{std::byte{'l'}, std::byte{'o'}, std::byte{'n'},
                                         std::byte{'g'}};
    libbsa::tes3_bsa_writer writer;
    REQUIRE(writer.add_bytes(leaf, payload).has_value());
    REQUIRE(writer.write_to(utf8_path(archive)).has_value());

    // Extended-length syntax removes MAX_PATH from this check: it catches a
    // staging suffix that pushes a valid leaf past the 255-character component limit.
    const auto output = std::filesystem::path{L"\\\\?\\" + (root / "output").wstring()};
    auto options = options_for(output);
    options.archive_path = utf8_path(archive);

    const auto outcome = libbsa::cli::extract(options);

    REQUIRE(std::holds_alternative<entry_results>(outcome));
    const auto& results = std::get<entry_results>(outcome);
    REQUIRE(results.size() == 1U);
    INFO((results[0].failure ? results[0].failure->message : "no extraction failure"));
    REQUIRE(results[0].succeeded());
    CHECK(read_text(output / leaf) == "long");
    CHECK(child_names(output) == std::set<std::wstring>{std::wstring(226, L'x') + L".txt"});
}

TEST_CASE("CLI Extraction reports output setup failures with their diagnostic context",
          "[unit][cli][fixture]") {
    const auto root = extraction_test_root();
    auto options = options_for(root / "output");

    SECTION("invalid UTF-8 carries the supplied output argument") {
        options.output_directory = std::string(1, static_cast<char>(0xff));
        const auto outcome = libbsa::cli::extract(options);
        REQUIRE(std::holds_alternative<libbsa::cli::extraction_failure>(outcome));
        const auto& failure = std::get<libbsa::cli::extraction_failure>(outcome);
        CHECK(failure.failure.code == libbsa::error_code::invalid_argument);
        CHECK(failure.context == options.output_directory);
    }

    SECTION("a file in place of the root is preserved and needs no extra context") {
        write_text(root / "output", "root sentinel");
        const auto outcome = libbsa::cli::extract(options);
        REQUIRE(std::holds_alternative<libbsa::cli::extraction_failure>(outcome));
        const auto& failure = std::get<libbsa::cli::extraction_failure>(outcome);
        CHECK(failure.failure.code == libbsa::error_code::io_error);
        CHECK(failure.context.empty());
        CHECK(read_text(root / "output") == "root sentinel");
    }
}

TEST_CASE("CLI Extraction creates an empty root for all-missing selections",
          "[unit][cli][fixture]") {
    const auto output = extraction_test_root() / "output";
    auto options = options_for(output);
    options.selected_paths = {"missing/first.txt", "missing/second.txt"};

    const auto outcome = libbsa::cli::extract(options);

    REQUIRE(std::holds_alternative<entry_results>(outcome));
    const auto& results = std::get<entry_results>(outcome);
    REQUIRE(results.size() == 2U);
    CHECK(results[0].path == "missing/first.txt");
    CHECK(results[1].path == "missing/second.txt");
    for (const auto& result : results) {
        REQUIRE(result.failure.has_value());
        CHECK(result.failure->code == libbsa::error_code::not_found);
        CHECK_FALSE(result.entry.has_value());
    }
    REQUIRE(std::filesystem::is_directory(output));
    CHECK(std::filesystem::is_empty(output));
}

TEST_CASE("CLI Extraction preserves mixed request order and coalesces exact duplicates",
          "[unit][cli][fixture]") {
    const auto workers = GENERATE(1U, 3U);
    const auto output = extraction_test_root() / "output";
    auto options = options_for(output);
    options.worker_count = workers;
    options.selected_paths = {"missing/first.txt", "meshes/tiny/probe.nif", "meshes/tiny/probe.nif",
                              "missing/second.txt"};

    const auto outcome = libbsa::cli::extract(options);

    REQUIRE(std::holds_alternative<entry_results>(outcome));
    const auto& results = std::get<entry_results>(outcome);
    REQUIRE(results.size() == 4U);
    CHECK(results[0].path == "missing/first.txt");
    REQUIRE(results[0].failure.has_value());
    CHECK(results[0].failure->code == libbsa::error_code::not_found);
    CHECK(results[1].path == "meshes/tiny/probe.nif");
    CHECK(results[1].succeeded());
    CHECK(results[2].path == "meshes/tiny/probe.nif");
    CHECK(results[2].succeeded());
    CHECK(results[3].path == "missing/second.txt");
    REQUIRE(results[3].failure.has_value());
    CHECK(results[3].failure->code == libbsa::error_code::not_found);
    REQUIRE(results[1].entry.has_value());
    CHECK(results[1].entry->original_path == "Meshes\\Tiny\\Probe.nif");
    CHECK(read_text(output / "Meshes/Tiny/Probe.nif") == "tes3 probe nif bytes\n");
    CHECK(child_names(output) == std::set<std::wstring>{L"Meshes"});
    CHECK(child_names(output / "Meshes") == std::set<std::wstring>{L"Tiny"});
    CHECK(child_names(output / "Meshes/Tiny") == std::set<std::wstring>{L"Probe.nif"});
}

TEST_CASE("CLI Extraction publishes all entries with serial and parallel workers",
          "[unit][cli][fixture]") {
    const auto workers = GENERATE(1U, 3U);
    const auto output = extraction_test_root() / "output";
    auto options = options_for(output);
    options.worker_count = workers;

    const auto outcome = libbsa::cli::extract(options);

    REQUIRE(std::holds_alternative<entry_results>(outcome));
    const auto& results = std::get<entry_results>(outcome);
    REQUIRE(results.size() == 4U);
    for (const auto& result : results) {
        CHECK(result.succeeded());
    }
    CHECK(results[0].path == "Icons\\EmptyMarker.txt");
    CHECK(results[1].path == "Meshes\\Tiny\\Probe.nif");
    CHECK(results[2].path == "Sound\\Fx\\Ping.wav");
    CHECK(results[3].path == "textures\\tx_probe.dds");
    CHECK(read_text(output / "Icons/EmptyMarker.txt").empty());
    CHECK(read_text(output / "Meshes/Tiny/Probe.nif") == "tes3 probe nif bytes\n");
    CHECK(read_text(output / "Sound/Fx/Ping.wav") == "tes3 ping wave bytes\n");
    CHECK(read_text(output / "textures/tx_probe.dds") == "tes3 tiny dds bytes\n");
}

TEST_CASE("CLI Extraction preserves existing destinations unless overwrite succeeds",
          "[unit][cli][fixture]") {
    const auto overwrite = GENERATE(false, true);
    const auto output = extraction_test_root() / "output";
    const auto destination = output / "Meshes/Tiny/Probe.nif";
    write_text(destination, "sentinel");
    auto options = options_for(output);
    options.selected_paths = {"meshes/tiny/probe.nif"};
    options.overwrite = overwrite;

    const auto outcome = libbsa::cli::extract(options);

    REQUIRE(std::holds_alternative<entry_results>(outcome));
    const auto& results = std::get<entry_results>(outcome);
    REQUIRE(results.size() == 1U);
    CHECK(results[0].succeeded() == overwrite);
    CHECK(read_text(destination) == (overwrite ? "tes3 probe nif bytes\n" : "sentinel"));
    if (!overwrite) {
        REQUIRE(results[0].failure.has_value());
        CHECK(results[0].failure->code == libbsa::error_code::io_error);
    }
    CHECK(child_names(destination.parent_path()) == std::set<std::wstring>{L"Probe.nif"});
}

TEST_CASE("CLI Extraction discards corrupt payload staging and preserves overwrite targets",
          "[unit][cli][fixture][malformed]") {
    const auto overwrite = GENERATE(false, true);
    const auto workers = GENERATE(1U, 3U);
    const auto output = extraction_test_root() / "output";
    const auto destination = output / "meshes/tiny/packedmesh.nif";
    if (overwrite) {
        write_text(destination, "sentinel");
    }
    auto options = options_for(output, "malformed_corrupt_compressed_payload.bsa");
    options.selected_paths = {"meshes/tiny/packedmesh.nif"};
    options.overwrite = overwrite;
    options.worker_count = workers;

    const auto outcome = libbsa::cli::extract(options);

    REQUIRE(std::holds_alternative<entry_results>(outcome));
    const auto& results = std::get<entry_results>(outcome);
    REQUIRE(results.size() == 1U);
    REQUIRE(results[0].failure.has_value());
    CHECK(results[0].failure->code == libbsa::error_code::format_error);
    if (overwrite) {
        CHECK(read_text(destination) == "sentinel");
        CHECK(child_names(destination.parent_path()) == std::set<std::wstring>{L"packedmesh.nif"});
    } else {
        CHECK_FALSE(std::filesystem::exists(destination));
        CHECK(std::filesystem::is_empty(destination.parent_path()));
    }
}

TEST_CASE("CLI Extraction rejects unsafe destination names before creating their parents",
          "[unit][cli][fixture][malformed]") {
    const auto output = extraction_test_root() / "output";
    auto options = options_for(output, "tes3_windows_unsafe_names.bsa");
    options.worker_count = 3U;

    const auto outcome = libbsa::cli::extract(options);

    REQUIRE(std::holds_alternative<entry_results>(outcome));
    const auto& results = std::get<entry_results>(outcome);
    REQUIRE(results.size() == 6U);
    for (const auto& result : results) {
        REQUIRE(result.failure.has_value());
        CHECK(result.failure->code == libbsa::error_code::invalid_argument);
    }
    REQUIRE(std::filesystem::is_directory(output));
    CHECK(std::filesystem::is_empty(output));
}

TEST_CASE("CLI Extraction decodes UTF-8 host archive and output paths", "[unit][cli][fixture]") {
    const auto root = extraction_test_root() / L"\u00c5ngstr\u00f6m-\u65e5\u672c\u8a9e";
    std::filesystem::create_directories(root);
    const auto archive = root / L"\u00e9.bsa";
    std::filesystem::copy_file(fixture_path("tes3_success.bsa"), archive);
    const auto output = root / L"\u51fa\u529b";
    auto options = options_for(output);
    options.archive_path = utf8_path(archive);
    options.selected_paths = {"meshes/tiny/probe.nif"};

    const auto outcome = libbsa::cli::extract(options);

    REQUIRE(std::holds_alternative<entry_results>(outcome));
    REQUIRE(std::get<entry_results>(outcome).size() == 1U);
    CHECK(std::get<entry_results>(outcome)[0].succeeded());
    CHECK(read_text(output / "Meshes/Tiny/Probe.nif") == "tes3 probe nif bytes\n");
}

TEST_CASE("CLI Extraction keeps simultaneous operation publication and cleanup isolated",
          "[unit][cli][fixture]") {
    const auto root = extraction_test_root();
    const auto success_output = root / "success";
    const auto failure_output = root / "failure";
    auto success_options = options_for(success_output);
    success_options.worker_count = 3U;
    auto failure_options = options_for(failure_output, "malformed_corrupt_compressed_payload.bsa");
    failure_options.selected_paths = {"meshes/tiny/packedmesh.nif"};
    failure_options.overwrite = true;
    write_text(failure_output / "meshes/tiny/packedmesh.nif", "sentinel");
    std::latch start{2};

    auto successful = std::async(std::launch::async, [&] {
        start.arrive_and_wait();
        return libbsa::cli::extract(success_options);
    });
    auto failed = std::async(std::launch::async, [&] {
        start.arrive_and_wait();
        return libbsa::cli::extract(failure_options);
    });
    const auto success = successful.get();
    const auto failure = failed.get();

    REQUIRE(std::holds_alternative<entry_results>(success));
    REQUIRE(std::get<entry_results>(success).size() == 4U);
    for (const auto& result : std::get<entry_results>(success)) {
        CHECK(result.succeeded());
    }
    CHECK(read_text(success_output / "Meshes/Tiny/Probe.nif") == "tes3 probe nif bytes\n");
    REQUIRE(std::holds_alternative<entry_results>(failure));
    REQUIRE(std::get<entry_results>(failure).size() == 1U);
    REQUIRE(std::get<entry_results>(failure)[0].failure.has_value());
    CHECK(std::get<entry_results>(failure)[0].failure->code == libbsa::error_code::format_error);
    CHECK(read_text(failure_output / "meshes/tiny/packedmesh.nif") == "sentinel");
    CHECK(child_names(failure_output / "meshes/tiny") == std::set<std::wstring>{L"packedmesh.nif"});
    CHECK(child_names(success_output / "Meshes/Tiny") == std::set<std::wstring>{L"Probe.nif"});
}

TEST_CASE("CLI Extraction publishes one winner for simultaneous non-overwriting operations",
          "[unit][cli][fixture]") {
    const auto output = extraction_test_root() / "output";
    // Prepare the common parent so this check concerns competing publication,
    // rather than unrelated directory-creation races.
    std::filesystem::create_directories(output / "Meshes/Tiny");
    auto options = options_for(output);
    options.selected_paths = {"meshes/tiny/probe.nif"};
    std::latch start{2};

    auto first = std::async(std::launch::async, [&] {
        start.arrive_and_wait();
        return libbsa::cli::extract(options);
    });
    auto second = std::async(std::launch::async, [&] {
        start.arrive_and_wait();
        return libbsa::cli::extract(options);
    });
    const auto first_outcome = first.get();
    const auto second_outcome = second.get();

    REQUIRE(std::holds_alternative<entry_results>(first_outcome));
    REQUIRE(std::holds_alternative<entry_results>(second_outcome));
    const auto& first_results = std::get<entry_results>(first_outcome);
    const auto& second_results = std::get<entry_results>(second_outcome);
    REQUIRE(first_results.size() == 1U);
    REQUIRE(second_results.size() == 1U);
    CHECK(first_results[0].succeeded() != second_results[0].succeeded());
    const auto& loser = first_results[0].succeeded() ? second_results[0] : first_results[0];
    REQUIRE(loser.failure.has_value());
    CHECK(loser.failure->code == libbsa::error_code::io_error);
    CHECK(read_text(output / "Meshes/Tiny/Probe.nif") == "tes3 probe nif bytes\n");
    CHECK(child_names(output / "Meshes/Tiny") == std::set<std::wstring>{L"Probe.nif"});
}
