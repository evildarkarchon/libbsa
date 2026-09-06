#include <libbsa/libbsa.hpp>

#include "extraction.hpp"
#include "path_support.hpp"

#include <argparse/argparse.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#endif

namespace {

using libbsa::cli::path_from_utf8;
using libbsa::cli::reject_reparse_point;

enum class process_exit : int {
    success = 0,
    operational_failure = 1,
    usage_error = 2,
};

enum class compression_choice {
    target_default,
    raw,
    compressed,
};

enum class writer_family {
    tes3_bsa,
    tes4_bsa,
    ba2_gnrl,
    ba2_dx10,
};

struct format_descriptor {
    std::string_view token;
    writer_family family;
    std::string_view description;
    libbsa::tes4_bsa_target tes4_target{};
    libbsa::ba2_gnrl_target ba2_gnrl_target{};
    libbsa::ba2_dx10_target ba2_dx10_target{};
    bool supports_raw{true};
    bool supports_compressed{true};
};

struct input_file {
    std::filesystem::path host_path;
    std::string archive_path;
};

// Mirrors detail::run_indexed_work's hard worker cap so oversized CLI values
// fail as usage errors before archive work starts.
constexpr std::uint32_t max_cli_worker_count = 1024U;

constexpr std::array format_table{
    format_descriptor{
        "bsa-tes3", writer_family::tes3_bsa, "TES3/Morrowind BSA (raw)", {}, {}, {}, true, false},
    format_descriptor{"bsa-oblivion",
                      writer_family::tes4_bsa,
                      "TES4 BSA for Oblivion",
                      libbsa::tes4_bsa_target::oblivion,
                      {},
                      {},
                      true,
                      true},
    format_descriptor{"bsa-fo3",
                      writer_family::tes4_bsa,
                      "TES4 BSA for Fallout 3/FNV",
                      libbsa::tes4_bsa_target::fallout3,
                      {},
                      {},
                      true,
                      true},
    format_descriptor{"bsa-sse",
                      writer_family::tes4_bsa,
                      "TES4 BSA for Skyrim Special Edition",
                      libbsa::tes4_bsa_target::skyrim_se,
                      {},
                      {},
                      true,
                      true},
    format_descriptor{"ba2-gnrl-fo4",
                      writer_family::ba2_gnrl,
                      "BA2 GNRL for Fallout 4",
                      {},
                      libbsa::ba2_gnrl_target::fallout4,
                      {},
                      true,
                      true},
    format_descriptor{"ba2-gnrl-sf-v2",
                      writer_family::ba2_gnrl,
                      "BA2 GNRL for Starfield v2",
                      {},
                      libbsa::ba2_gnrl_target::starfield_v2,
                      {},
                      true,
                      true},
    format_descriptor{"ba2-gnrl-sf-v3",
                      writer_family::ba2_gnrl,
                      "BA2 GNRL for Starfield v3",
                      {},
                      libbsa::ba2_gnrl_target::starfield_v3,
                      {},
                      true,
                      true},
    format_descriptor{"ba2-dx10-fo4",
                      writer_family::ba2_dx10,
                      "BA2 DX10 texture archive for Fallout 4",
                      {},
                      {},
                      libbsa::ba2_dx10_target::fallout4,
                      false,
                      true},
    format_descriptor{"ba2-dx10-sf-v2",
                      writer_family::ba2_dx10,
                      "BA2 DX10 texture archive for Starfield v2",
                      {},
                      {},
                      libbsa::ba2_dx10_target::starfield_v2,
                      false,
                      true},
    format_descriptor{"ba2-dx10-sf-v3",
                      writer_family::ba2_dx10,
                      "BA2 DX10 texture archive for Starfield v3",
                      {},
                      {},
                      libbsa::ba2_dx10_target::starfield_v3,
                      false,
                      true},
};

libbsa::error make_error(libbsa::error_code code, std::string message) {
    return libbsa::error{code, std::move(message)};
}

libbsa::error make_usage_error(std::string message) {
    return make_error(libbsa::error_code::invalid_argument, std::move(message));
}

std::string error_code_name(libbsa::error_code code) {
    switch (code) {
        case libbsa::error_code::unsupported:
            return "unsupported";
        case libbsa::error_code::invalid_argument:
            return "invalid_argument";
        case libbsa::error_code::not_found:
            return "not_found";
        case libbsa::error_code::io_error:
            return "io_error";
        case libbsa::error_code::format_error:
            return "format_error";
    }

    return "unknown";
}

std::string archive_type_name(libbsa::archive_type type) {
    switch (type) {
        case libbsa::archive_type::bsa:
            return "bsa";
        case libbsa::archive_type::ba2:
            return "ba2";
    }

    return "unknown";
}

std::string archive_variant_name(libbsa::archive_variant variant) {
    switch (variant) {
        case libbsa::archive_variant::tes3:
            return "tes3";
        case libbsa::archive_variant::tes4:
            return "tes4";
        case libbsa::archive_variant::fallout4:
            return "fallout4";
        case libbsa::archive_variant::starfield:
            return "starfield";
    }

    return "unknown";
}

std::string compression_name(libbsa::entry_compression compression) {
    switch (compression) {
        case libbsa::entry_compression::none:
            return "none";
        case libbsa::entry_compression::deflate:
            return "deflate";
        case libbsa::entry_compression::lz4_frame:
            return "lz4_frame";
        case libbsa::entry_compression::lz4_block:
            return "lz4_block";
    }

    return "unknown";
}

std::string warning_code_name(libbsa::compatibility_warning_code code) {
    switch (code) {
        case libbsa::compatibility_warning_code::compressed_sound_payload:
            return "compressed_sound_payload";
        case libbsa::compatibility_warning_code::bsa_embedded_name_compatibility_risk:
            return "bsa_embedded_name_compatibility_risk";
        case libbsa::compatibility_warning_code::target_family_mismatch:
            return "target_family_mismatch";
        case libbsa::compatibility_warning_code::ba2_record_identity_mismatch:
            return "ba2_record_identity_mismatch";
        case libbsa::compatibility_warning_code::bsa_file_name_table_trailing_bytes:
            return "bsa_file_name_table_trailing_bytes";
        case libbsa::compatibility_warning_code::bsa_folder_name_table_length_mismatch:
            return "bsa_folder_name_table_length_mismatch";
    }

    return "unknown";
}

std::string warning_severity_name(libbsa::compatibility_warning_severity severity) {
    switch (severity) {
        case libbsa::compatibility_warning_severity::advisory:
            return "advisory";
        case libbsa::compatibility_warning_severity::risky:
            return "risky";
    }

    return "unknown";
}

std::string escaped_cli_text(std::string_view text) {
    std::ostringstream output;
    output << std::hex << std::uppercase << std::setfill('0');
    for (const unsigned char ch : text) {
        switch (ch) {
            case '\n':
                output << "\\n";
                break;
            case '\r':
                output << "\\r";
                break;
            case '\t':
                output << "\\t";
                break;
            default:
                if (ch < 0x20U || ch == 0x7FU) {
                    output << "\\x" << std::setw(2) << static_cast<unsigned int>(ch);
                } else {
                    output << static_cast<char>(ch);
                }
                break;
        }
    }
    return output.str();
}

void render_error(const libbsa::error& err, std::string_view context = {}) {
    std::cerr << "error";
    if (!context.empty()) {
        std::cerr << " (" << escaped_cli_text(context) << ")";
    }
    std::cerr << ": " << error_code_name(err.code) << ": " << escaped_cli_text(err.message) << '\n';
}

int usage_failure(const libbsa::error& err, const argparse::ArgumentParser& parser) {
    std::cerr << "usage error: " << escaped_cli_text(err.message) << "\n\n";
    std::cerr << parser.help().str();
    return static_cast<int>(process_exit::usage_error);
}

const format_descriptor* find_format(std::string_view token) {
    const auto found = std::find_if(
        format_table.begin(), format_table.end(),
        [token](const format_descriptor& descriptor) { return descriptor.token == token; });
    return found == format_table.end() ? nullptr : &*found;
}

std::string valid_format_tokens() {
    std::ostringstream output;
    for (std::size_t index = 0; index < format_table.size(); ++index) {
        if (index != 0U) {
            output << ", ";
        }
        output << format_table[index].token;
    }
    return output.str();
}

std::string path_to_utf8(const std::filesystem::path& path) {
    const auto text = path.u8string();
    return {reinterpret_cast<const char*>(text.data()), text.size()};
}

std::string generic_utf8_path(const std::filesystem::path& path) {
    const auto text = path.generic_u8string();
    return {reinterpret_cast<const char*>(text.data()), text.size()};
}

#if defined(_WIN32)
struct local_command_line_argv {
    wchar_t** value{};

