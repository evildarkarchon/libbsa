#include <libbsa/libbsa.hpp>

#include <argparse/argparse.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
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

libbsa::result<std::filesystem::path> path_from_utf8(std::string_view utf8_path) {
    if (utf8_path.find('\0') != std::string_view::npos) {
        return make_error(libbsa::error_code::invalid_argument, "path contains embedded NUL bytes");
    }
    // Empty host paths resolve as the current working directory in later
    // std::filesystem::absolute() calls, which would silently pack or extract CWD.
    if (utf8_path.empty()) {
        return make_error(libbsa::error_code::invalid_argument, "path is empty");
    }

#if defined(_WIN32)
    if (utf8_path.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
        return make_error(libbsa::error_code::invalid_argument,
                          "path is too long to decode as UTF-8");
    }

    // MSVC decodes narrow filesystem paths through the active ANSI code page, but
    // archive paths are UTF-8.
    const auto source_size = static_cast<int>(utf8_path.size());
    const auto wide_size = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_path.data(),
                                                 source_size, nullptr, 0);
    if (wide_size <= 0) {
        return make_error(libbsa::error_code::invalid_argument, "path is not valid UTF-8");
    }

    std::wstring wide_path(static_cast<std::size_t>(wide_size), L'\0');
    const auto converted = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_path.data(),
                                                 source_size, wide_path.data(), wide_size);
    if (converted != wide_size) {
        return make_error(libbsa::error_code::invalid_argument, "path is not valid UTF-8");
    }

    return std::filesystem::path{std::move(wide_path)};
#else
    std::u8string path;
    path.reserve(utf8_path.size());
    for (const unsigned char ch : utf8_path) {
        path.push_back(static_cast<char8_t>(ch));
    }
    return std::filesystem::path{std::move(path)};
#endif
}

#if defined(_WIN32)
char lower_ascii(char value) noexcept {
    if (value >= 'A' && value <= 'Z') {
        return static_cast<char>(value - 'A' + 'a');
    }
    return value;
}

bool ascii_iequals(std::string_view lhs, std::string_view rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        if (lower_ascii(lhs[index]) != rhs[index]) {
            return false;
        }
    }
    return true;
}

std::string_view windows_reserved_device_stem(std::string_view component) noexcept {
    auto stem = component.substr(0U, component.find('.'));
    while (!stem.empty() && (stem.back() == ' ' || stem.back() == '.')) {
        stem.remove_suffix(1U);
    }
    return stem;
}

bool is_windows_reserved_device_name(std::string_view component) noexcept {
    const auto stem = windows_reserved_device_stem(component);
    if (ascii_iequals(stem, "con") || ascii_iequals(stem, "prn") || ascii_iequals(stem, "aux") ||
        ascii_iequals(stem, "nul") || ascii_iequals(stem, "conin$") ||
        ascii_iequals(stem, "conout$")) {
        return true;
    }
    if (stem.size() >= 4U) {
        const auto prefix = stem.substr(0U, 3U);
        if (!ascii_iequals(prefix, "com") && !ascii_iequals(prefix, "lpt")) {
            return false;
        }

        const auto suffix = stem.substr(3U);
        // Win32 also treats superscript 1/2/3 as reserved COM/LPT suffixes.
        return (suffix.size() == 1U && suffix.front() >= '1' && suffix.front() <= '9') ||
               suffix == std::string_view{"\xC2\xB9", 2U} ||
               suffix == std::string_view{"\xC2\xB2", 2U} ||
               suffix == std::string_view{"\xC2\xB3", 2U};
    }
    return false;
}

libbsa::result<void> reject_windows_unsafe_destination_components(
    std::string_view normalized_entry) {
    for (std::size_t start = 0U; start <= normalized_entry.size();) {
        const auto slash = normalized_entry.find('/', start);
        const auto end = slash == std::string_view::npos ? normalized_entry.size() : slash;
        const auto component = normalized_entry.substr(start, end - start);

        if (component.find(':') != std::string_view::npos) {
            return make_error(
                libbsa::error_code::invalid_argument,
                "refusing colon in archive entry path component: " + std::string{normalized_entry});
        }
        if (!component.empty() && (component.back() == '.' || component.back() == ' ')) {
            return make_error(libbsa::error_code::invalid_argument,
                              "refusing trailing dot or space in archive entry path component '" +
                                  std::string{component} + "': " + std::string{normalized_entry});
        }
        // Win32 resolves DOS device names as special files even when an extension
        // is present, such as NUL.txt.
        if (is_windows_reserved_device_name(component)) {
            return make_error(libbsa::error_code::invalid_argument,
                              "refusing Windows-reserved archive entry path component '" +
                                  std::string{component} + "': " + std::string{normalized_entry});
        }

        if (slash == std::string_view::npos) {
            break;
        }
        start = slash + 1U;
    }
    return {};
}
#endif

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

