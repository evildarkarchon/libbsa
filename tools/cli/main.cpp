#include <libbsa/libbsa.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
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

namespace
{
  enum class process_exit : int
  {
    success = 0,
    operational_failure = 1,
    usage_error = 2,
  };

  enum class option_kind
  {
    flag,
    value,
  };

  struct option_spec
  {
    std::string_view name;
    option_kind kind;
  };

  struct parsed_option
  {
    std::string name;
    std::optional<std::string> value;
  };

  struct parsed_arguments
  {
    bool help{false};
    bool version{false};
    std::vector<parsed_option> options;
    std::vector<std::string> positionals;
  };

  enum class compression_choice
  {
    target_default,
    raw,
    compressed,
  };

  enum class writer_family
  {
    tes3_bsa,
    tes4_bsa,
    ba2_gnrl,
    ba2_dx10,
  };

  struct format_descriptor
  {
    std::string_view token;
    writer_family family;
    std::string_view description;
    libbsa::tes4_bsa_target tes4_target{};
    libbsa::ba2_gnrl_target ba2_gnrl_target{};
    libbsa::ba2_dx10_target ba2_dx10_target{};
    bool supports_raw{true};
    bool supports_compressed{true};
  };

  struct input_file
  {
    std::filesystem::path host_path;
    std::string archive_path;
  };

  constexpr std::array format_table{
      format_descriptor{"bsa-tes3",
                        writer_family::tes3_bsa,
                        "TES3/Morrowind BSA (raw)",
                        {},
                        {},
                        {},
                        true,
                        false},
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
      format_descriptor{"ba2-dx10-sf-v3",
                        writer_family::ba2_dx10,
                        "BA2 DX10 texture archive for Starfield v3",
                        {},
                        {},
                        libbsa::ba2_dx10_target::starfield_v3,
                        false,
                        true},
  };

  libbsa::error make_error(libbsa::error_code code, std::string message)
  {
    return libbsa::error{code, std::move(message)};
  }

  libbsa::error make_usage_error(std::string message)
  {
    return make_error(libbsa::error_code::invalid_argument, std::move(message));
  }