    explicit local_command_line_argv(wchar_t** argv) noexcept : value(argv) {}
    local_command_line_argv(const local_command_line_argv&) = delete;
    local_command_line_argv& operator=(const local_command_line_argv&) = delete;
    ~local_command_line_argv() {
        if (value != nullptr) {
            ::LocalFree(value);
        }
    }
};

libbsa::result<std::string> wide_argument_to_utf8(std::wstring_view argument) {
    if (argument.empty()) {
        return std::string{};
    }
    if (argument.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
        return make_error(libbsa::error_code::invalid_argument,
                          "Windows command-line argument is too long to encode as UTF-8");
    }

    // Windows exposes the authoritative process command line as UTF-16; encode it
    // once to honor the CLI/library UTF-8 host-path contract instead of using CRT
    // ANSI-code-page argv bytes.
    const auto source_size = static_cast<int>(argument.size());
    const auto utf8_size = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argument.data(),
                                                 source_size, nullptr, 0, nullptr, nullptr);
    if (utf8_size <= 0) {
        return make_error(libbsa::error_code::invalid_argument,
                          "Windows command-line argument is not valid Unicode");
    }

    std::string utf8_argument(static_cast<std::size_t>(utf8_size), '\0');
    const auto converted =
        ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argument.data(), source_size,
                              utf8_argument.data(), utf8_size, nullptr, nullptr);
    if (converted != utf8_size) {
        return make_error(libbsa::error_code::invalid_argument,
                          "Windows command-line argument is not valid Unicode");
    }
    return utf8_argument;
}
#endif