bool is_within_root(const std::filesystem::path& root, const std::filesystem::path& candidate);

libbsa::result<bool> is_reparse_point(const std::filesystem::path& path, std::string_view context) {
#if defined(_WIN32)
    const auto attributes = ::GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        const auto last_error = ::GetLastError();
        if (last_error == ERROR_FILE_NOT_FOUND || last_error == ERROR_PATH_NOT_FOUND) {
            return false;
        }
        return make_error(
            libbsa::error_code::io_error,
            "cannot inspect " + std::string{context} + " for reparse points: " + path.string());
    }
    return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
    (void)path;
    (void)context;
    return false;
#endif
}

libbsa::result<void> reject_reparse_point(const std::filesystem::path& path,
                                          std::string_view context) {
    auto reparse = is_reparse_point(path, context);
    if (!reparse) {
        return reparse.error();
    }
    if (reparse.value()) {
        return make_error(libbsa::error_code::io_error,
                          "refusing reparse-point " + std::string{context} + ": " + path.string());
    }
    return {};
}

/// Rejects reparse points on `directory` and on every ancestor up to `root`.
///
/// Checks run outermost-first so the redirection closest to the output root is the
/// one reported, which is the order the per-entry ancestor walk used before the
/// unpack pre-pass hoisted this work out of the sink factory. Ancestors outside
/// `root` are left alone: the output root itself is validated by
/// `prepare_output_root`, and whatever sits above it is not this command's to
/// judge.
libbsa::result<void> reject_reparse_chain(const std::filesystem::path& root,
                                          const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> chain;
    for (auto current = directory; !current.empty(); current = current.parent_path()) {
        if (!is_within_root(root, current)) {
            break;
        }
        chain.push_back(current);
        if (current.lexically_normal() == root.lexically_normal()) {
            break;
        }
    }

    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        auto checked = reject_reparse_point(*it, "destination parent");
        if (!checked) {
            return checked.error();
        }
    }
    return {};
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

/// Payload sink that writes every extracted byte to one host file.
#if defined(_WIN32)
struct unique_windows_handle {
    HANDLE value{INVALID_HANDLE_VALUE};

    unique_windows_handle() noexcept = default;
    explicit unique_windows_handle(HANDLE handle) noexcept : value(handle) {}
    unique_windows_handle(const unique_windows_handle&) = delete;
    unique_windows_handle& operator=(const unique_windows_handle&) = delete;

    unique_windows_handle(unique_windows_handle&& other) noexcept : value(other.value) {
        other.value = INVALID_HANDLE_VALUE;
    }

    unique_windows_handle& operator=(unique_windows_handle&& other) noexcept {
        if (this != &other) {
            reset();
            value = other.value;
            other.value = INVALID_HANDLE_VALUE;
        }
        return *this;
    }

    ~unique_windows_handle() { reset(); }

    explicit operator bool() const noexcept {
        return value != nullptr && value != INVALID_HANDLE_VALUE;
    }

    void reset() noexcept {
        if (*this) {
            ::CloseHandle(value);
        }
        value = INVALID_HANDLE_VALUE;
    }
};

std::string windows_error_message(DWORD error_code) {
    return std::system_category().message(static_cast<int>(error_code));
}

libbsa::error windows_io_error(std::string action, DWORD error_code) {
    return make_error(libbsa::error_code::io_error,
                      std::move(action) + ": " + windows_error_message(error_code));
}

libbsa::result<std::wstring> final_path_for_handle(HANDLE handle, std::string_view context) {
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;) {
        const DWORD written =
            ::GetFinalPathNameByHandleW(handle, buffer.data(), static_cast<DWORD>(buffer.size()),
                                        FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
        if (written == 0U) {
            return windows_io_error("cannot resolve final " + std::string{context} + " path",
                                    ::GetLastError());
        }
        if (written < buffer.size()) {
            buffer.resize(written);
            return buffer;
        }
        buffer.assign(static_cast<std::size_t>(written) + 1U, L'\0');
    }
}

bool is_separator(wchar_t value) noexcept { return value == L'\\' || value == L'/'; }

std::wstring trim_final_path(std::wstring path) {
    while (path.size() > 1U && is_separator(path.back())) {
        if (path.size() >= 7U && path.rfind(L"\\\\?\\", 0U) == 0U && path[5] == L':' &&
            path.size() == 7U) {
            break;
        }
        path.pop_back();
    }
    return path;
}

bool same_windows_prefix(std::wstring_view lhs, std::wstring_view rhs) noexcept {
    if (lhs.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()) ||
        rhs.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
        return false;
    }
    return ::CompareStringOrdinal(lhs.data(), static_cast<int>(lhs.size()), rhs.data(),
                                  static_cast<int>(rhs.size()), TRUE) == CSTR_EQUAL;
}

