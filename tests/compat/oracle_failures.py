"""Match exact reviewed native-oracle failures without waiving archive verification.

A match binds one tool, one archive, explicit command options, an ordinary exit
status and a complete set of exact error lines. It returns the reviewed affected
entries for the caller to check against missing oracle output and libbsa's pinned
regression observations. It does not certify those entries independently, waive
other entries, or decide that an entire archive/release passed.
"""

import copy
import re


_SHA256 = re.compile(r"[0-9a-f]{64}\Z")
_ERROR_PREFIX = re.compile(
    r"^\s*(?:\[[^\]\r\n]+\]\s*)?"
    r"(?:error\b|exception\b|fatal\b|[A-Za-z_][A-Za-z0-9_]*(?:Error|Exception)\s*:|EAccessViolation\s*:)",
    re.IGNORECASE,
)
_LOG_LINES = re.compile(r"[^\r\n]+")
_ASCII_LOWER = str.maketrans("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "abcdefghijklmnopqrstuvwxyz")


def _require(condition, message):
    if not condition:
        raise ValueError(message)


def _text(value):
    return isinstance(value, str) and bool(value.strip()) and not any(c in value for c in "\r\n\0")


def _hash(value):
    return isinstance(value, str) and _SHA256.fullmatch(value) is not None


def _path(value):
    """Validate a relative archive/corpus path and return its ASCII-canonical key."""
    _require(_text(value), "entry/archive path must be nonempty single-line text")
    normalized = value.replace("\\", "/").translate(_ASCII_LOWER)
    _require(":" not in normalized and all(part not in ("", ".", "..")
                                          for part in normalized.split("/")),
             "entry/archive path must be relative and contain no dot or empty components")
    return normalized


def _options(value):
    """Require an argv list, preserving order and exact spelling for identity matching."""
    return isinstance(value, list) and all(_text(option) for option in value)


def _validate_failure(failure):
    """Validate a complete reviewed record while allowing descriptive extra metadata."""
    _require(isinstance(failure, dict), "each oracle failure must be an object")
    required = {"id", "review", "oracle_sha256", "archive_sha256", "archive_path", "phase",
                "exit_code", "allowed_options", "diagnostics", "entries"}
    _require(required <= failure.keys(), "oracle failure is missing required fields")
    _require(_text(failure["id"]), "oracle failure id must be nonempty single-line text")
    _require(_hash(failure["oracle_sha256"]), "oracle_sha256 must be 64 lowercase hex digits")
    _require(_hash(failure["archive_sha256"]), "archive_sha256 must be 64 lowercase hex digits")
    _path(failure["archive_path"])
    _require(failure["phase"] == "retail-unpack", "only retail-unpack failures can be enrolled")
    _require(type(failure["exit_code"]) is int and failure["exit_code"] == 1,
             "reviewed native-oracle failures require ordinary exit code 1")

    review = failure["review"]
    _require(isinstance(review, dict), "oracle failure review must be an object")
    _require({"status", "gate_qualifies", "reviewed_by", "reason"} <= review.keys(),
             "oracle failure review is missing required fields")
    _require(review["status"] in ("approved", "pending", "rejected"), "invalid review status")
    _require(type(review["gate_qualifies"]) is bool, "gate_qualifies must be a boolean")
    reviewer = review["reviewed_by"]
    if review["status"] == "pending":
        _require(isinstance(reviewer, str) and not any(c in reviewer for c in "\r\n\0"),
                 "pending reviewed_by must be single-line text, possibly empty")
    else:
        _require(_text(reviewer), "reviewed_by must identify the reviewer")
    _require(_text(review["reason"]), "review reason must be nonempty single-line text")

    options = failure["allowed_options"]
    _require(isinstance(options, list) and bool(options), "allowed_options must be a nonempty list")
    _require(all(_options(option) for option in options), "allowed_options must contain argv lists")
    _require(len({tuple(option) for option in options}) == len(options), "duplicate allowed_options")

    diagnostics = failure["diagnostics"]
    _require(isinstance(diagnostics, list) and bool(diagnostics), "diagnostics must be a nonempty list")
    _require(all(_text(line) and _ERROR_PREFIX.match(line) is not None for line in diagnostics),
             "diagnostics must be exact single error/exception/fatal lines")
    _require(len(set(diagnostics)) == len(diagnostics), "duplicate expected diagnostic lines")

    entries = failure["entries"]
    _require(isinstance(entries, list) and bool(entries), "review must name at least one affected entry")
    seen = set()
    for entry in entries:
        _require(isinstance(entry, dict) and {"path", "decoded_size"} <= entry.keys(),
                 "affected entry must contain path and decoded_size")
        path = _path(entry["path"])
        _require(path not in seen, "duplicate canonical affected-entry path")
        seen.add(path)
        size = entry["decoded_size"]
        _require(type(size) is int and 0 <= size <= 0xFFFFFFFFFFFFFFFF,
                 "decoded_size must be a nonnegative uint64 byte count")
        if "decoded_sha256" in entry:
            _require(_hash(entry["decoded_sha256"]), "decoded_sha256 must be 64 lowercase hex digits")
        if "fingerprint_provenance" in entry:
            _require(_text(entry["fingerprint_provenance"]), "fingerprint_provenance must be nonempty text")


def validate_registry(registry) -> None:
    """Validate schema 1 and every stored approval, raising ValueError if malformed.

    Pending, rejected and nonqualifying records may be stored but cannot match.
    Identifiers and affected entry keys must be unique within their scopes.
    An empty registry is valid and grants no exceptions.
    """
    _require(isinstance(registry, dict), "oracle failure registry must be an object")
    _require(type(registry.get("schema")) is int and registry["schema"] == 1,
             "unsupported oracle failure registry schema")
    records = registry.get("failures")
    _require(isinstance(records, list), "registry failures must be a list")
    seen = set()
    for failure in records:
        _validate_failure(failure)
        _require(failure["id"] not in seen, "duplicate oracle failure id")
        seen.add(failure["id"])


def match_failure(registry, *, oracle_sha256, archive_sha256, options: list[str],
                  exit_code: int, log_text: str) -> dict | None:
    """Return a detached approved record only for one exact qualifying observation.

    Invalid registry/input, timeouts, crashes, changed identities/options, duplicate
    or additional errors and ambiguous approvals return None. The caller should
    call validate_registry at configuration load to report malformed configuration.
    CR and LF separate log records; no case, whitespace or diagnostic text is
    normalized. The returned entries still require explicit partial-output checks.
    """
    try:
        validate_registry(registry)
    except ValueError:
        return None
    if (not _hash(oracle_sha256) or not _hash(archive_sha256) or not _options(options)
            or type(exit_code) is not int or exit_code != 1
            or not isinstance(log_text, str) or "\0" in log_text):
        return None
    candidates = [failure for failure in registry["failures"]
                  if failure["review"]["status"] == "approved"
                  and failure["review"]["gate_qualifies"]
                  and failure["oracle_sha256"] == oracle_sha256
                  and failure["archive_sha256"] == archive_sha256
                  and options in failure["allowed_options"]
                  and failure["exit_code"] == exit_code
                  and failure["phase"] == "retail-unpack"]
    if not candidates:
        return None
    permitted = {line for failure in candidates for line in failure["diagnostics"]}
    observed = set()
    # Iterate lines instead of copying/splitting a potentially large unpack log.
    # Only reviewed error strings are retained; an unexpected error fails early.
    for match in _LOG_LINES.finditer(log_text):
        line = match.group(0)
        if _ERROR_PREFIX.match(line):
            if line not in permitted or line in observed:
                return None
            observed.add(line)
    matches = [failure for failure in candidates if set(failure["diagnostics"]) == observed]
    if len(matches) != 1:
        return None
    return copy.deepcopy(matches[0])