libbsa::result<std::vector<std::string>> command_line_arguments(int argc, char** argv) {
    std::vector<std::string> arguments;

#if defined(_WIN32)
    (void)argc;
    (void)argv;

    int wide_argc = 0;
    local_command_line_argv wide_argv{::CommandLineToArgvW(::GetCommandLineW(), &wide_argc)};
    if (wide_argv.value == nullptr) {
        return make_error(libbsa::error_code::invalid_argument,
                          "cannot parse Windows command line");
    }

    arguments.reserve(wide_argc > 1 ? static_cast<std::size_t>(wide_argc - 1) : 0U);
    for (int index = 1; index < wide_argc; ++index) {
        auto utf8_argument = wide_argument_to_utf8(wide_argv.value[index]);
        if (!utf8_argument) {
            return utf8_argument.error();
        }
        arguments.push_back(std::move(utf8_argument).value());
    }
#else
    arguments.reserve(argc > 1 ? static_cast<std::size_t>(argc - 1) : 0U);
    for (int index = 1; index < argc; ++index) {
        arguments.emplace_back(argv[index]);
    }
#endif

    return arguments;
}

void print_format_table(std::ostream& output) {
    output << "Formats:\n";
    for (const auto& format : format_table) {
        output << "  " << std::left << std::setw(18) << format.token << format.description << '\n';
    }
}

std::string format_table_help() {
    std::ostringstream output;
    print_format_table(output);
    return output.str();
}

std::string version_text() {
    return std::to_string(libbsa::version_major) + "." + std::to_string(libbsa::version_minor) +
           "." + std::to_string(libbsa::version_patch);
}

void print_version(std::ostream& output) {
    output << "bsa (libbsa " << libbsa::version_major << '.' << libbsa::version_minor << '.'
           << libbsa::version_patch << ")\n";
}

void add_help_argument(argparse::ArgumentParser& parser) {
    parser.add_argument("-h", "--help").help("Show help").flag();
}

void add_thread_argument(argparse::ArgumentParser& parser) {
    parser.add_argument("-j", "--threads")
        .metavar("<value>")
        .default_value(std::string{"auto"})
        .help("Worker threads: -j <value>; 1..1024, auto, or 0 for auto");
}

void add_positionals_argument(argparse::ArgumentParser& parser, std::string_view metavar) {
    parser.add_argument("positionals")
        .metavar(std::string{metavar})
        .nargs(argparse::nargs_pattern::any)
        .default_value<std::vector<std::string>>({});
}

std::vector<std::string> positional_values(const argparse::ArgumentParser& parser) {
    return parser.get<std::vector<std::string>>("positionals");
}

bool is_known_subcommand(std::string_view command) noexcept {
    return command == "pack" || command == "unpack" || command == "list" || command == "info" ||
           command == "validate";
}

bool is_global_option(std::string_view command) noexcept {
    return command == "-h" || command == "--help" || command == "-V" || command == "--version";
}

libbsa::result<compression_choice> parse_compression(std::string_view value) {
    if (value == "default") {
        return compression_choice::target_default;
    }
    if (value == "raw") {
        return compression_choice::raw;
    }
    if (value == "compressed") {
        return compression_choice::compressed;
    }
    return make_usage_error("unknown --compress value '" + std::string{value} + "'");
}