bool final_path_is_within_root(std::wstring root, std::wstring candidate) {
    root = trim_final_path(std::move(root));
    candidate = trim_final_path(std::move(candidate));
    if (candidate.size() < root.size()) {
        return false;
    }
    if (!same_windows_prefix(std::wstring_view{candidate}.substr(0U, root.size()), root)) {
        return false;
    }
    return candidate.size() == root.size() || is_separator(candidate[root.size()]);
}

libbsa::result<bool> handle_is_reparse_point(HANDLE handle, std::string_view context) {
    FILE_ATTRIBUTE_TAG_INFO info{};
    if (!::GetFileInformationByHandleEx(handle, FileAttributeTagInfo, &info, sizeof(info))) {
        return windows_io_error("cannot inspect " + std::string{context} + " handle attributes",
                                ::GetLastError());
    }
    return (info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0U;
}

void mark_delete_on_close(HANDLE handle) noexcept {
    FILE_DISPOSITION_INFO disposition{};
    disposition.DeleteFile = TRUE;
    (void)::SetFileInformationByHandle(handle, FileDispositionInfo, &disposition,
                                       sizeof(disposition));
}

libbsa::result<void> clear_delete_on_close(HANDLE handle) {
    FILE_DISPOSITION_INFO disposition{};
    disposition.DeleteFile = FALSE;
    if (!::SetFileInformationByHandle(handle, FileDispositionInfo, &disposition,
                                      sizeof(disposition))) {
        return windows_io_error("cannot clear temporary extracted file delete-on-close",
                                ::GetLastError());
    }
    return {};
}

/// Resolves the output root's canonical final path once for a whole unpack run.
///
/// Every staged temporary destination is validated against this string, so
/// resolving it up front removes a CreateFileW and a GetFinalPathNameByHandleW
/// from every extracted entry. Caching the root's *real* path does not weaken the
/// check: if the output root is swapped for a junction mid-run, the temp file's own
/// final path stops matching the cached root and staging is rejected, which is the
/// same outcome the per-entry root check produced.
libbsa::result<std::wstring> resolve_output_root_final_path(
    const std::filesystem::path& output_root) {
    unique_windows_handle root_handle{::CreateFileW(
        output_root.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr)};
    if (!root_handle) {
        return windows_io_error("cannot open output directory for final-path validation",
                                ::GetLastError());
    }

    auto root_reparse = handle_is_reparse_point(root_handle.value, "output directory");
    if (!root_reparse) {
        return root_reparse.error();
    }
    if (root_reparse.value()) {
        return make_error(libbsa::error_code::io_error,
                          "refusing reparse-point output directory: " + output_root.string());
    }

    return final_path_for_handle(root_handle.value, "output directory");
}
#endif

std::filesystem::path make_staged_temp_path(const std::filesystem::path& final_path) {
    static std::atomic<std::uint64_t> next_temp_id{0U};
    const auto parent = final_path.parent_path();
#if defined(_WIN32)
    const auto name = final_path.filename().wstring();
    const auto process_id = static_cast<std::uint64_t>(::GetCurrentProcessId());
    const auto id = next_temp_id.fetch_add(1U, std::memory_order_relaxed);
    return parent /
           (L"." + name + L".bsa-tmp-" + std::to_wstring(process_id) + L"-" + std::to_wstring(id));
#else
    const auto name = final_path.filename().string();
    const auto id = next_temp_id.fetch_add(1U, std::memory_order_relaxed);
    return parent / ("." + name + ".bsa-tmp-" + std::to_string(id));
#endif
}

/// Factory-owned state for one CLI extraction target.
///
/// The libbsa extraction API destroys the payload sink before returning the
/// per-entry result, so the temporary file handle must outlive the sink. Keeping
/// this state in the factory lets successful entries publish atomically and lets
/// failed or abandoned entries be discarded without touching the final path.
struct staged_extraction {
#if defined(_WIN32)
    unique_windows_handle handle;
#endif
    std::filesystem::path temp_path;
    std::filesystem::path final_path;
    bool overwrite{false};
    bool finished{false};
};

#if defined(_WIN32)
/// Opens a sibling temporary destination and validates its final handle path.
///
/// The temp is marked delete-on-close immediately so sink-creation failures,
/// extraction failures, and early returns clean up automatically. The open handle
/// is kept in staged_extraction because publish happens after the sink has been
/// destroyed by archive_reader::extract_entries.
///
/// `output_root_final_path` is the canonical output-root path resolved once by
/// `resolve_output_root_final_path`; validating the opened handle's final path
/// against it is what stops a raced parent junction from redirecting extraction
/// outside the output root.
libbsa::result<std::shared_ptr<staged_extraction>> open_staged_destination(
    const std::filesystem::path& temp_path, const std::filesystem::path& final_path,
    const std::wstring& output_root_final_path, bool overwrite) {
    unique_windows_handle temp_handle{::CreateFileW(
        temp_path.c_str(), GENERIC_WRITE | FILE_READ_ATTRIBUTES | DELETE,
        FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_SEQUENTIAL_SCAN, nullptr)};
    if (!temp_handle) {
        return windows_io_error(
            "cannot open temporary destination for writing: " + temp_path.string(),
            ::GetLastError());
    }
    mark_delete_on_close(temp_handle.value);

    auto temp_reparse = handle_is_reparse_point(temp_handle.value, "temporary destination");
    if (!temp_reparse) {
        return temp_reparse.error();
    }
    if (temp_reparse.value()) {
        return make_error(libbsa::error_code::io_error,
                          "refusing reparse-point temporary destination: " + temp_path.string());
    }

    auto temp_final_path = final_path_for_handle(temp_handle.value, "temporary destination");
    if (!temp_final_path) {
        return temp_final_path.error();
    }
    if (!final_path_is_within_root(output_root_final_path, temp_final_path.value())) {
        return make_error(libbsa::error_code::invalid_argument,
                          "refusing temporary destination handle outside output directory: " +
                              temp_path.string());
    }

    auto staged = std::make_shared<staged_extraction>();
    staged->handle = std::move(temp_handle);
    staged->temp_path = temp_path;
    staged->final_path = final_path;
    staged->overwrite = overwrite;
    return staged;
}

/// Publishes a completed temp file to its final path with a handle-based rename.
///
/// Clearing delete-on-close only at commit time preserves automatic cleanup for
/// extraction failures. ReplaceIfExists follows --overwrite; without overwrite,
/// a raced final-path creator makes publish fail instead of clobbering data.
libbsa::result<void> commit_staged(const std::shared_ptr<staged_extraction>& staged) {
    if (!staged || staged->finished) {
        return {};
    }

    auto cleared = clear_delete_on_close(staged->handle.value);
    if (!cleared) {
        return cleared.error();
    }

    const std::wstring final_name = staged->final_path.wstring();
    const auto final_name_bytes = final_name.size() * sizeof(wchar_t);
    const auto rename_info_size =
        offsetof(FILE_RENAME_INFO, FileName) + final_name_bytes + sizeof(wchar_t);
    std::vector<std::byte> rename_storage(rename_info_size);
    auto* rename_info = reinterpret_cast<FILE_RENAME_INFO*>(rename_storage.data());
    rename_info->ReplaceIfExists = staged->overwrite ? TRUE : FALSE;
    rename_info->RootDirectory = nullptr;
    rename_info->FileNameLength = static_cast<DWORD>(final_name_bytes);
    std::memcpy(rename_info->FileName, final_name.data(), final_name_bytes);
    rename_info->FileName[final_name.size()] = L'\0';

    if (!::SetFileInformationByHandle(staged->handle.value, FileRenameInfo, rename_info,
                                      static_cast<DWORD>(rename_storage.size()))) {
        mark_delete_on_close(staged->handle.value);
        return windows_io_error("cannot publish extracted file: " + staged->final_path.string(),
                                ::GetLastError());
    }

    staged->handle.reset();
    staged->finished = true;
    return {};
}

/// Discards an unfinished staged extraction.
///
/// On Windows the handle was marked delete-on-close when opened, so closing it is
/// sufficient and avoids leaving failed extraction temps behind.
void discard_staged(const std::shared_ptr<staged_extraction>& staged) noexcept {
    if (!staged || staged->finished) {
        return;
    }
    staged->handle.reset();
    staged->finished = true;
}

class file_payload_sink final : public libbsa::payload_sink {
   public:
    explicit file_payload_sink(std::shared_ptr<staged_extraction> staged)
        : staged_(std::move(staged)) {}

    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        std::size_t accepted = 0U;
        while (accepted < bytes.size()) {
            const auto remaining = bytes.size() - accepted;
            const auto chunk = static_cast<DWORD>(std::min<std::size_t>(
                remaining, static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())));
            DWORD written = 0U;
            if (!::WriteFile(staged_->handle.value, bytes.data() + accepted, chunk, &written,
                             nullptr)) {
                return windows_io_error("failed to write extracted payload bytes",
                                        ::GetLastError());
            }
            if (written == 0U && chunk != 0U) {
                return make_error(libbsa::error_code::io_error,
                                  "failed to write extracted payload bytes");
            }
            accepted += written;
        }
        return bytes.size();
    }

   private:
    std::shared_ptr<staged_extraction> staged_;
};

