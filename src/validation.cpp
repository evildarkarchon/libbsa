#include <libbsa/validation.hpp>

#include <fstream>
#include <string>

namespace libbsa {
namespace {

std::string diagnostic_message_for(error_code code) {
  switch (code) {
  case error_code::unsupported:
    return "archive format is unsupported";
  case error_code::format_error:
    return "archive bytes are malformed";
  case error_code::invalid_argument:
    return "validation input is invalid";
  case error_code::io_error:
    return "archive host path could not be read";
  case error_code::not_found:
    return "archive entry could not be found during validation";
  }
  return "archive validation failed";
}

void append_fatal(validation_report& report, error_code code) {
  report.valid = false;
  report.errors.push_back(validation_diagnostic{code, diagnostic_message_for(code)});
}

bool host_path_can_be_opened(std::string_view host_path) {
  std::ifstream input{std::string{host_path}, std::ios::binary};
  return static_cast<bool>(input);
}

validation_report report_from_open_error(const error& err) {
  validation_report report;
  append_fatal(report, err.code);
  return report;
}

void validate_extractability(const archive_reader& reader, validation_report& report) {
  auto entries = reader.entries();
  if (!entries) {
    append_fatal(report, entries.error().code);
    return;
  }

  for (const auto& entry : entries.value()) {
    auto extracted = reader.extract_bytes(entry.path);
    if (!extracted) {
      append_fatal(report, extracted.error().code);
    }
  }
}

} // namespace

bool validation_report::is_valid() const noexcept { return valid && errors.empty(); }

result<validation_report> validate_archive(std::string_view host_path, validation_options options) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }
  if (!host_path_can_be_opened(host_path)) {
    return error{error_code::io_error, "failed to open archive host path"};
  }

  auto opened = archive_reader::open(host_path);
  if (!opened) {
    const auto& err = opened.error();
    // Setup failures stay result-level; readable archive bytes become report diagnostics.
    if (err.code == error_code::io_error || err.code == error_code::invalid_argument) {
      return err;
    }
    return report_from_open_error(err);
  }

  validation_report report;
  auto metadata = opened.value().metadata();
  if (!metadata) {
    append_fatal(report, metadata.error().code);
    return report;
  }
  report.metadata = metadata.value();
  report.valid = true;

  if (options.validate_entry_extractability) {
    validate_extractability(opened.value(), report);
  }

  report.valid = report.errors.empty();
  return report;
}

} // namespace libbsa