/// Parses a CLI worker-count token into a concrete positive library worker count.
/// The CLI resolves `auto` and `0` because public libbsa APIs keep `0` invalid.
libbsa::result<std::uint32_t> resolve_worker_count(std::string_view value) {
    const auto invalid_worker_count = [value] {
        return make_usage_error("invalid --threads value '" + std::string{value} +
                                "'; expected 1.." + std::to_string(max_cli_worker_count) +
                                ", auto, or 0");
    };

    if (value.empty()) {
        return invalid_worker_count();
    }

    if (value == "auto" || value == "0") {
        const auto hardware_workers = std::thread::hardware_concurrency();
        return hardware_workers == 0U ? 1U : hardware_workers;
    }

    std::uint32_t parsed = 0U;
    const auto* begin = value.data();
    const auto* end = value.data() + value.size();
    const auto [parsed_end, error] = std::from_chars(begin, end, parsed, 10);
    if (error != std::errc{} || parsed_end != end || parsed == 0U ||
        parsed > max_cli_worker_count) {
        return invalid_worker_count();
    }

    return parsed;
}

libbsa::archive_compression_policy archive_policy_from(compression_choice choice) {
    switch (choice) {
        case compression_choice::target_default:
            return libbsa::archive_compression_policy::target_default;
        case compression_choice::raw:
            return libbsa::archive_compression_policy::all_raw;
        case compression_choice::compressed:
            return libbsa::archive_compression_policy::all_compressed;
    }

    return libbsa::archive_compression_policy::target_default;
}

libbsa::result<std::vector<input_file>> collect_input_files(
    const std::filesystem::path& input_dir) {
    std::error_code fs_error;
    const auto root = std::filesystem::absolute(input_dir, fs_error).lexically_normal();
    if (fs_error) {
        return make_error(
            libbsa::error_code::io_error,
            "cannot resolve input directory '" + input_dir.string() + "': " + fs_error.message());
    }
    if (!std::filesystem::exists(root, fs_error)) {
        return make_error(libbsa::error_code::io_error,
                          "input directory does not exist: " + root.string());
    }
    if (fs_error || !std::filesystem::is_directory(root, fs_error)) {
        return make_error(libbsa::error_code::io_error,
                          "input path is not a directory: " + root.string());
    }
    auto root_reparse = reject_reparse_point(root, "input directory");
    if (!root_reparse) {
        return root_reparse.error();
    }

    std::vector<input_file> files;
    std::filesystem::recursive_directory_iterator iterator{
        root, std::filesystem::directory_options::none, fs_error};
    if (fs_error) {
        return make_error(
            libbsa::error_code::io_error,
            "cannot enumerate input directory '" + root.string() + "': " + fs_error.message());
    }

    const std::filesystem::recursive_directory_iterator end;
    for (; iterator != end;) {
        const auto& entry = *iterator;
        auto input_reparse = reject_reparse_point(entry.path(), "input path");
        if (!input_reparse) {
            return input_reparse.error();
        }

        const bool regular = entry.is_regular_file(fs_error);
        if (fs_error) {
            return make_error(
                libbsa::error_code::io_error,
                "cannot inspect input path '" + entry.path().string() + "': " + fs_error.message());
        }
        if (regular) {
            const auto relative = entry.path().lexically_normal().lexically_relative(root);
            const auto archive_path = generic_utf8_path(relative);
            if (archive_path.empty() || archive_path == ".") {
                return make_error(
                    libbsa::error_code::io_error,
                    "cannot derive archive path for input file: " + entry.path().string());
            }
            files.push_back(input_file{entry.path(), archive_path});
        }

        iterator.increment(fs_error);
        if (fs_error) {
            return make_error(libbsa::error_code::io_error,
                              "directory enumeration failed: " + fs_error.message());
        }
    }

    std::sort(files.begin(), files.end(), [](const input_file& lhs, const input_file& rhs) {
        return lhs.archive_path < rhs.archive_path;
    });
    return files;
}

int pack_tes3(const std::vector<input_file>& files, const std::filesystem::path& output_path,
              bool overwrite, std::uint32_t worker_count) {
    libbsa::tes3_bsa_writer_options options;
    options.overwrite_existing = overwrite;
    libbsa::tes3_bsa_writer writer{options};

    for (const auto& file : files) {
        auto added = writer.add_file(file.archive_path, path_to_utf8(file.host_path));
        if (!added) {
            render_error(added.error(), file.archive_path);
            return static_cast<int>(process_exit::operational_failure);
        }
    }

    auto written =
        writer.write_to(path_to_utf8(output_path), libbsa::write_execution_options{worker_count});
    if (!written) {
        render_error(written.error(), path_to_utf8(output_path));
        return static_cast<int>(process_exit::operational_failure);
    }
    return static_cast<int>(process_exit::success);
}