#else
/// POSIX fallback: there is no handle-based final-path validation to prepare.
///
/// Returns an empty string so the staging signature stays identical on both
/// platforms; the fallback `open_staged_destination` ignores it.
libbsa::result<std::wstring> resolve_output_root_final_path(const std::filesystem::path&) {
    return std::wstring{};
}

/// Creates POSIX fallback staging metadata.
///
/// The Windows CLI path is the supported safety boundary, but keeping the
/// fallback staged avoids final-path truncation if this file is compiled without
/// Win32 APIs.
libbsa::result<std::shared_ptr<staged_extraction>> open_staged_destination(
    const std::filesystem::path& temp_path, const std::filesystem::path& final_path,
    const std::wstring&, bool overwrite) {
    auto staged = std::make_shared<staged_extraction>();
    staged->temp_path = temp_path;
    staged->final_path = final_path;
    staged->overwrite = overwrite;
    return staged;
}

/// Publishes a completed POSIX fallback temp file to its final path.
libbsa::result<void> commit_staged(const std::shared_ptr<staged_extraction>& staged) {
    if (!staged || staged->finished) {
        return {};
    }
    std::error_code fs_error;
    std::filesystem::rename(staged->temp_path, staged->final_path, fs_error);
    if (fs_error) {
        return make_error(libbsa::error_code::io_error, "cannot publish extracted file '" +
                                                            staged->final_path.string() +
                                                            "': " + fs_error.message());
    }
    staged->finished = true;
    return {};
}

