#include "extraction.hpp"
#include "path_support.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <span>
#include <stdexcept>
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
#endif

namespace libbsa::cli {
namespace {

libbsa::error make_error(libbsa::error_code code, std::string message) {
    return libbsa::error{code, std::move(message)};
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

bool is_within_root(const std::filesystem::path& root, const std::filesystem::path& candidate);

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

/// Builds a candidate name with the established suffix length; CREATE_NEW arbitrates collisions.
std::filesystem::path make_staged_temp_path(const std::filesystem::path& final_path,
                                            std::uint64_t id) {
    const auto parent = final_path.parent_path();
#if defined(_WIN32)
    const auto name = final_path.filename().wstring();
    const auto process_id = static_cast<std::uint64_t>(::GetCurrentProcessId());
    return parent /
           (L"." + name + L".bsa-tmp-" + std::to_wstring(process_id) + L"-" + std::to_wstring(id));
#else
    const auto name = final_path.filename().string();
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
using extraction_plan = std::map<std::string, libbsa::result<std::filesystem::path>, std::less<>>;

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
    /// Takes ownership of a complete plan before any worker is dispatched.
    file_sink_factory(extraction_plan plan, std::wstring output_root_final_path, bool overwrite)
        : plan_(std::move(plan)),
          output_root_final_path_(std::move(output_root_final_path)),
          overwrite_(overwrite) {}

    /// Discards any staged files left after worker completion or an early exit.
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

    /// Opens and tracks one planned destination; independent requests may call concurrently.
    libbsa::result<std::unique_ptr<libbsa::payload_sink>> create(
        std::string_view path, const libbsa::entry_metadata& entry) override {
        // The destination was derived from this entry's archive spelling during the
        // pre-pass, so the entry metadata is not consulted again here.
        (void)entry;

        const auto planned = plan_.find(path);
        if (planned == plan_.end()) {
            return make_error(
                libbsa::error_code::io_error,
                "no planned extraction destination for archive path: " + std::string{path});
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
            return make_error(
                libbsa::error_code::io_error,
                "cannot inspect destination '" + destination.string() + "': " + fs_error.message());
        }
        if (exists && !overwrite_) {
            return make_error(
                libbsa::error_code::io_error,
                "destination exists and --overwrite was not specified: " + destination.string());
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
            const auto temp_path = make_staged_temp_path(
                destination, next_temp_id_.fetch_add(1U, std::memory_order_relaxed));
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
    // The operation owns its sequence; CREATE_NEW and retries arbitrate names shared with another
    // operation or stale file. Keep the old suffix length so long destination leaves still fit.
    std::atomic<std::uint64_t> next_temp_id_{0U};
    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<staged_extraction>> staged_;
    std::map<std::string, std::shared_ptr<staged_extraction>, std::less<>> by_path_;
};

/// Resolves and prepares the output directory before entry selection, preserving setup order.
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

}  // namespace

extraction_outcome extract(const extraction_options& options) {
    if (options.worker_count == 0U || options.worker_count > 1024U) {
        throw std::invalid_argument{"CLI Extraction requires a parsed worker count in [1, 1024]"};
    }

    // Archive opening must precede output decoding and creation: an invalid archive leaves the
    // destination untouched, whereas a valid archive with only missing selections creates its root.
    auto opened = libbsa::archive_reader::open(options.archive_path);
    if (!opened) {
        return extraction_failure{opened.error(), options.archive_path};
    }

    auto output_dir = path_from_utf8(options.output_directory);
    if (!output_dir) {
        return extraction_failure{output_dir.error(), options.output_directory};
    }

    auto output_root = prepare_output_root(output_dir.value());
    if (!output_root) {
        return extraction_failure{output_root.error(), {}};
    }

    std::vector<libbsa::bulk_extract_request> requests;
    const auto& selected_paths = options.selected_paths;
    if (!selected_paths.empty()) {
        requests.reserve(selected_paths.size());
        for (const auto& path : selected_paths) {
            requests.push_back(libbsa::bulk_extract_request{path});
        }
    } else {
        auto entries = opened.value().entries();
        if (!entries) {
            return extraction_failure{entries.error(), options.archive_path};
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
        return extraction_failure{output_root_final_path.error(), {}};
    }

    // Serial pre-pass: containment is verified and every distinct destination
    // directory is created here, before any worker thread starts, so an entry path
    // that cannot be written safely is rejected before any file is created.
    auto plan = plan_extraction(opened.value(), output_root.value(), requests);

    file_sink_factory sink_factory{std::move(plan), std::move(output_root_final_path).value(),
                                   options.overwrite};
    auto extracted = opened.value().extract_entries(
        requests, sink_factory, libbsa::bulk_extract_options{options.worker_count});
    if (!extracted) {
        return extraction_failure{extracted.error(), options.archive_path};
    }

    // The factory releases any remaining staged state as this operation exits, after all
    // workers have joined. Presentation sees only completed outcomes, never a live extraction.
    return std::move(extracted).value();
}

}  // namespace libbsa::cli