int pack_tes4(const format_descriptor& format, const std::vector<input_file>& files,
              const std::filesystem::path& output_path, bool overwrite,
              compression_choice compression, std::uint32_t worker_count) {
    libbsa::tes4_bsa_writer_options options;
    options.overwrite_existing = overwrite;
    options.compression_policy = archive_policy_from(compression);
    libbsa::tes4_bsa_writer writer{format.tes4_target, options};

    for (const auto& file : files) {
        auto added = writer.add_file(file.archive_path, path_to_utf8(file.host_path));
        if (!added) {
            render_error(added.error(), file.archive_path);
            return static_cast<int>(process_exit::operational_failure);
        }
    }

    auto written =
        writer.write_to(path_to_utf8(output_path), libbsa::write_execution_options{worker_count});
    if (!written) {
        render_error(written.error(), path_to_utf8(output_path));
        return static_cast<int>(process_exit::operational_failure);
    }
    return static_cast<int>(process_exit::success);
}

int pack_ba2_gnrl(const format_descriptor& format, const std::vector<input_file>& files,
                  const std::filesystem::path& output_path, bool overwrite,
                  compression_choice compression, std::uint32_t worker_count) {
    libbsa::ba2_gnrl_writer_options options;
    options.overwrite_existing = overwrite;
    options.compression = archive_policy_from(compression);
    libbsa::ba2_gnrl_writer writer{format.ba2_gnrl_target, options};

    for (const auto& file : files) {
        auto added = writer.add_file(file.archive_path, path_to_utf8(file.host_path));
        if (!added) {
            render_error(added.error(), file.archive_path);
            return static_cast<int>(process_exit::operational_failure);
        }
    }

    auto written =
        writer.write_to(path_to_utf8(output_path), libbsa::write_execution_options{worker_count});
    if (!written) {
        render_error(written.error(), path_to_utf8(output_path));
        return static_cast<int>(process_exit::operational_failure);
    }
    return static_cast<int>(process_exit::success);
}

int pack_ba2_dx10(const format_descriptor& format, const std::vector<input_file>& files,
                  const std::filesystem::path& output_path, bool overwrite,
                  std::uint32_t worker_count) {
    libbsa::ba2_dx10_writer_options options;
    options.overwrite_existing = overwrite;
    libbsa::ba2_dx10_writer writer{format.ba2_dx10_target, options};

    for (const auto& file : files) {
        auto added = writer.add_file(file.archive_path, path_to_utf8(file.host_path));
        if (!added) {
            render_error(added.error(), file.archive_path);
            return static_cast<int>(process_exit::operational_failure);
        }
    }

    auto written =
        writer.write_to(path_to_utf8(output_path), libbsa::write_execution_options{worker_count});
    if (!written) {
        render_error(written.error(), path_to_utf8(output_path));
        return static_cast<int>(process_exit::operational_failure);
    }
    return static_cast<int>(process_exit::success);
}

/// Runs `bsa pack` after argparse syntax parsing, preserving CLI semantic checks.
/// Usage failures return exit code 2; archive and filesystem failures stay on `render_error`.
int run_pack(const argparse::ArgumentParser& parser) {
    const auto positionals = positional_values(parser);
    if (positionals.size() != 2U) {
        return usage_failure(make_usage_error("pack requires <input-dir> and <output-archive>"),
                             parser);
    }

    const auto format_value = parser.present<std::string>("--format");
    if (!format_value.has_value()) {
        return usage_failure(make_usage_error("pack requires --format"), parser);
    }
    const format_descriptor* format = find_format(*format_value);
    if (format == nullptr) {
        return usage_failure(make_usage_error("unknown --format token '" + *format_value +
                                              "'; valid tokens: " + valid_format_tokens()),
                             parser);
    }

    compression_choice compression = compression_choice::target_default;
    auto parsed_compression = parse_compression(parser.get<std::string>("--compress"));
    if (!parsed_compression) {
        return usage_failure(parsed_compression.error(), parser);
    }
    compression = parsed_compression.value();

    if (compression == compression_choice::raw && !format->supports_raw) {
        return usage_failure(make_usage_error("selected --format does not support raw compression"),
                             parser);
    }
    if (compression == compression_choice::compressed && !format->supports_compressed) {
        return usage_failure(
            make_usage_error("selected --format does not support compressed payloads"), parser);
    }

    auto resolved_worker_count = resolve_worker_count(parser.get<std::string>("--threads"));
    if (!resolved_worker_count) {
        return usage_failure(resolved_worker_count.error(), parser);
    }
    const std::uint32_t worker_count = resolved_worker_count.value();

    auto input_dir = path_from_utf8(positionals[0]);
    if (!input_dir) {
        render_error(input_dir.error(), positionals[0]);
        return static_cast<int>(process_exit::operational_failure);
    }
    auto output_path = path_from_utf8(positionals[1]);
    if (!output_path) {
        render_error(output_path.error(), positionals[1]);
        return static_cast<int>(process_exit::operational_failure);
    }

    auto files = collect_input_files(input_dir.value());
    if (!files) {
        render_error(files.error());
        return static_cast<int>(process_exit::operational_failure);
    }

    const bool overwrite = parser.get<bool>("--overwrite");
    int exit_code = static_cast<int>(process_exit::operational_failure);
    switch (format->family) {
        case writer_family::tes3_bsa:
            exit_code = pack_tes3(files.value(), output_path.value(), overwrite, worker_count);
            break;
        case writer_family::tes4_bsa:
            exit_code = pack_tes4(*format, files.value(), output_path.value(), overwrite,
                                  compression, worker_count);
            break;
        case writer_family::ba2_gnrl:
            exit_code = pack_ba2_gnrl(*format, files.value(), output_path.value(), overwrite,
                                      compression, worker_count);
            break;
        case writer_family::ba2_dx10:
            exit_code =
                pack_ba2_dx10(*format, files.value(), output_path.value(), overwrite, worker_count);
            break;
    }

    if (exit_code == static_cast<int>(process_exit::success)) {
        std::cout << "packed " << files.value().size() << " file(s) into "
                  << output_path.value().string() << '\n';
    }
    return exit_code;
}