/// Discards an unfinished POSIX fallback staged extraction temp file.
void discard_staged(const std::shared_ptr<staged_extraction>& staged) noexcept {
    if (!staged || staged->finished) {
        return;
    }
    std::error_code ignored;
    std::filesystem::remove(staged->temp_path, ignored);
    staged->finished = true;
}

class file_payload_sink final : public libbsa::payload_sink {
   public:
    explicit file_payload_sink(std::shared_ptr<staged_extraction> staged, std::ofstream out)
        : staged_(std::move(staged)), stream_(std::move(out)) {}

    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        if (!bytes.empty()) {
            stream_.write(reinterpret_cast<const char*>(bytes.data()),
                          static_cast<std::streamsize>(bytes.size()));
        }
        if (!stream_.good()) {
            return make_error(libbsa::error_code::io_error,
                              "failed to write extracted payload bytes");
        }
        return bytes.size();
    }

   private:
    std::shared_ptr<staged_extraction> staged_;
    std::ofstream stream_;
};
#endif

libbsa::result<std::unique_ptr<libbsa::payload_sink>> open_file_payload_sink(
    const std::shared_ptr<staged_extraction>& staged) {
#if defined(_WIN32)
    return std::unique_ptr<libbsa::payload_sink>{new file_payload_sink{staged}};
#else
    std::ofstream temp_stream{staged->temp_path, std::ios::binary | std::ios::trunc};
    if (!temp_stream.is_open()) {
        return make_error(
            libbsa::error_code::io_error,
            "cannot open temporary destination for writing: " + staged->temp_path.string());
    }
    return std::unique_ptr<libbsa::payload_sink>{
        new file_payload_sink{staged, std::move(temp_stream)}};
#endif
}

bool is_within_root(const std::filesystem::path& root, const std::filesystem::path& candidate) {
    const auto normalized_root = root.lexically_normal();
    const auto normalized_candidate = candidate.lexically_normal();

    auto root_it = normalized_root.begin();
    auto candidate_it = normalized_candidate.begin();
    for (; root_it != normalized_root.end(); ++root_it, ++candidate_it) {
        if (candidate_it == normalized_candidate.end() || *root_it != *candidate_it) {
            return false;
        }
    }
    return true;
}

libbsa::result<std::filesystem::path> safe_destination_path(
    const std::filesystem::path& output_root, std::string_view entry_path) {
    std::string normalized_entry{entry_path};
    std::replace(normalized_entry.begin(), normalized_entry.end(), '\\', '/');
    if (normalized_entry.empty()) {
        return make_error(libbsa::error_code::invalid_argument, "empty archive entry path");
    }

    auto decoded_relative = path_from_utf8(normalized_entry);
    if (!decoded_relative) {
        return decoded_relative.error();
    }
    const std::filesystem::path relative = std::move(decoded_relative).value();
    if (relative.is_absolute() || relative.has_root_name() || relative.has_root_directory()) {
        return make_error(libbsa::error_code::invalid_argument,
                          "refusing absolute archive entry path: " + normalized_entry);
    }
    for (const auto& component : relative) {
        if (component == "..") {
            return make_error(
                libbsa::error_code::invalid_argument,
                "refusing parent-directory traversal in archive entry path: " + normalized_entry);
        }
    }

#if defined(_WIN32)
    auto windows_destination_path = reject_windows_unsafe_destination_components(normalized_entry);
    if (!windows_destination_path) {
        return windows_destination_path.error();
    }
#endif

    const auto destination = (output_root / relative).lexically_normal();
    if (!is_within_root(output_root, destination)) {
        return make_error(
            libbsa::error_code::invalid_argument,
            "refusing archive entry path outside output directory: " + normalized_entry);
    }
    return destination;
}