  std::string error_code_name(libbsa::error_code code)
  {
    switch (code)
    {
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

  std::string archive_type_name(libbsa::archive_type type)
  {
    switch (type)
    {
    case libbsa::archive_type::bsa:
      return "bsa";
    case libbsa::archive_type::ba2:
      return "ba2";
    }

    return "unknown";
  }

  std::string archive_variant_name(libbsa::archive_variant variant)
  {
    switch (variant)
    {
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

  std::string compression_name(libbsa::entry_compression compression)
  {
    switch (compression)
    {
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

  std::string warning_code_name(libbsa::compatibility_warning_code code)
  {
    switch (code)
    {
    case libbsa::compatibility_warning_code::compressed_sound_payload:
      return "compressed_sound_payload";
    case libbsa::compatibility_warning_code::bsa_embedded_name_compatibility_risk:
      return "bsa_embedded_name_compatibility_risk";
    case libbsa::compatibility_warning_code::target_family_mismatch:
      return "target_family_mismatch";
    }

    return "unknown";
  }

  std::string warning_severity_name(libbsa::compatibility_warning_severity severity)
  {
    switch (severity)
    {
    case libbsa::compatibility_warning_severity::advisory:
      return "advisory";
    case libbsa::compatibility_warning_severity::risky:
      return "risky";
    }

    return "unknown";
  }

  void render_error(const libbsa::error &err, std::string_view context = {})
  {
    std::cerr << "error";
    if (!context.empty())
    {
      std::cerr << " (" << context << ")";
    }
    std::cerr << ": " << error_code_name(err.code) << ": " << err.message << '\n';
  }

  int usage_failure(const libbsa::error &err, void (*usage)(std::ostream &))
  {
    std::cerr << "usage error: " << err.message << "\n\n";
    usage(std::cerr);
    return static_cast<int>(process_exit::usage_error);
  }

  const option_spec *find_option_spec(std::span<const option_spec> specs, std::string_view name)
  {
    const auto found = std::find_if(specs.begin(), specs.end(), [name](const option_spec &spec)
                                    { return spec.name == name; });
    return found == specs.end() ? nullptr : &*found;
  }

  libbsa::result<parsed_arguments> parse_options(std::span<const std::string_view> args,
                                                 std::span<const option_spec> specs)
  {
    parsed_arguments parsed;
    bool end_of_options = false;

    for (std::size_t index = 0; index < args.size(); ++index)
    {
      const auto token = args[index];
      if (end_of_options)
      {
        parsed.positionals.emplace_back(token);
        continue;
      }

      if (token == "--")
      {
        end_of_options = true;
        continue;
      }
      if (token == "-h" || token == "--help")
      {
        parsed.help = true;
        continue;
      }
      if (token == "-V" || token == "--version")
      {
        parsed.version = true;
        continue;
      }

      if (token.rfind("--", 0U) == 0U)
      {
        const auto body = token.substr(2U);
        const auto equals = body.find('=');
        const auto name = equals == std::string_view::npos ? body : body.substr(0U, equals);
        if (name.empty())
        {
          return make_usage_error("empty option name");
        }

        const option_spec *spec = find_option_spec(specs, name);
        if (spec == nullptr)
        {
          return make_usage_error("unknown option --" + std::string{name});
        }

        if (spec->kind == option_kind::flag)
        {
          if (equals != std::string_view::npos)
          {
            return make_usage_error("option --" + std::string{name} + " does not take a value");
          }
          parsed.options.push_back(parsed_option{std::string{name}, std::nullopt});
          continue;
        }

        std::string value;
        if (equals != std::string_view::npos)
        {
          value = std::string{body.substr(equals + 1U)};
        }
        else
        {
          if (index + 1U >= args.size() || args[index + 1U].rfind("-", 0U) == 0U)
          {
            return make_usage_error("missing value for --" + std::string{name});
          }
          ++index;
          value = std::string{args[index]};
        }

        if (value.empty())
        {
          return make_usage_error("missing value for --" + std::string{name});
        }

        parsed.options.push_back(parsed_option{std::string{name}, std::move(value)});
        continue;
      }

      if (token.rfind("-", 0U) == 0U)
      {
        return make_usage_error("unknown option " + std::string{token});
      }

      parsed.positionals.emplace_back(token);
    }

    return parsed;
  }

  bool has_flag(const parsed_arguments &args, std::string_view name)
  {
    return std::any_of(args.options.begin(), args.options.end(), [name](const parsed_option &option)
                       { return option.name == name; });
  }

  std::vector<std::string> values_for(const parsed_arguments &args, std::string_view name)
  {
    std::vector<std::string> values;
    for (const auto &option : args.options)
    {
      if (option.name == name && option.value.has_value())
      {
        values.push_back(*option.value);
      }
    }
    return values;
  }

  libbsa::result<std::optional<std::string>> single_value_for(const parsed_arguments &args, std::string_view name)
  {
    auto values = values_for(args, name);
    if (values.size() > 1U)
    {
      return make_usage_error("option --" + std::string{name} + " may be specified only once");
    }
    if (values.empty())
    {
      return std::optional<std::string>{};
    }
    return std::optional<std::string>{std::move(values.front())};
  }

  const format_descriptor *find_format(std::string_view token)
  {
    const auto found = std::find_if(format_table.begin(), format_table.end(), [token](const format_descriptor &descriptor)
                                    { return descriptor.token == token; });
    return found == format_table.end() ? nullptr : &*found;
  }

  std::string valid_format_tokens()
  {
    std::ostringstream output;
    for (std::size_t index = 0; index < format_table.size(); ++index)
    {
      if (index != 0U)
      {
        output << ", ";
      }
      output << format_table[index].token;
    }
    return output.str();
  }

  std::string path_to_utf8(const std::filesystem::path &path)
  {
    const auto text = path.u8string();
    return {reinterpret_cast<const char *>(text.data()), text.size()};
  }

  std::string generic_utf8_path(const std::filesystem::path &path)
  {
    const auto text = path.generic_u8string();
    return {reinterpret_cast<const char *>(text.data()), text.size()};
  }

  libbsa::result<std::filesystem::path> path_from_utf8(std::string_view utf8_path)
  {
    if (utf8_path.find('\0') != std::string_view::npos)
    {
      return make_error(libbsa::error_code::invalid_argument, "path contains embedded NUL bytes");
    }
    if (utf8_path.empty())
    {
      return std::filesystem::path{};
    }

#if defined(_WIN32)
    if (utf8_path.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
    {
      return make_error(libbsa::error_code::invalid_argument, "path is too long to decode as UTF-8");
    }

    // MSVC decodes narrow filesystem paths through the active ANSI code page, but archive paths are UTF-8.
    const auto source_size = static_cast<int>(utf8_path.size());
    const auto wide_size = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_path.data(), source_size,
                                                 nullptr, 0);
    if (wide_size <= 0)
    {
      return make_error(libbsa::error_code::invalid_argument, "path is not valid UTF-8");
    }

    std::wstring wide_path(static_cast<std::size_t>(wide_size), L'\0');
    const auto converted = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_path.data(), source_size,
                                                 wide_path.data(), wide_size);
    if (converted != wide_size)
    {
      return make_error(libbsa::error_code::invalid_argument, "path is not valid UTF-8");
    }

    return std::filesystem::path{std::move(wide_path)};
#else
    std::u8string path;
    path.reserve(utf8_path.size());
    for (const unsigned char ch : utf8_path)
    {
      path.push_back(static_cast<char8_t>(ch));
    }
    return std::filesystem::path{std::move(path)};
#endif
  }

#if defined(_WIN32)
  char lower_ascii(char value) noexcept
  {
    if (value >= 'A' && value <= 'Z')
    {
      return static_cast<char>(value - 'A' + 'a');
    }
    return value;
  }

  bool ascii_iequals(std::string_view lhs, std::string_view rhs) noexcept
  {
    if (lhs.size() != rhs.size())
    {
      return false;
    }
    for (std::size_t index = 0; index < lhs.size(); ++index)
    {
      if (lower_ascii(lhs[index]) != rhs[index])
      {
        return false;
      }
    }
    return true;
  }

  std::string_view windows_reserved_device_stem(std::string_view component) noexcept
  {
    auto stem = component.substr(0U, component.find('.'));
    while (!stem.empty() && (stem.back() == ' ' || stem.back() == '.'))
    {
      stem.remove_suffix(1U);
    }
    return stem;
  }

  bool is_windows_reserved_device_name(std::string_view component) noexcept
  {
    const auto stem = windows_reserved_device_stem(component);
    if (ascii_iequals(stem, "con") || ascii_iequals(stem, "prn") || ascii_iequals(stem, "aux") ||
        ascii_iequals(stem, "nul") || ascii_iequals(stem, "conin$") || ascii_iequals(stem, "conout$"))
    {
      return true;
    }
    if (stem.size() >= 4U)
    {
      const auto prefix = stem.substr(0U, 3U);
      if (!ascii_iequals(prefix, "com") && !ascii_iequals(prefix, "lpt"))
      {
        return false;
      }

      const auto suffix = stem.substr(3U);
      // Win32 also treats superscript 1/2/3 as reserved COM/LPT suffixes.
      return (suffix.size() == 1U && suffix.front() >= '1' && suffix.front() <= '9') ||
             suffix == std::string_view{"\xC2\xB9", 2U} || suffix == std::string_view{"\xC2\xB2", 2U} ||
             suffix == std::string_view{"\xC2\xB3", 2U};
    }
    return false;
  }

  libbsa::result<void> reject_windows_unsafe_destination_components(std::string_view normalized_entry)
  {
    for (std::size_t start = 0U; start <= normalized_entry.size();)
    {
      const auto slash = normalized_entry.find('/', start);
      const auto end = slash == std::string_view::npos ? normalized_entry.size() : slash;
      const auto component = normalized_entry.substr(start, end - start);

      if (component.find(':') != std::string_view::npos)
      {
        return make_error(libbsa::error_code::invalid_argument,
                          "refusing colon in archive entry path component: " + std::string{normalized_entry});
      }
      if (!component.empty() && (component.back() == '.' || component.back() == ' '))
      {
        return make_error(libbsa::error_code::invalid_argument,
                          "refusing trailing dot or space in archive entry path component '" +
                              std::string{component} + "': " + std::string{normalized_entry});
      }
      // Win32 resolves DOS device names as special files even when an extension is present, such as NUL.txt.
      if (is_windows_reserved_device_name(component))
      {
        return make_error(libbsa::error_code::invalid_argument,
                          "refusing Windows-reserved archive entry path component '" + std::string{component} +
                              "': " + std::string{normalized_entry});
      }

      if (slash == std::string_view::npos)
      {
        break;
      }
      start = slash + 1U;
    }
    return {};
  }
#endif

#if defined(_WIN32)
  struct local_command_line_argv
  {
    wchar_t **value{};

    explicit local_command_line_argv(wchar_t **argv) noexcept : value(argv) {}
    local_command_line_argv(const local_command_line_argv &) = delete;
    local_command_line_argv &operator=(const local_command_line_argv &) = delete;
    ~local_command_line_argv()
    {
      if (value != nullptr)
      {
        ::LocalFree(value);
      }
    }
  };

  libbsa::result<std::string> wide_argument_to_utf8(std::wstring_view argument)
  {
    if (argument.empty())
    {
      return std::string{};
    }
    if (argument.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
    {
      return make_error(libbsa::error_code::invalid_argument,
                        "Windows command-line argument is too long to encode as UTF-8");
    }

    // Windows exposes the authoritative process command line as UTF-16; encode it once to honor
    // the CLI/library UTF-8 host-path contract instead of using CRT ANSI-code-page argv bytes.
    const auto source_size = static_cast<int>(argument.size());
    const auto utf8_size = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argument.data(), source_size,
                                                 nullptr, 0, nullptr, nullptr);
    if (utf8_size <= 0)
    {
      return make_error(libbsa::error_code::invalid_argument,
                        "Windows command-line argument is not valid Unicode");
    }

    std::string utf8_argument(static_cast<std::size_t>(utf8_size), '\0');
    const auto converted = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argument.data(), source_size,
                                                 utf8_argument.data(), utf8_size, nullptr, nullptr);
    if (converted != utf8_size)
    {
      return make_error(libbsa::error_code::invalid_argument,
                        "Windows command-line argument is not valid Unicode");
    }
    return utf8_argument;
  }
#endif

  libbsa::result<std::vector<std::string>> command_line_arguments(int argc, char **argv)
  {
    std::vector<std::string> arguments;

#if defined(_WIN32)
    (void)argc;
    (void)argv;

    int wide_argc = 0;
    local_command_line_argv wide_argv{::CommandLineToArgvW(::GetCommandLineW(), &wide_argc)};
    if (wide_argv.value == nullptr)
    {
      return make_error(libbsa::error_code::invalid_argument, "cannot parse Windows command line");
    }

    arguments.reserve(wide_argc > 1 ? static_cast<std::size_t>(wide_argc - 1) : 0U);
    for (int index = 1; index < wide_argc; ++index)
    {
      auto utf8_argument = wide_argument_to_utf8(wide_argv.value[index]);
      if (!utf8_argument)
      {
        return utf8_argument.error();
      }
      arguments.push_back(std::move(utf8_argument).value());
    }
#else
    arguments.reserve(argc > 1 ? static_cast<std::size_t>(argc - 1) : 0U);
    for (int index = 1; index < argc; ++index)
    {
      arguments.emplace_back(argv[index]);
    }
#endif

    return arguments;
  }

  void print_format_table(std::ostream &output)
  {
    output << "Formats:\n";
    for (const auto &format : format_table)
    {
      output << "  " << std::left << std::setw(18) << format.token << format.description << '\n';
    }
  }

  void print_top_level_usage(std::ostream &output)
  {
    output << "Usage: bsa <command> [options]\n\n"
           << "Commands:\n"
           << "  pack      Create a new archive from a directory\n"
           << "  unpack    Extract archive entries to a directory\n"
           << "  list      List archive entries\n"
           << "  info      Print archive metadata\n"
           << "  validate  Validate an archive\n\n"
           << "Global options:\n"
           << "  -h, --help     Show help\n"
           << "  -V, --version  Show version\n";
  }

  void print_pack_usage(std::ostream &output)
  {
    output << "Usage: bsa pack --format <token> [--compress <default|raw|compressed>] [--overwrite] <input-dir> <output-archive>\n\n"
           << "Options:\n"
           << "  --format <token>      Required archive writer/target token\n"
           << "  --compress <value>    Compression policy: default, raw, compressed\n"
           << "  --overwrite           Replace an existing output archive\n"
           << "  -h, --help            Show help\n\n";
    print_format_table(output);
  }

  void print_unpack_usage(std::ostream &output)
  {
    output << "Usage: bsa unpack [--path <archive-path>]... [--overwrite] <archive> <output-dir>\n\n"
           << "Options:\n"
           << "  --path <archive-path>  Extract only the requested archive path; may repeat\n"
           << "  --overwrite            Replace existing destination files\n"
           << "  -h, --help             Show help\n";
  }

  void print_list_usage(std::ostream &output)
  {
    output << "Usage: bsa list [--details] <archive>\n\n"
           << "Options:\n"
           << "  --details, --detail  Include raw size, stored size, and compression\n"
           << "  -h, --help           Show help\n";
  }

  void print_info_usage(std::ostream &output)
  {
    output << "Usage: bsa info <archive>\n\n"
           << "Options:\n"
           << "  -h, --help  Show help\n";
  }

  void print_validate_usage(std::ostream &output)
  {
    output << "Usage: bsa validate [--strict] <archive>\n\n"
           << "Options:\n"
           << "  --strict    Treat compatibility warnings as failures\n"
           << "  -h, --help  Show help\n";
  }

  void print_version(std::ostream &output)
  {
    output << "bsa (libbsa " << libbsa::version_major << '.' << libbsa::version_minor << '.'
           << libbsa::version_patch << ")\n";
  }

  libbsa::result<compression_choice> parse_compression(std::string_view value)
  {
    if (value == "default")
    {
      return compression_choice::target_default;
    }
    if (value == "raw")
    {
      return compression_choice::raw;
    }
    if (value == "compressed")
    {
      return compression_choice::compressed;
    }
    return make_usage_error("unknown --compress value '" + std::string{value} + "'");
  }

  libbsa::archive_compression_policy archive_policy_from(compression_choice choice)
  {
    switch (choice)
    {
    case compression_choice::target_default:
      return libbsa::archive_compression_policy::target_default;
    case compression_choice::raw:
      return libbsa::archive_compression_policy::all_raw;
    case compression_choice::compressed:
      return libbsa::archive_compression_policy::all_compressed;
    }

    return libbsa::archive_compression_policy::target_default;
  }

  bool is_within_root(const std::filesystem::path &root, const std::filesystem::path &candidate);

  libbsa::result<bool> is_reparse_point(const std::filesystem::path &path, std::string_view context)
  {
#if defined(_WIN32)
    const auto attributes = ::GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
      const auto last_error = ::GetLastError();
      if (last_error == ERROR_FILE_NOT_FOUND || last_error == ERROR_PATH_NOT_FOUND)
      {
        return false;
      }
      return make_error(libbsa::error_code::io_error,
                        "cannot inspect " + std::string{context} + " for reparse points: " + path.string());
    }
    return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
    (void)path;
    (void)context;
    return false;
#endif
  }

  libbsa::result<void> reject_reparse_point(const std::filesystem::path &path, std::string_view context)
  {
    auto reparse = is_reparse_point(path, context);
    if (!reparse)
    {
      return reparse.error();
    }
    if (reparse.value())
    {
      return make_error(libbsa::error_code::io_error,
                        "refusing reparse-point " + std::string{context} + ": " + path.string());
    }
    return {};
  }

  libbsa::result<void> reject_reparse_ancestors(const std::filesystem::path &root,
                                                const std::filesystem::path &destination)
  {
    std::vector<std::filesystem::path> ancestors;
    for (auto current = destination.parent_path(); !current.empty(); current = current.parent_path())
    {
      if (!is_within_root(root, current))
      {
        break;
      }
      ancestors.push_back(current);
      if (current.lexically_normal() == root.lexically_normal())
      {
        break;
      }
    }

    for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it)
    {
      auto checked = reject_reparse_point(*it, "destination parent");
      if (!checked)
      {
        return checked.error();
      }
    }
    return {};
  }

  libbsa::result<std::vector<input_file>> collect_input_files(const std::filesystem::path &input_dir)
  {
    std::error_code fs_error;
    const auto root = std::filesystem::absolute(input_dir, fs_error).lexically_normal();
    if (fs_error)
    {
      return make_error(libbsa::error_code::io_error,
                        "cannot resolve input directory '" + input_dir.string() + "': " + fs_error.message());
    }
    if (!std::filesystem::exists(root, fs_error))
    {
      return make_error(libbsa::error_code::io_error, "input directory does not exist: " + root.string());
    }
    if (fs_error || !std::filesystem::is_directory(root, fs_error))
    {
      return make_error(libbsa::error_code::io_error, "input path is not a directory: " + root.string());
    }
    auto root_reparse = reject_reparse_point(root, "input directory");
    if (!root_reparse)
    {
      return root_reparse.error();
    }

    std::vector<input_file> files;
    std::filesystem::recursive_directory_iterator iterator{root,
                                                           std::filesystem::directory_options::none,
                                                           fs_error};
    if (fs_error)
    {
      return make_error(libbsa::error_code::io_error,
                        "cannot enumerate input directory '" + root.string() + "': " + fs_error.message());
    }

    const std::filesystem::recursive_directory_iterator end;
    for (; iterator != end;)
    {
      const auto &entry = *iterator;
      auto input_reparse = reject_reparse_point(entry.path(), "input path");
      if (!input_reparse)
      {
        return input_reparse.error();
      }

      const bool regular = entry.is_regular_file(fs_error);
      if (fs_error)
      {
        return make_error(libbsa::error_code::io_error,
                          "cannot inspect input path '" + entry.path().string() + "': " + fs_error.message());
      }
      if (regular)
      {
        const auto relative = entry.path().lexically_normal().lexically_relative(root);
        const auto archive_path = generic_utf8_path(relative);
        if (archive_path.empty() || archive_path == ".")
        {
          return make_error(libbsa::error_code::io_error,
                            "cannot derive archive path for input file: " + entry.path().string());
        }
        files.push_back(input_file{entry.path(), archive_path});
      }

      iterator.increment(fs_error);
      if (fs_error)
      {
        return make_error(libbsa::error_code::io_error, "directory enumeration failed: " + fs_error.message());
      }
    }

    std::sort(files.begin(), files.end(), [](const input_file &lhs, const input_file &rhs)
              { return lhs.archive_path < rhs.archive_path; });
    return files;
  }

  int pack_tes3(const std::vector<input_file> &files,
                const std::filesystem::path &output_path,
                bool overwrite)
  {
    libbsa::tes3_bsa_writer_options options;
    options.overwrite_existing = overwrite;
    libbsa::tes3_bsa_writer writer{options};

    for (const auto &file : files)
    {
      auto added = writer.add_file(file.archive_path, path_to_utf8(file.host_path));
      if (!added)
      {
        render_error(added.error(), file.archive_path);
        return static_cast<int>(process_exit::operational_failure);
      }
    }

    auto written = writer.write_to(path_to_utf8(output_path), libbsa::write_execution_options{});
    if (!written)
    {
      render_error(written.error(), path_to_utf8(output_path));
      return static_cast<int>(process_exit::operational_failure);
    }
    return static_cast<int>(process_exit::success);
  }

  int pack_tes4(const format_descriptor &format,
                const std::vector<input_file> &files,
                const std::filesystem::path &output_path,
                bool overwrite,
                compression_choice compression)
  {
    libbsa::tes4_bsa_writer_options options;
    options.overwrite_existing = overwrite;
    options.compression_policy = archive_policy_from(compression);
    libbsa::tes4_bsa_writer writer{format.tes4_target, options};

    for (const auto &file : files)
    {
      auto added = writer.add_file(file.archive_path, path_to_utf8(file.host_path));
      if (!added)
      {
        render_error(added.error(), file.archive_path);
        return static_cast<int>(process_exit::operational_failure);
      }
    }

    auto written = writer.write_to(path_to_utf8(output_path), libbsa::write_execution_options{});
    if (!written)
    {
      render_error(written.error(), path_to_utf8(output_path));
      return static_cast<int>(process_exit::operational_failure);
    }
    return static_cast<int>(process_exit::success);
  }

  int pack_ba2_gnrl(const format_descriptor &format,
                    const std::vector<input_file> &files,
                    const std::filesystem::path &output_path,
                    bool overwrite,
                    compression_choice compression)
  {
    libbsa::ba2_gnrl_writer_options options;
    options.overwrite_existing = overwrite;
    options.compression = archive_policy_from(compression);
    libbsa::ba2_gnrl_writer writer{format.ba2_gnrl_target, options};

    for (const auto &file : files)
    {
      auto added = writer.add_file(file.archive_path, path_to_utf8(file.host_path));
      if (!added)
      {
        render_error(added.error(), file.archive_path);
        return static_cast<int>(process_exit::operational_failure);
      }
    }

    auto written = writer.write_to(path_to_utf8(output_path), libbsa::write_execution_options{});
    if (!written)
    {
      render_error(written.error(), path_to_utf8(output_path));
      return static_cast<int>(process_exit::operational_failure);
    }
    return static_cast<int>(process_exit::success);
  }

  int pack_ba2_dx10(const format_descriptor &format,
                    const std::vector<input_file> &files,
                    const std::filesystem::path &output_path,
                    bool overwrite)
  {
    libbsa::ba2_dx10_writer_options options;
    options.overwrite_existing = overwrite;
    libbsa::ba2_dx10_writer writer{format.ba2_dx10_target, options};

    for (const auto &file : files)
    {
      auto added = writer.add_file(file.archive_path, path_to_utf8(file.host_path));
      if (!added)
      {
        render_error(added.error(), file.archive_path);
        return static_cast<int>(process_exit::operational_failure);
      }
    }

    auto written = writer.write_to(path_to_utf8(output_path), libbsa::write_execution_options{});
    if (!written)
    {
      render_error(written.error(), path_to_utf8(output_path));
      return static_cast<int>(process_exit::operational_failure);
    }
    return static_cast<int>(process_exit::success);
  }

  int run_pack(std::span<const std::string_view> args)
  {
    constexpr std::array specs{
        option_spec{"format", option_kind::value},
        option_spec{"compress", option_kind::value},
        option_spec{"overwrite", option_kind::flag},
    };

    auto parsed = parse_options(args, specs);
    if (!parsed)
    {
      return usage_failure(parsed.error(), print_pack_usage);
    }
    if (parsed.value().help)
    {
      print_pack_usage(std::cout);
      return static_cast<int>(process_exit::success);
    }
    if (parsed.value().version)
    {
      print_version(std::cout);
      return static_cast<int>(process_exit::success);
    }
    if (parsed.value().positionals.size() != 2U)
    {
      return usage_failure(make_usage_error("pack requires <input-dir> and <output-archive>"), print_pack_usage);
    }

    auto format_value = single_value_for(parsed.value(), "format");
    if (!format_value)
    {
      return usage_failure(format_value.error(), print_pack_usage);
    }
    if (!format_value.value().has_value())
    {
      return usage_failure(make_usage_error("pack requires --format"), print_pack_usage);
    }
    const format_descriptor *format = find_format(*format_value.value());
    if (format == nullptr)
    {
      return usage_failure(make_usage_error("unknown --format token '" + *format_value.value() + "'; valid tokens: " +
                                            valid_format_tokens()),
                           print_pack_usage);
    }

    compression_choice compression = compression_choice::target_default;
    auto compression_value = single_value_for(parsed.value(), "compress");
    if (!compression_value)
    {
      return usage_failure(compression_value.error(), print_pack_usage);
    }
    if (compression_value.value().has_value())
    {
      auto parsed_compression = parse_compression(*compression_value.value());
      if (!parsed_compression)
      {
        return usage_failure(parsed_compression.error(), print_pack_usage);
      }
      compression = parsed_compression.value();
    }

    if (compression == compression_choice::raw && !format->supports_raw)
    {
      return usage_failure(make_usage_error("selected --format does not support raw compression"), print_pack_usage);
    }
    if (compression == compression_choice::compressed && !format->supports_compressed)
    {
      return usage_failure(make_usage_error("selected --format does not support compressed payloads"), print_pack_usage);
    }

    auto input_dir = path_from_utf8(parsed.value().positionals[0]);
    if (!input_dir)
    {
      render_error(input_dir.error(), parsed.value().positionals[0]);
      return static_cast<int>(process_exit::operational_failure);
    }
    auto output_path = path_from_utf8(parsed.value().positionals[1]);
    if (!output_path)
    {
      render_error(output_path.error(), parsed.value().positionals[1]);
      return static_cast<int>(process_exit::operational_failure);
    }

    auto files = collect_input_files(input_dir.value());
    if (!files)
    {
      render_error(files.error());
      return static_cast<int>(process_exit::operational_failure);
    }

    const bool overwrite = has_flag(parsed.value(), "overwrite");
    int exit_code = static_cast<int>(process_exit::operational_failure);
    switch (format->family)
    {
    case writer_family::tes3_bsa:
      exit_code = pack_tes3(files.value(), output_path.value(), overwrite);
      break;
    case writer_family::tes4_bsa:
      exit_code = pack_tes4(*format, files.value(), output_path.value(), overwrite, compression);
      break;
    case writer_family::ba2_gnrl:
      exit_code = pack_ba2_gnrl(*format, files.value(), output_path.value(), overwrite, compression);
      break;
    case writer_family::ba2_dx10:
      exit_code = pack_ba2_dx10(*format, files.value(), output_path.value(), overwrite);
      break;
    }

    if (exit_code == static_cast<int>(process_exit::success))
    {
      std::cout << "packed " << files.value().size() << " file(s) into " << output_path.value().string() << '\n';
    }
    return exit_code;
  }

  /// Payload sink that writes every extracted byte to one host file.
  class file_payload_sink final : public libbsa::payload_sink
  {
  public:
    explicit file_payload_sink(std::ofstream stream) : stream_(std::move(stream)) {}

    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override
    {
      if (!bytes.empty())
      {
        stream_.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
      }
      if (!stream_.good())
      {
        return make_error(libbsa::error_code::io_error, "failed to write extracted payload bytes");
      }
      return bytes.size();
    }

  private:
    std::ofstream stream_;
  };

  bool is_within_root(const std::filesystem::path &root, const std::filesystem::path &candidate)
  {
    const auto normalized_root = root.lexically_normal();
    const auto normalized_candidate = candidate.lexically_normal();

    auto root_it = normalized_root.begin();
    auto candidate_it = normalized_candidate.begin();
    for (; root_it != normalized_root.end(); ++root_it, ++candidate_it)
    {
      if (candidate_it == normalized_candidate.end() || *root_it != *candidate_it)
      {
        return false;
      }
    }
    return true;
  }

  libbsa::result<std::filesystem::path> safe_destination_path(const std::filesystem::path &output_root,
                                                              std::string_view entry_path)
  {
    std::string normalized_entry{entry_path};
    std::replace(normalized_entry.begin(), normalized_entry.end(), '\\', '/');
    if (normalized_entry.empty())
    {
      return make_error(libbsa::error_code::invalid_argument, "empty archive entry path");
    }

    auto decoded_relative = path_from_utf8(normalized_entry);
    if (!decoded_relative)
    {
      return decoded_relative.error();
    }
    const std::filesystem::path relative = std::move(decoded_relative).value();
    if (relative.is_absolute() || relative.has_root_name() || relative.has_root_directory())
    {
      return make_error(libbsa::error_code::invalid_argument,
                        "refusing absolute archive entry path: " + normalized_entry);
    }
    for (const auto &component : relative)
    {
      if (component == "..")
      {
        return make_error(libbsa::error_code::invalid_argument,
                          "refusing parent-directory traversal in archive entry path: " + normalized_entry);
      }
    }

#if defined(_WIN32)
    auto windows_destination_path = reject_windows_unsafe_destination_components(normalized_entry);
    if (!windows_destination_path)
    {
      return windows_destination_path.error();
    }
#endif

    const auto destination = (output_root / relative).lexically_normal();
    if (!is_within_root(output_root, destination))
    {
      return make_error(libbsa::error_code::invalid_argument,
                        "refusing archive entry path outside output directory: " + normalized_entry);
    }
    return destination;
  }

  /// Bulk extraction factory that validates destinations before creating file sinks.
  class file_sink_factory final : public libbsa::bulk_extract_sink_factory
  {
  public:
    file_sink_factory(std::filesystem::path output_root, bool overwrite)
        : output_root_(std::move(output_root)), overwrite_(overwrite)
    {
    }

    libbsa::result<std::unique_ptr<libbsa::payload_sink>> create(std::string_view path,
                                                                 const libbsa::entry_metadata &) override
    {
      auto destination = safe_destination_path(output_root_, path);
      if (!destination)
      {
        return destination.error();
      }

      auto destination_reparse = reject_reparse_point(destination.value(), "destination");
      if (!destination_reparse)
      {
        return destination_reparse.error();
      }

      auto ancestors_reparse = reject_reparse_ancestors(output_root_, destination.value());
      if (!ancestors_reparse)
      {
        return ancestors_reparse.error();
      }

      std::error_code fs_error;
      const bool exists = std::filesystem::exists(destination.value(), fs_error);
      if (fs_error)
      {
        return make_error(libbsa::error_code::io_error,
                          "cannot inspect destination '" + destination.value().string() + "': " + fs_error.message());
      }
      if (exists && !overwrite_)
      {
        return make_error(libbsa::error_code::io_error,
                          "destination exists and --overwrite was not specified: " + destination.value().string());
      }
      if (exists && std::filesystem::is_directory(destination.value(), fs_error))
      {
        return make_error(libbsa::error_code::io_error,
                          "destination is a directory: " + destination.value().string());
      }
      if (fs_error)
      {
        return make_error(libbsa::error_code::io_error,
                          "cannot inspect destination type '" + destination.value().string() + "': " + fs_error.message());
      }

      const auto parent = destination.value().parent_path();
      if (!parent.empty())
      {
        std::filesystem::create_directories(parent, fs_error);
        if (fs_error)
        {
          return make_error(libbsa::error_code::io_error,
                            "cannot create destination directory '" + parent.string() + "': " + fs_error.message());
        }
      }

      ancestors_reparse = reject_reparse_ancestors(output_root_, destination.value());
      if (!ancestors_reparse)
      {
        return ancestors_reparse.error();
      }

      destination_reparse = reject_reparse_point(destination.value(), "destination");
      if (!destination_reparse)
      {
        return destination_reparse.error();
      }

      std::ofstream stream{destination.value(), std::ios::binary | std::ios::trunc};
      if (!stream.is_open())
      {
        return make_error(libbsa::error_code::io_error,
                          "cannot open destination for writing: " + destination.value().string());
      }
      return std::unique_ptr<libbsa::payload_sink>{new file_payload_sink{std::move(stream)}};
    }

  private:
    std::filesystem::path output_root_;
    bool overwrite_;
  };

  libbsa::result<std::filesystem::path> prepare_output_root(const std::filesystem::path &output_dir)
  {
    std::error_code fs_error;
    const auto root = std::filesystem::absolute(output_dir, fs_error).lexically_normal();
    if (fs_error)
    {
      return make_error(libbsa::error_code::io_error,
                        "cannot resolve output directory '" + output_dir.string() + "': " + fs_error.message());
    }

    if (std::filesystem::exists(root, fs_error))
    {
      if (fs_error)
      {
        return make_error(libbsa::error_code::io_error,
                          "cannot inspect output directory '" + root.string() + "': " + fs_error.message());
      }
      if (!std::filesystem::is_directory(root, fs_error))
      {
        return make_error(libbsa::error_code::io_error, "output path is not a directory: " + root.string());
      }
      auto root_reparse = reject_reparse_point(root, "output directory");
      if (!root_reparse)
      {
        return root_reparse.error();
      }
    }
    else
    {
      std::filesystem::create_directories(root, fs_error);
      if (fs_error)
      {
        return make_error(libbsa::error_code::io_error,
                          "cannot create output directory '" + root.string() + "': " + fs_error.message());
      }
    }
    return root;
  }

  int run_unpack(std::span<const std::string_view> args)
  {
    constexpr std::array specs{
        option_spec{"path", option_kind::value},
        option_spec{"overwrite", option_kind::flag},
    };

    auto parsed = parse_options(args, specs);
    if (!parsed)
    {
      return usage_failure(parsed.error(), print_unpack_usage);
    }
    if (parsed.value().help)
    {
      print_unpack_usage(std::cout);
      return static_cast<int>(process_exit::success);
    }
    if (parsed.value().version)
    {
      print_version(std::cout);
      return static_cast<int>(process_exit::success);
    }
    if (parsed.value().positionals.size() != 2U)
    {
      return usage_failure(make_usage_error("unpack requires <archive> and <output-dir>"), print_unpack_usage);
    }

    auto opened = libbsa::archive_reader::open(parsed.value().positionals[0]);
    if (!opened)
    {
      render_error(opened.error(), parsed.value().positionals[0]);
      return static_cast<int>(process_exit::operational_failure);
    }

    auto output_dir = path_from_utf8(parsed.value().positionals[1]);
    if (!output_dir)
    {
      render_error(output_dir.error(), parsed.value().positionals[1]);
      return static_cast<int>(process_exit::operational_failure);
    }

    auto output_root = prepare_output_root(output_dir.value());
    if (!output_root)
    {
      render_error(output_root.error());
      return static_cast<int>(process_exit::operational_failure);
    }

    std::vector<libbsa::bulk_extract_request> requests;
    const auto selected_paths = values_for(parsed.value(), "path");
    if (!selected_paths.empty())
    {
      requests.reserve(selected_paths.size());
      for (const auto &path : selected_paths)
      {
        requests.push_back(libbsa::bulk_extract_request{path});
      }
    }
    else
    {
      auto entries = opened.value().entries();
      if (!entries)
      {
        render_error(entries.error(), parsed.value().positionals[0]);
        return static_cast<int>(process_exit::operational_failure);
      }
      requests.reserve(entries.value().size());
      for (const auto &entry : entries.value())
      {
        requests.push_back(libbsa::bulk_extract_request{entry.path});
      }
    }

    file_sink_factory sink_factory{output_root.value(), has_flag(parsed.value(), "overwrite")};
    auto extracted = opened.value().extract_entries(requests, sink_factory, libbsa::bulk_extract_options{});
    if (!extracted)
    {
      render_error(extracted.error(), parsed.value().positionals[0]);
      return static_cast<int>(process_exit::operational_failure);
    }

    std::size_t failure_count = 0;
    for (const auto &result : extracted.value())
    {
      if (result.failure.has_value())
      {
        ++failure_count;
        render_error(*result.failure, result.path);
      }
    }

    std::cout << "extracted " << (extracted.value().size() - failure_count) << " of " << extracted.value().size()
              << " requested entr" << (extracted.value().size() == 1U ? "y" : "ies") << '\n';
    return failure_count == 0U ? static_cast<int>(process_exit::success)
                               : static_cast<int>(process_exit::operational_failure);
  }

  int run_list(std::span<const std::string_view> args)
  {
    constexpr std::array specs{
        option_spec{"details", option_kind::flag},
        option_spec{"detail", option_kind::flag},
    };

    auto parsed = parse_options(args, specs);
    if (!parsed)
    {
      return usage_failure(parsed.error(), print_list_usage);
    }
    if (parsed.value().help)
    {
      print_list_usage(std::cout);
      return static_cast<int>(process_exit::success);
    }
    if (parsed.value().version)
    {
      print_version(std::cout);
      return static_cast<int>(process_exit::success);
    }
    if (parsed.value().positionals.size() != 1U)
    {
      return usage_failure(make_usage_error("list requires <archive>"), print_list_usage);
    }

    auto opened = libbsa::archive_reader::open(parsed.value().positionals[0]);
    if (!opened)
    {
      render_error(opened.error(), parsed.value().positionals[0]);
      return static_cast<int>(process_exit::operational_failure);
    }
    auto entries = opened.value().entries();
    if (!entries)
    {
      render_error(entries.error(), parsed.value().positionals[0]);
      return static_cast<int>(process_exit::operational_failure);
    }

    const bool details = has_flag(parsed.value(), "details") || has_flag(parsed.value(), "detail");
    for (const auto &entry : entries.value())
    {
      std::cout << entry.path;
      if (details)
      {
        std::cout << " raw=" << entry.raw_size << " stored=" << entry.stored_size
                  << " compression=" << compression_name(entry.compression);
      }
      std::cout << '\n';
    }
    return static_cast<int>(process_exit::success);
  }

  int run_info(std::span<const std::string_view> args)
  {
    auto parsed = parse_options(args, std::span<const option_spec>{});
    if (!parsed)
    {
      return usage_failure(parsed.error(), print_info_usage);
    }
    if (parsed.value().help)
    {
      print_info_usage(std::cout);
      return static_cast<int>(process_exit::success);
    }
    if (parsed.value().version)
    {
      print_version(std::cout);
      return static_cast<int>(process_exit::success);
    }
    if (parsed.value().positionals.size() != 1U)
    {
      return usage_failure(make_usage_error("info requires <archive>"), print_info_usage);
    }

    auto opened = libbsa::archive_reader::open(parsed.value().positionals[0]);
    if (!opened)
    {
      render_error(opened.error(), parsed.value().positionals[0]);
      return static_cast<int>(process_exit::operational_failure);
    }
    auto metadata = opened.value().metadata();
    if (!metadata)
    {
      render_error(metadata.error(), parsed.value().positionals[0]);
      return static_cast<int>(process_exit::operational_failure);
    }

    std::cout << "type: " << archive_type_name(metadata.value().type) << '\n'
              << "variant: " << archive_variant_name(metadata.value().variant) << '\n'
              << "version: " << metadata.value().version << '\n'
              << "archive_flags: 0x" << std::hex << metadata.value().archive_flags << std::dec << '\n'
              << "file_count: " << metadata.value().file_count << '\n'
              << "default_compression: " << compression_name(metadata.value().default_compression) << '\n';
    if (metadata.value().ba2.has_value())
    {
      const auto &ba2 = *metadata.value().ba2;
      if (ba2.starfield_unknown1.has_value())
      {
        std::cout << "ba2.starfield_unknown1: " << *ba2.starfield_unknown1 << '\n';
      }
      if (ba2.starfield_unknown2.has_value())
      {
        std::cout << "ba2.starfield_unknown2: " << *ba2.starfield_unknown2 << '\n';
      }
      if (ba2.compression_method.has_value())
      {
        std::cout << "ba2.compression_method: " << *ba2.compression_method << '\n';
      }
    }
    return static_cast<int>(process_exit::success);
  }

  int run_validate(std::span<const std::string_view> args)
  {
    constexpr std::array specs{
        option_spec{"strict", option_kind::flag},
    };

    auto parsed = parse_options(args, specs);
    if (!parsed)
    {
      return usage_failure(parsed.error(), print_validate_usage);
    }
    if (parsed.value().help)
    {
      print_validate_usage(std::cout);
      return static_cast<int>(process_exit::success);
    }
    if (parsed.value().version)
    {
      print_version(std::cout);
      return static_cast<int>(process_exit::success);
    }
    if (parsed.value().positionals.size() != 1U)
    {
      return usage_failure(make_usage_error("validate requires <archive>"), print_validate_usage);
    }

    auto validated = libbsa::validate_archive(parsed.value().positionals[0]);
    if (!validated)
    {
      render_error(validated.error(), parsed.value().positionals[0]);
      return static_cast<int>(process_exit::operational_failure);
    }

    const auto &report = validated.value();
    std::cout << "valid: " << (report.is_valid() ? "yes" : "no") << '\n';
    if (report.metadata.has_value())
    {
      std::cout << "type: " << archive_type_name(report.metadata->type) << '\n'
                << "variant: " << archive_variant_name(report.metadata->variant) << '\n'
                << "file_count: " << report.metadata->file_count << '\n';
    }
    for (const auto &diagnostic : report.errors)
    {
      std::cout << "error: " << error_code_name(diagnostic.code) << ": " << diagnostic.message << '\n';
    }
    for (const auto &warning : report.warnings)
    {
      std::cout << "warning: " << warning_code_name(warning.code)
                << " severity=" << warning_severity_name(warning.severity)
                << " message=" << warning.message;
      if (warning.archive_path.has_value())
      {
        std::cout << " path=" << *warning.archive_path;
      }
      std::cout << '\n';
    }

    if (!report.is_valid() || (has_flag(parsed.value(), "strict") && !report.warnings.empty()))
    {
      return static_cast<int>(process_exit::operational_failure);
    }
    return static_cast<int>(process_exit::success);
  }

  int dispatch(std::span<const std::string_view> args)
  {
    if (args.empty())
    {
      std::cerr << "usage error: missing subcommand\n\n";
      print_top_level_usage(std::cerr);
      return static_cast<int>(process_exit::usage_error);
    }

    const auto command = args.front();
    if (command == "-h" || command == "--help")
    {
      print_top_level_usage(std::cout);
      return static_cast<int>(process_exit::success);
    }
    if (command == "-V" || command == "--version")
    {
      print_version(std::cout);
      return static_cast<int>(process_exit::success);
    }

    const auto sub_args = args.subspan(1U);
    if (command == "pack")
    {
      return run_pack(sub_args);
    }
    if (command == "unpack")
    {
      return run_unpack(sub_args);
    }
    if (command == "list")
    {
      return run_list(sub_args);
    }
    if (command == "info")
    {
      return run_info(sub_args);
    }
    if (command == "validate")
    {
      return run_validate(sub_args);
    }

    std::cerr << "usage error: unknown subcommand '" << command << "'\n\n";
    print_top_level_usage(std::cerr);
    return static_cast<int>(process_exit::usage_error);
  }
} // namespace

int main(int argc, char **argv)
{
  try
  {
    auto owned_args = command_line_arguments(argc, argv);
    if (!owned_args)
    {
      render_error(owned_args.error());
      return static_cast<int>(process_exit::operational_failure);
    }

    std::vector<std::string_view> args;
    args.reserve(owned_args.value().size());
    for (const auto &argument : owned_args.value())
    {
      args.emplace_back(argument);
    }
    return dispatch(args);
  }
  catch (const std::exception &ex)
  {
    std::cerr << "error: unhandled exception: " << ex.what() << '\n';
    return static_cast<int>(process_exit::operational_failure);
  }
  catch (...)
  {
    std::cerr << "error: unhandled non-standard exception\n";
    return static_cast<int>(process_exit::operational_failure);
  }
}