/// Runs `bsa unpack` using parsed subcommand values and guarded destination staging.
/// CLI Extraction owns execution; this command owns argument parsing and result presentation.
int run_unpack(const argparse::ArgumentParser& parser) {
    const auto positionals = positional_values(parser);
    if (positionals.size() != 2U) {
        return usage_failure(make_usage_error("unpack requires <archive> and <output-dir>"),
                             parser);
    }

    auto resolved_worker_count = resolve_worker_count(parser.get<std::string>("--threads"));
    if (!resolved_worker_count) {
        return usage_failure(resolved_worker_count.error(), parser);
    }
    libbsa::cli::extraction_options options{
        .archive_path = positionals[0],
        .output_directory = positionals[1],
        .selected_paths = parser.get<std::vector<std::string>>("--path"),
        .overwrite = parser.get<bool>("--overwrite"),
        .worker_count = resolved_worker_count.value()};
    auto outcome = libbsa::cli::extract(options);
    if (const auto* failure = std::get_if<libbsa::cli::extraction_failure>(&outcome)) {
        render_error(failure->failure, failure->context);
        return static_cast<int>(process_exit::operational_failure);
    }
    const auto& extracted = std::get<std::vector<libbsa::bulk_extract_entry_result>>(outcome);

    // Destinations are already published or discarded incrementally by
    // CLI Extraction during execution; surface any per-entry failure
    // (including a publish failure promoted by finish) here.
    std::size_t failure_count = 0;
    for (const auto& result : extracted) {
        if (result.failure.has_value()) {
            ++failure_count;
            render_error(*result.failure, result.path);
        }
    }

    std::cout << "extracted " << (extracted.size() - failure_count) << " of " << extracted.size()
              << " requested entr" << (extracted.size() == 1U ? "y" : "ies") << '\n';
    return failure_count == 0U ? static_cast<int>(process_exit::success)
                               : static_cast<int>(process_exit::operational_failure);
}

/// Runs `bsa list`, preserving escaped archive-path output and optional detail columns.
int run_list(const argparse::ArgumentParser& parser) {
    const auto positionals = positional_values(parser);
    if (positionals.size() != 1U) {
        return usage_failure(make_usage_error("list requires <archive>"), parser);
    }

    auto opened = libbsa::archive_reader::open(positionals[0]);
    if (!opened) {
        render_error(opened.error(), positionals[0]);
        return static_cast<int>(process_exit::operational_failure);
    }
    auto entries = opened.value().entries();
    if (!entries) {
        render_error(entries.error(), positionals[0]);
        return static_cast<int>(process_exit::operational_failure);
    }

    const bool details = parser.get<bool>("--details");
    for (const auto& entry : entries.value()) {
        std::cout << escaped_cli_text(entry.path);
        if (details) {
            std::cout << " raw=" << entry.raw_size << " stored=" << entry.stored_size
                      << " compression=" << compression_name(entry.compression);
        }
        std::cout << '\n';
    }
    return static_cast<int>(process_exit::success);
}