/// Destination decided for every unique unpack request, keyed by the exact request
/// string.
///
/// A request whose destination path or destination directory was rejected holds the
/// diagnostic instead of a path; the sink factory replays it, so a rejected entry
/// still fails on its own instead of aborting the siblings that extract cleanly.
///
/// `archive_reader::extract_entries` coalesces duplicate exact request strings and
/// passes that same string to `bulk_extract_sink_factory::create`, so keying the
/// plan the same way makes the factory's lookup exact.
using extraction_plan =
    std::map<std::string, libbsa::result<std::filesystem::path>, std::less<>>;

/// Verifies containment for one destination directory and creates it.
///
/// Reparse-point rejection runs both before and after creation because
/// `create_directories` materialises components between the two observations; this
/// is the same before/after pairing the per-entry path used, moved to run once per
/// distinct directory.
///
/// Because this now runs once up front rather than again for every entry, a
/// junction swapped into an ancestor *after* the pre-pass is no longer caught here.
/// It is still caught, and still cannot write outside the output root, because
/// `open_staged_destination` compares the opened handle's final path against the
/// output root; only the diagnostic differs.
libbsa::result<void> prepare_destination_directory(const std::filesystem::path& output_root,
                                                   const std::filesystem::path& directory) {
    auto before_creation = reject_reparse_chain(output_root, directory);
    if (!before_creation) {
        return before_creation.error();
    }

    std::error_code fs_error;
    std::filesystem::create_directories(directory, fs_error);
    if (fs_error) {
        return make_error(libbsa::error_code::io_error, "cannot create destination directory '" +
                                                            directory.string() +
                                                            "': " + fs_error.message());
    }

    return reject_reparse_chain(output_root, directory);
}

/// Resolves every unpack destination and creates each distinct directory once,
/// before any extraction worker starts.
///
/// Real archives hold between tens and thousands of entries per directory, so
/// doing containment verification and directory creation per entry repeats almost
/// all of it. This pre-pass converts that work from proportional to entry count
/// into proportional to directory count (issue #53).
///
/// Destination paths come from each archive entry's own spelling rather than from
/// the requested path string — `--path meshes/tiny/probe.nif` still writes
/// `Meshes/Tiny/Probe.nif` — so each request has to be resolved to its entry before
/// its directory is known. Deriving the directory set from the request list rather
/// than from the whole catalog is what keeps a subset extraction from creating
/// directories for entries the caller did not ask for.
///
/// Requests whose lookup fails are deliberately left out of the plan:
/// `extract_entries` repeats the same lookup and records the identical per-entry
/// failure, so the pre-pass must not promote a missing entry into a whole-run
/// failure. A directory that cannot be prepared fails only the requests that
/// resolved into it, which is likewise how the per-entry path behaved.
///
/// The plan is complete before the first worker starts and is only read after that
/// point, so it needs no lock and adds no per-thread state.
extraction_plan plan_extraction(const libbsa::archive_reader& reader,
                                const std::filesystem::path& output_root,
                                std::span<const libbsa::bulk_extract_request> requests) {
    extraction_plan plan;
    std::set<std::filesystem::path> directories;

    for (const auto& request : requests) {
        if (plan.contains(request.path)) {
            continue;
        }

        auto found = reader.find(request.path);
        if (!found || !found.value()) {
            continue;
        }
        const auto& entry = *found.value();

        auto destination = safe_destination_path(output_root, entry.original_path);
        if (!destination) {
            plan.emplace(request.path, destination.error());
            continue;
        }

        auto parent = destination.value().parent_path();
        plan.emplace(request.path, std::move(destination).value());
        if (!parent.empty()) {
            directories.insert(std::move(parent));
        }
    }

    // std::set orders a parent before the directories nested inside it, so an
    // ancestor is created and checked before its children are visited.
    std::map<std::filesystem::path, libbsa::error> failed_directories;
    for (const auto& directory : directories) {
        auto prepared = prepare_destination_directory(output_root, directory);
        if (!prepared) {
            failed_directories.emplace(directory, prepared.error());
        }
    }

    for (auto& [request_path, planned] : plan) {
        if (!planned) {
            continue;
        }
        const auto failed = failed_directories.find(planned.value().parent_path());
        if (failed != failed_directories.end()) {
            planned = failed->second;
        }
    }

    return plan;
}

/// Bulk extraction factory that stages entries into destinations decided by the
/// serial pre-pass.
///
/// The factory performs no destination-directory containment or creation work: by
/// the time `create` runs, `plan_extraction` has already verified containment and
/// created every directory. `plan_` is immutable for the whole extraction, so
/// worker threads read it without synchronization.
class file_sink_factory final : public libbsa::bulk_extract_sink_factory {
   public:
    file_sink_factory(extraction_plan plan, std::wstring output_root_final_path, bool overwrite)
        : plan_(std::move(plan)),
          output_root_final_path_(std::move(output_root_final_path)),
          overwrite_(overwrite) {}

    ~file_sink_factory() override {
        std::vector<std::shared_ptr<staged_extraction>> staged;
        {
            std::lock_guard lock{mutex_};
            staged = staged_;
        }
        for (const auto& entry : staged) {
            discard_staged(entry);
        }
    }

    libbsa::result<std::unique_ptr<libbsa::payload_sink>> create(
        std::string_view path, const libbsa::entry_metadata& entry) override {
        // The destination was derived from this entry's archive spelling during the
        // pre-pass, so the entry metadata is not consulted again here.
        (void)entry;

        const auto planned = plan_.find(path);
        if (planned == plan_.end()) {
            return make_error(libbsa::error_code::io_error,
                              "no planned extraction destination for archive path: " +
                                  std::string{path});
        }
        if (!planned->second) {
            return planned->second.error();
        }
        const auto& destination = planned->second.value();

        auto destination_reparse = reject_reparse_point(destination, "destination");
        if (!destination_reparse) {
            return destination_reparse.error();
        }

        std::error_code fs_error;
        const bool exists = std::filesystem::exists(destination, fs_error);
        if (fs_error) {
            return make_error(libbsa::error_code::io_error, "cannot inspect destination '" +
                                                                destination.string() +
                                                                "': " + fs_error.message());
        }
        if (exists && !overwrite_) {
            return make_error(libbsa::error_code::io_error,
                              "destination exists and --overwrite was not specified: " +
                                  destination.string());
        }
        if (exists && std::filesystem::is_directory(destination, fs_error)) {
            return make_error(libbsa::error_code::io_error,
                              "destination is a directory: " + destination.string());
        }
        if (fs_error) {
            return make_error(libbsa::error_code::io_error, "cannot inspect destination type '" +
                                                                destination.string() +
                                                                "': " + fs_error.message());
        }

        libbsa::result<std::shared_ptr<staged_extraction>> staged{make_error(
            libbsa::error_code::io_error, "cannot prepare staged extraction destination")};
        for (int attempt = 0; attempt != 16; ++attempt) {
            const auto temp_path = make_staged_temp_path(destination);
            if (std::filesystem::exists(temp_path, fs_error)) {
                if (fs_error) {
                    return make_error(libbsa::error_code::io_error,
                                      "cannot inspect temporary destination '" +
                                          temp_path.string() + "': " + fs_error.message());
                }
                continue;
            }
            if (fs_error) {
                return make_error(libbsa::error_code::io_error,
                                  "cannot inspect temporary destination '" + temp_path.string() +
                                      "': " + fs_error.message());
            }

            staged = open_staged_destination(temp_path, destination, output_root_final_path_,
                                             overwrite_);
            if (staged) {
                break;
            }
        }
        if (!staged) {
            return staged.error();
        }

        {
            std::lock_guard lock{mutex_};
            staged_.push_back(staged.value());
            by_path_.emplace(std::string{path}, staged.value());
        }

        auto sink = open_file_payload_sink(staged.value());
        if (!sink) {
            untrack_staged(path, staged.value());
            discard_staged(staged.value());
            return sink.error();
        }
        return sink;
    }

    /// Publishes on success and discards on failure as soon as each entry
    /// finishes, so at most worker_count staged destinations stay open at once.
    ///
    /// take_staged() takes the factory mutex only for the map update; the
    /// publish/discard syscalls run without the lock, so independent worker
    /// threads never block one another while committing their own entries.
    libbsa::result<void> finish(std::string_view path, bool succeeded) override {
        auto staged = take_staged(path);
        if (!staged) {
            return {};
        }
        if (succeeded) {
            auto committed = commit_staged(staged);
            if (!committed) {
                discard_staged(staged);
                return committed.error();
            }
            return {};
        }
        discard_staged(staged);
        return {};
    }

   private:
    void untrack_staged(std::string_view path, const std::shared_ptr<staged_extraction>& staged) {
        std::lock_guard lock{mutex_};
        by_path_.erase(std::string{path});
        std::erase(staged_, staged);
    }

    std::shared_ptr<staged_extraction> take_staged(std::string_view path) {
        std::lock_guard lock{mutex_};
        const auto found = by_path_.find(std::string{path});
        if (found == by_path_.end()) {
            return {};
        }
        auto staged = std::move(found->second);
        by_path_.erase(found);
        std::erase(staged_, staged);
        return staged;
    }