/// Runs `bsa info` and emits the existing archive metadata text format.
int run_info(const argparse::ArgumentParser& parser) {
    const auto positionals = positional_values(parser);
    if (positionals.size() != 1U) {
        return usage_failure(make_usage_error("info requires <archive>"), parser);
    }

    auto opened = libbsa::archive_reader::open(positionals[0]);
    if (!opened) {
        render_error(opened.error(), positionals[0]);
        return static_cast<int>(process_exit::operational_failure);
    }
    auto metadata = opened.value().metadata();
    if (!metadata) {
        render_error(metadata.error(), positionals[0]);
        return static_cast<int>(process_exit::operational_failure);
    }

    std::cout << "type: " << archive_type_name(metadata.value().type) << '\n'
              << "variant: " << archive_variant_name(metadata.value().variant) << '\n'
              << "version: " << metadata.value().version << '\n'
              << "archive_flags: 0x" << std::hex << metadata.value().archive_flags << std::dec
              << '\n'
              << "file_count: " << metadata.value().file_count << '\n'
              << "default_compression: " << compression_name(metadata.value().default_compression)
              << '\n';
    if (metadata.value().ba2.has_value()) {
        const auto& ba2 = *metadata.value().ba2;
        if (ba2.starfield_unknown1.has_value()) {
            std::cout << "ba2.starfield_unknown1: " << *ba2.starfield_unknown1 << '\n';
        }
        if (ba2.starfield_unknown2.has_value()) {
            std::cout << "ba2.starfield_unknown2: " << *ba2.starfield_unknown2 << '\n';
        }
        if (ba2.compression_method.has_value()) {
            std::cout << "ba2.compression_method: " << *ba2.compression_method << '\n';
        }
    }
    return static_cast<int>(process_exit::success);
}

/// Runs `bsa validate`, keeping warning output and strict-warning failure semantics.
int run_validate(const argparse::ArgumentParser& parser) {
    const auto positionals = positional_values(parser);
    if (positionals.size() != 1U) {
        return usage_failure(make_usage_error("validate requires <archive>"), parser);
    }

    auto validated = libbsa::validate_archive(positionals[0]);
    if (!validated) {
        render_error(validated.error(), positionals[0]);
        return static_cast<int>(process_exit::operational_failure);
    }

    const auto& report = validated.value();
    std::cout << "valid: " << (report.is_valid() ? "yes" : "no") << '\n';
    if (report.metadata.has_value()) {
        std::cout << "type: " << archive_type_name(report.metadata->type) << '\n'
                  << "variant: " << archive_variant_name(report.metadata->variant) << '\n'
                  << "file_count: " << report.metadata->file_count << '\n';
    }
    for (const auto& diagnostic : report.errors) {
        std::cout << "error: " << error_code_name(diagnostic.code) << ": "
                  << escaped_cli_text(diagnostic.message) << '\n';
    }
    for (const auto& warning : report.warnings) {
        std::cout << "warning: " << warning_code_name(warning.code)
                  << " severity=" << warning_severity_name(warning.severity)
                  << " message=" << escaped_cli_text(warning.message);
        if (warning.archive_path.has_value()) {
            std::cout << " path=" << escaped_cli_text(*warning.archive_path);
        }
        std::cout << '\n';
    }

    if (!report.is_valid() || (parser.get<bool>("--strict") && !report.warnings.empty())) {
        return static_cast<int>(process_exit::operational_failure);
    }
    return static_cast<int>(process_exit::success);
}

/// Builds the argparse grammar, parses the UTF-8 argument vector, and dispatches one command.
/// Parser objects stay in this scope because argparse v3 parsers are non-copyable/non-movable
/// and registered subparsers must outlive parse_args and dispatch.
int dispatch(const std::vector<std::string>& args) {
    argparse::ArgumentParser program("bsa", version_text(), argparse::default_arguments::none);
    program.add_description("Bethesda archive command-line tool");
    add_help_argument(program);
    program.add_argument("-V", "--version").help("Show version").flag();

    argparse::ArgumentParser pack_parser("pack", version_text(), argparse::default_arguments::none);
    pack_parser.add_description("Create a new archive from a directory");
    add_help_argument(pack_parser);
    pack_parser.add_argument("--format")
        .metavar("<token>")
        .help("Required archive writer/target token. Valid tokens: " + valid_format_tokens());
    pack_parser.add_argument("--compress")
        .metavar("<value>")
        .default_value(std::string{"default"})
        .help("Compression policy: default, raw, or compressed");
    add_thread_argument(pack_parser);
    pack_parser.add_argument("--overwrite").help("Replace an existing output archive").flag();
    add_positionals_argument(pack_parser, "<input-dir> <output-archive>");
    pack_parser.add_epilog(format_table_help());

    argparse::ArgumentParser unpack_parser("unpack", version_text(),
                                           argparse::default_arguments::none);
    unpack_parser.add_description("Extract archive entries to a directory");
    add_help_argument(unpack_parser);
    unpack_parser.add_argument("--path")
        .metavar("<archive-path>")
        .default_value<std::vector<std::string>>({})
        .append()
        .help("Extract only the requested archive path; may repeat");
    add_thread_argument(unpack_parser);
    unpack_parser.add_argument("--overwrite").help("Replace existing destination files").flag();
    add_positionals_argument(unpack_parser, "<archive> <output-dir>");

    argparse::ArgumentParser list_parser("list", version_text(), argparse::default_arguments::none);
    list_parser.add_description("List archive entries");
    add_help_argument(list_parser);
    list_parser.add_argument("--details", "--detail")
        .help("Include raw size, stored size, and compression")
        .flag();
    add_positionals_argument(list_parser, "<archive>");

    argparse::ArgumentParser info_parser("info", version_text(), argparse::default_arguments::none);
    info_parser.add_description("Print archive metadata");
    add_help_argument(info_parser);
    add_positionals_argument(info_parser, "<archive>");

    argparse::ArgumentParser validate_parser("validate", version_text(),
                                             argparse::default_arguments::none);
    validate_parser.add_description("Validate an archive");
    add_help_argument(validate_parser);
    validate_parser.add_argument("--strict")
        .help("Treat compatibility warnings as failures")
        .flag();
    add_positionals_argument(validate_parser, "<archive>");

    program.add_subparser(pack_parser);
    program.add_subparser(unpack_parser);
    program.add_subparser(list_parser);
    program.add_subparser(info_parser);
    program.add_subparser(validate_parser);

    struct command_route {
        std::string_view name;
        argparse::ArgumentParser* parser;
        int (*handler)(const argparse::ArgumentParser&);
    };
    const std::array<command_route, 5> routes{{
        {"pack", &pack_parser, run_pack},
        {"unpack", &unpack_parser, run_unpack},
        {"list", &list_parser, run_list},
        {"info", &info_parser, run_info},
        {"validate", &validate_parser, run_validate},
    }};

    if (args.empty()) {
        std::cerr << "usage error: missing subcommand\n\n" << program.help().str();
        return static_cast<int>(process_exit::usage_error);
    }

    const auto first_token = std::string_view{args.front()};
    if (!is_global_option(first_token) && !is_known_subcommand(first_token) &&
        !first_token.empty() && first_token.front() != '-') {
        std::cerr << "usage error: unknown subcommand '" << first_token << "'\n\n"
                  << program.help().str();
        return static_cast<int>(process_exit::usage_error);
    }

    std::vector<std::string> parser_args;
    parser_args.reserve(args.size() + 1U);
    parser_args.emplace_back("bsa");
    parser_args.insert(parser_args.end(), args.begin(), args.end());

    const auto usage_parser_for_error = [&]() -> const argparse::ArgumentParser& {
        for (const auto& route : routes) {
            if (first_token == route.name) {
                return *route.parser;
            }
        }
        return program;
    };

    try {
        program.parse_args(parser_args);
    } catch (const std::exception& ex) {
        // argparse reports syntax failures with exceptions; translate them at the CLI boundary
        // so parse errors keep the documented usage-error exit code instead of escaping main().
        std::cerr << "usage error: " << escaped_cli_text(ex.what()) << "\n\n"
                  << usage_parser_for_error().help().str();
        return static_cast<int>(process_exit::usage_error);
    }

    if (program.get<bool>("--version")) {
        print_version(std::cout);
        return static_cast<int>(process_exit::success);
    }
    if (program.get<bool>("--help")) {
        std::cout << program.help().str();
        return static_cast<int>(process_exit::success);
    }

    for (const auto& route : routes) {
        if (!program.is_subcommand_used(std::string{route.name})) {
            continue;
        }
        if (route.parser->get<bool>("--help")) {
            std::cout << route.parser->help().str();
            return static_cast<int>(process_exit::success);
        }
        return route.handler(*route.parser);
    }

    std::cerr << "usage error: missing subcommand\n\n" << program.help().str();
    return static_cast<int>(process_exit::usage_error);
}
}  // namespace

int main(int argc, char** argv) {
    try {
        auto owned_args = command_line_arguments(argc, argv);
        if (!owned_args) {
            render_error(owned_args.error());
            return static_cast<int>(process_exit::operational_failure);
        }

        return dispatch(owned_args.value());
    } catch (const std::exception& ex) {
        std::cerr << "error: unhandled exception: " << ex.what() << '\n';
        return static_cast<int>(process_exit::operational_failure);
    } catch (...) {
        std::cerr << "error: unhandled non-standard exception\n";
        return static_cast<int>(process_exit::operational_failure);
    }
}