    /// Immutable for the lifetime of the extraction, so worker threads read it
    /// without taking mutex_.
    const extraction_plan plan_;
    const std::wstring output_root_final_path_;
    const bool overwrite_;
    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<staged_extraction>> staged_;
    std::map<std::string, std::shared_ptr<staged_extraction>, std::less<>> by_path_;
};

libbsa::result<std::filesystem::path> prepare_output_root(const std::filesystem::path& output_dir) {
    std::error_code fs_error;
    const auto root = std::filesystem::absolute(output_dir, fs_error).lexically_normal();
    if (fs_error) {
        return make_error(
            libbsa::error_code::io_error,
            "cannot resolve output directory '" + output_dir.string() + "': " + fs_error.message());
    }

    if (std::filesystem::exists(root, fs_error)) {
        if (fs_error) {
            return make_error(
                libbsa::error_code::io_error,
                "cannot inspect output directory '" + root.string() + "': " + fs_error.message());
        }
        if (!std::filesystem::is_directory(root, fs_error)) {
            return make_error(libbsa::error_code::io_error,
                              "output path is not a directory: " + root.string());
        }
        auto root_reparse = reject_reparse_point(root, "output directory");
        if (!root_reparse) {
            return root_reparse.error();
        }
    } else {
        std::filesystem::create_directories(root, fs_error);
        if (fs_error) {
            return make_error(
                libbsa::error_code::io_error,
                "cannot create output directory '" + root.string() + "': " + fs_error.message());
        }
    }
    return root;
}

/// Runs `bsa unpack` using parsed subcommand values and guarded destination staging.
/// Selection, traversal checks, and operational diagnostics remain owned by libbsa paths.
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
    const std::uint32_t worker_count = resolved_worker_count.value();

    auto opened = libbsa::archive_reader::open(positionals[0]);
    if (!opened) {
        render_error(opened.error(), positionals[0]);
        return static_cast<int>(process_exit::operational_failure);
    }

    auto output_dir = path_from_utf8(positionals[1]);
    if (!output_dir) {
        render_error(output_dir.error(), positionals[1]);
        return static_cast<int>(process_exit::operational_failure);
    }

    auto output_root = prepare_output_root(output_dir.value());
    if (!output_root) {
        render_error(output_root.error());
        return static_cast<int>(process_exit::operational_failure);
    }

    std::vector<libbsa::bulk_extract_request> requests;
    const auto selected_paths = parser.get<std::vector<std::string>>("--path");
    if (!selected_paths.empty()) {
        requests.reserve(selected_paths.size());
        for (const auto& path : selected_paths) {
            requests.push_back(libbsa::bulk_extract_request{path});
        }
    } else {
        auto entries = opened.value().entries();
        if (!entries) {
            render_error(entries.error(), positionals[0]);
            return static_cast<int>(process_exit::operational_failure);
        }
        requests.reserve(entries.value().size());
        for (const auto& entry : entries.value()) {
            // Preserve archive spelling for destination paths; extract_entries still
            // normalizes the request for lookup.
            requests.push_back(libbsa::bulk_extract_request{entry.original_path});
        }
    }

    // Resolve the output root's canonical path once; every staged destination is
    // validated against it instead of reopening the root per entry.
    auto output_root_final_path = resolve_output_root_final_path(output_root.value());
    if (!output_root_final_path) {
        render_error(output_root_final_path.error());
        return static_cast<int>(process_exit::operational_failure);
    }

    // Serial pre-pass: containment is verified and every distinct destination
    // directory is created here, before any worker thread starts, so an entry path
    // that cannot be written safely is rejected before any file is created.
    auto plan = plan_extraction(opened.value(), output_root.value(), requests);

    file_sink_factory sink_factory{std::move(plan), std::move(output_root_final_path).value(),
                                   parser.get<bool>("--overwrite")};
    auto extracted = opened.value().extract_entries(requests, sink_factory,
                                                    libbsa::bulk_extract_options{worker_count});
    if (!extracted) {
        render_error(extracted.error(), positionals[0]);
        return static_cast<int>(process_exit::operational_failure);
    }

    // Destinations are already published or discarded incrementally by
    // file_sink_factory::finish during extraction; surface any per-entry failure
    // (including a publish failure promoted by finish) here.
    std::size_t failure_count = 0;
    for (const auto& result : extracted.value()) {
        if (result.failure.has_value()) {
            ++failure_count;
            render_error(*result.failure, result.path);
        }
    }

    std::cout << "extracted " << (extracted.value().size() - failure_count) << " of "
              << extracted.value().size() << " requested entr"
              << (extracted.value().size() == 1U ? "y" : "ies") << '\n';
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
