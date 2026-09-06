"""Independent local archive interoperability runner (ADR-0005).

Only the tiny bridge calls libbsa. Oracle evidence, inventory, content comparison,
and release eligibility are derived independently in this test-only runner.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
SCHEMA = 1
ORACLE_FAILURES = HERE / "oracle-failures.json"


def accepted_case_errors(case, report, registry):
    """Revalidate a reviewed entry-level exception and all remaining coverage obligations."""
    from oracle_failures import match_failure
    exceptions = case.get("oracle_exceptions", [])
    if case.get("status") == "passed":
        return ["An oracle exception cannot be labeled as an ordinary pass"] if exceptions else []
    if case.get("status") != "accepted_oracle_failure" or case.get("kind") != "retail" or not exceptions:
        return ["Every required comparison must pass or carry a reviewed oracle exception"]
    record = next((item for item in report.get("corpus", []) if item["path"] == case.get("id")), None)
    if record is None:
        return ["Accepted oracle failure is not bound to a supplied retail archive"]
    errors, waived = [], []
    for evidence in exceptions:
        matched = match_failure(
            registry, oracle_sha256=report.get("oracle", {}).get("sha256"),
            archive_sha256=record["sha256"], options=evidence.get("options"),
            exit_code=evidence.get("exit_code"), log_text="\n".join(evidence.get("diagnostics", [])))
        if matched is None or any(evidence.get(key) != matched[key] for key in
                                  ("id", "oracle_sha256", "archive_sha256", "entries", "diagnostics")):
            errors.append("Oracle exception does not match a current qualifying review")
            continue
        if matched["archive_path"] != case["id"] or evidence.get("original_content_verified") is not False:
            errors.append("Oracle exception scope or independent-proof claim is incorrect")
        waived.extend(entry["path"] for entry in matched["entries"])
    if len(waived) != len(set(waived)):
        errors.append("Duplicate oracle exception entries")
    total = record["profile"]["entry_count"]
    if (case.get("entries") != total or case.get("oracle_compared_entries") != total - len(waived) or
            case.get("repack_verified_entries") != total):
        errors.append("Accepted oracle failure has incomplete remaining read or repack coverage")
    return errors


def discover(root):
    """Discover supplied archives recursively, refusing linked corpus entries."""
    root = Path(root).resolve(strict=True)
    found = []
    for path in root.rglob("*"):
        if path.suffix.lower() not in (".bsa", ".ba2") or not path.is_file():
            continue
        if path.is_symlink() or not path.resolve().is_relative_to(root):
            raise ValueError(f"Linked archive is outside the corpus contract: {path}")
        found.append(path)
    return sorted(found, key=lambda p: p.relative_to(root).as_posix().lower())


def sha256(path):
    """Fingerprint an entire file with bounded memory; detect concurrent changes."""
    before = path.stat()
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(8 * 1024 * 1024), b""):
            digest.update(block)
    after = path.stat()
    if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
        raise ValueError(f"File changed during fingerprinting: {path}")
    return digest.hexdigest()


def read_json(path):
    """Load UTF-8 JSON, allowing a Windows UTF-8 BOM."""
    return json.loads(Path(path).read_text(encoding="utf-8-sig"))


def write_json(path, value):
    """Publish a complete report atomically within its destination directory."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=path.parent,
                                     delete=False, suffix=".tmp") as stream:
        temporary = Path(stream.name)
        json.dump(value, stream, indent=2, ensure_ascii=True)
        stream.write("\n")
    temporary.replace(path)


def profile(path):
    """Read fixed headers independently, without libbsa decoding helpers."""
    with path.open("rb") as stream:
        data = stream.read(36)
    if len(data) < 12:
        raise ValueError("Truncated archive header")
    if data[:4] == b"BTDX":
        version = struct.unpack_from("<I", data, 4)[0]
        subtype = data[8:12].decode("ascii")
        if version not in (1, 2, 3, 7, 8) or subtype not in ("GNRL", "DX10"):
            raise ValueError("Unsupported BA2 profile")
        width = 36 if version == 3 else 32 if version == 2 else 24
        if len(data) < width:
            raise ValueError("Truncated BA2 header")
        method = struct.unpack_from("<I", data, 32)[0] if version == 3 else 0
        if method not in (0, 3):
            raise ValueError("Unsupported BA2 compression method")
        return {"type": "ba2", "version": version, "subtype": subtype,
                "method": method, "entry_count": struct.unpack_from("<I", data, 12)[0]}
    if data[:4] == b"BSA\0":
        version = struct.unpack_from("<I", data, 4)[0]
        if version not in (103, 104, 105) or len(data) < 36:
            raise ValueError("Unsupported or truncated BSA header")
        return {"type": "bsa", "version": version, "subtype": "",
                "entry_count": struct.unpack_from("<I", data, 20)[0]}
    if struct.unpack_from("<I", data)[0] == 256:
        return {"type": "bsa", "version": 256, "subtype": "",
                "entry_count": struct.unpack_from("<I", data, 8)[0]}
    raise ValueError("Unknown archive magic")


def inventory(root):
    """Read and hash every supplied archive, recording no retail payload bytes."""
    records = []
    root = Path(root).resolve()
    for index, path in enumerate(discover(root), 1):
        print(f"Fingerprinting archive {index}: {path.name}", flush=True)
        records.append({"path": path.relative_to(root).as_posix(), "size": path.stat().st_size,
                        "sha256": sha256(path), "profile": profile(path)})
    return records


def verify_baseline(baseline, actual):
    """Return missing or changed required baseline archive diagnostics."""
    errors = []
    required = baseline.get("archives", [])
    if not required:
        return ["Required retail baseline is empty"]
    lookup = {item["path"]: item for item in actual}
    if len(lookup) != len(actual):
        errors.append("Duplicate corpus identities")
    for expected in required:
        observed = lookup.get(expected["path"])
        if observed is None:
            errors.append(f"Missing required archive: {expected['path']}")
        elif observed["sha256"] != expected["sha256"]:
            errors.append(f"Changed required archive: {expected['path']}")
    return errors


def cache_identity(archive_hash, oracle_hash, options, schema):
    """Identify evidence by its complete independent inputs."""
    encoded = json.dumps([archive_hash, oracle_hash, options, schema],
                         sort_keys=True, separators=(",", ":")).encode()
    return hashlib.sha256(encoded).hexdigest()


def release_errors(report):
    """Reject incomplete, failed, or unbound release evidence."""
    errors = list(report.get("errors", []))
    if report.get("schema") != SCHEMA or not report.get("finished") or report.get("status") not in ("passed", "passed_with_exceptions"):
        errors.append("Release evidence must be a completed successful supported report")
    if report.get("mode") != "thorough":
        errors.append("Release evidence must include the thorough suite")
    if report.get("diagnostic"):
        errors.append("Diagnostic evidence cannot qualify a release")
    if not report.get("source", {}).get("clean") or not report.get("source", {}).get("commit"):
        errors.append("Release evidence requires an exact clean source revision")
    for field in ("build", "oracle", "baseline_sha256", "corpus"):
        if not report.get(field):
            errors.append(f"Missing release identity: {field}")
    cases = report.get("cases", [])
    from synthetic import cases as required_synthetic_cases
    required = [("synthetic", case["id"]) for case in required_synthetic_cases()]
    required += [("retail", archive["path"]) for archive in report.get("corpus", [])]
    observed = [(case.get("kind"), case.get("id")) for case in cases]
    if Counter(required) != Counter(observed) or len(observed) != len(set(observed)):
        errors.append("Required comparison identities are missing, duplicated, or unexpected")
    baseline_path = HERE / "retail-baseline.json"
    if report.get("baseline_sha256") != sha256(baseline_path):
        errors.append("Release baseline must match the committed required corpus")
    errors.extend(verify_baseline(read_json(baseline_path), report.get("corpus", [])))
    from oracle_failures import validate_registry
    registry = read_json(ORACLE_FAILURES)
    validate_registry(registry)
    if report.get("oracle_failures_sha256") != sha256(ORACLE_FAILURES):
        errors.append("Oracle failure review registry is missing or changed")
    for case in cases:
        errors.extend(accepted_case_errors(case, report, registry))
    expected_status = "passed_with_exceptions" if any(c.get("status") == "accepted_oracle_failure" for c in cases) else "passed"
    if report.get("status") != expected_status:
        errors.append("Report status does not disclose its oracle exception verdicts")
    if not cases:
        errors.append("Required comparison results are absent")
    if report.get("expected_cases") != len(cases):
        errors.append("Required comparison count is incomplete")
    if not any(case.get("kind") == "retail" for case in cases):
        errors.append("Retail coverage is absent")
    if not any(case.get("kind") == "synthetic" for case in cases):
        errors.append("Controlled interoperability coverage is absent")
    if report.get("build", {}).get("configuration") != "Release":
        errors.append("The full release gate requires a Release build")
    return errors


def source_identity():
    """Record the exact checkout including dirty state, never silently label it clean."""
    def git(*args):
        return subprocess.check_output(["git", "-C", str(ROOT), *args],
                                       stderr=subprocess.DEVNULL).decode().strip()
    return {"commit": git("rev-parse", "HEAD"),
            "clean": not bool(git("status", "--porcelain")),
            "submodule": git("ls-tree", "HEAD", "TES5Edit")}


def build_identity(args):
    """Bind a run to binaries, runtime DLLs, CMake settings, and dependency pins."""
    bridge = Path(args.bridge).resolve(strict=True)
    library = Path(args.library).resolve(strict=True)
    cache = Path(args.build_dir) / "CMakeCache.txt"
    settings = cache.read_text(encoding="utf-8")
    home = next((line.split("=", 1)[1] for line in settings.splitlines()
                 if line.startswith("CMAKE_HOME_DIRECTORY:")), "")
    if Path(home).resolve() != ROOT:
        raise ValueError("Build directory belongs to a different source tree")
    shared = "BUILD_SHARED_LIBS:BOOL=ON" in settings or "BUILD_SHARED_LIBS:UNINITIALIZED=ON" in settings
    configuration = bridge.parent.name
    if configuration not in ("Debug", "Release"):
        configuration = next((line.split("=", 1)[1] for line in settings.splitlines()
                              if line.startswith("CMAKE_BUILD_TYPE:")), "unknown")
    artifacts = read_json(Path(args.build_dir) / "tests/compat" / f"artifacts-{configuration}.json")
    if (Path(artifacts["bridge"]).resolve() != bridge or
            Path(artifacts["library"]).resolve() != library):
        raise ValueError("Supplied binaries are not the configured CMake targets")
    dlls = sorted({p.resolve() for parent in (bridge.parent, library.parent)
                   for p in parent.glob("*.dll")})
    return {"lane": "shared" if shared else "static", "configuration": configuration,
            "bridge": {"path": str(bridge), "sha256": sha256(bridge)},
            "library": {"path": str(library), "sha256": sha256(library)},
            "runtime_dlls": [{"path": str(p), "sha256": sha256(p)} for p in dlls],
            "cmake_cache_path": str(cache.resolve()), "cmake_cache_sha256": sha256(cache),
            "dependencies": {name: sha256(ROOT / name) for name in
                             ("vcpkg.json", "vcpkg-configuration.json")}}


class Harness:
    """Run isolated native operations and retain logs without retail payload publication."""

    def __init__(self, args):
        self.args = args
        self.scratch = Path(args.scratch).resolve()
        self.scratch.mkdir(parents=True, exist_ok=True)
        self.logs = Path(args.report).resolve().parent / (Path(args.report).stem + "-logs")
        self.logs.mkdir(parents=True, exist_ok=True)
        self.counter = 0
        self.phase = "setup"
        self.oracle = Path(args.oracle).resolve(strict=True)
        pin = read_json(HERE / "oracle.json")
        observed = sha256(self.oracle)
        if observed != pin["sha256"]:
            raise ValueError("BSArch digest differs from committed pin; explicit upgrade required")
        self.oracle_id = {"path": str(self.oracle), "sha256": observed, "banner": pin["banner"]}
        from oracle_failures import validate_registry
        self.failure_registry = read_json(ORACLE_FAILURES)
        validate_registry(self.failure_registry)
        self.failure_registry_sha256 = sha256(ORACLE_FAILURES)
        self.cache = Path(args.cache).resolve()
        self.cache.mkdir(parents=True, exist_ok=True)

    def process(self, argv, workspace):
        """Apply per-operation limits in the operation's private scratch environment."""
        from processes import run_process
        if Path(argv[0]).resolve() == self.oracle:
            self.phase = f"oracle-{argv[1]}"
        occupied = sum(path.stat().st_size for path in workspace.rglob("*") if path.is_file())
        remaining = self.args.scratch_gib * 1024**3 - occupied
        if remaining <= 0:
            raise RuntimeError("Private workspace already exceeds the scratch limit")
        self.counter += 1
        log = self.logs / f"process-{self.counter:06d}.log"
        original = {name: os.environ.get(name) for name in ("TEMP", "TMP", "PATH")}
        os.environ["TEMP"] = os.environ["TMP"] = str(workspace)
        os.environ["PATH"] = str(Path(self.args.library).resolve().parent) + os.pathsep + os.environ["PATH"]
        try:
            run_process([str(a) for a in argv], log, workspace,
                        self.args.timeout, self.args.memory_gib * 1024**3,
                        remaining)
        finally:
            for name, value in original.items():
                if value is None:
                    os.environ.pop(name, None)
                else:
                    os.environ[name] = value

    def bridge(self, request, workspace):
        """Invoke the public-library bridge using unambiguous JSON host paths."""
        self.phase = "libbsa-" + request["command"]
        request_path, response_path = workspace / "request.json", workspace / "response.json"
        write_json(request_path, request)
        if response_path.exists():
            response_path.unlink()
        try:
            self.process([self.args.bridge, "--request", request_path, "--response", response_path], workspace)
        except RuntimeError as error:
            if response_path.is_file():
                response = read_json(response_path)
                raise RuntimeError(f"{error}; bridge: {response.get('error', 'unknown error')}") from error
            raise
        response = read_json(response_path)
        if not response.get("ok"):
            raise ValueError(response.get("error", "Bridge returned no success verdict"))
        return response

    def oracle_catalog(self, archive, workspace, options, cache_hash=None, approved_failures=None):
        """Obtain independent fingerprints, disclosing narrowly approved retail read gaps.

        Only retail callers may supply approved_failures to receive fresh, exact
        reviewed failures. Such partial catalogs are never stored as oracle cache
        successes, and no libbsa fingerprint is inserted into independent evidence.
        """
        from fingerprints import fingerprint_file, canonical_path
        from oracle_failures import match_failure
        from processes import ProcessExitError
        comparator = sha256(HERE / "fingerprints.py")
        key = cache_identity(cache_hash, self.oracle_id["sha256"],
                             [options, comparator], SCHEMA)
        cached = self.cache / (key + ".json")
        if cache_hash and cached.exists():
            evidence = read_json(cached)
            if evidence.get("identity") != key:
                raise ValueError("Oracle cache identity mismatch")
            if evidence.get("entries_sha256") != hashlib.sha256(
                    json.dumps(evidence["entries"], sort_keys=True).encode()).hexdigest():
                raise ValueError("Oracle cache contents are damaged")
            return evidence["entries"]
        destination = workspace / f"oracle-output-{self.counter}"
        destination.mkdir()
        try:
            accepted = None
            try:
                self.process([self.oracle, "unpack", archive, destination, *options], workspace)
            except ProcessExitError as error:
                if cache_hash is None or approved_failures is None:
                    raise
                accepted = match_failure(self.failure_registry,
                    oracle_sha256=self.oracle_id["sha256"], archive_sha256=cache_hash,
                    options=options, exit_code=error.exit_code,
                    log_text=error.log.read_text(encoding="utf-8-sig", errors="replace"))
                if accepted is None:
                    raise
                if sha256(Path(archive)) != cache_hash or sha256(self.oracle) != self.oracle_id["sha256"]:
                    raise ValueError("Reviewed oracle failure input identity changed") from error
            excluded = {entry["path"] for entry in accepted["entries"]} if accepted else set()
            entries = []
            for file in sorted(destination.rglob("*")):
                if file.is_file():
                    if file.is_symlink() or not file.resolve().is_relative_to(destination):
                        raise ValueError("Oracle output contains a filesystem link")
                    key_path = file.relative_to(destination).as_posix()
                    if canonical_path(key_path) in excluded:
                        continue
                    entries.append(fingerprint_file(file, key_path))
            # Count equality prevents two archive paths silently overwriting one host file.
            if len(entries) + len(excluded) != profile(Path(archive))["entry_count"]:
                raise ValueError("Oracle extraction catalog is incomplete or has colliding host paths")
            if accepted:
                evidence = {key: accepted[key] for key in
                            ("id", "oracle_sha256", "archive_sha256", "entries", "diagnostics", "exit_code")}
                evidence.update(options=list(options), original_content_verified=False,
                                reason=accepted["review"]["reason"])
                approved_failures.append(evidence)
            elif cache_hash:
                write_json(cached, {"identity": key, "entries": entries,
                                   "entries_sha256": hashlib.sha256(
                                       json.dumps(entries, sort_keys=True).encode()).hexdigest()})
            return entries
        finally:
            # This directory was created exclusively by this operation under its private root.
            shutil.rmtree(destination)

    def assert_catalog(self, expected, observed, *, dds_semantics=False):
        """Require every canonical path and its independently comparable contents."""
        from fingerprints import compare_catalogs
        self.phase = "content-comparison"
        differences = compare_catalogs(expected, observed, dds_semantics=dds_semantics)
        if differences:
            raise ValueError("; ".join(differences[:30]) +
                             (f"; {len(differences)} differences total" if len(differences) > 30 else ""))

    def synthetic_case(self, case, workspace):
        """Cross-read controlled sources in both directions and verify writer structure."""
        from fingerprints import fingerprint_file
        from synthetic import prepare_case, adapt_archive
        sources = workspace / "sources"
        entries = prepare_case(case, sources)
        expected = [fingerprint_file(Path(e["source"]), e["path"]) for e in entries]
        native = workspace / ("oracle.bsa" if case["target"].startswith(("tes", "bsa")) else "oracle.ba2")
        self.process([self.oracle, "pack", sources, native, *case["oracle_args"]], workspace)
        oracle_archive = native
        if case.get("adaptation"):
            oracle_archive = workspace / "adapted.ba2"
            adapt_archive(native, oracle_archive, case["adaptation"])
        observed_profile = profile(oracle_archive)
        want = case["expected_profile"]
        actual_version = observed_profile["version"]
        if case["target"] == "tes3":
            actual_version = 256
        if actual_version != want["version"] or observed_profile["subtype"] != (want["subtype"] or ""):
            raise ValueError(f"Oracle did not produce requested profile: {observed_profile}")
        if observed_profile["version"] == 3 and observed_profile["method"] != (3 if case["target"].endswith("lz4") else 0):
            raise ValueError("Oracle did not produce the requested compression method")
        unpack_options = ["-mt:yes" if case["options"]["workers"] > 1 else "-mt:no"]
        semantic = observed_profile["subtype"] == "DX10"
        self.assert_catalog(expected, self.oracle_catalog(oracle_archive, workspace, unpack_options), dds_semantics=semantic)
        scanned = self.bridge({"command": "scan", "archive": str(oracle_archive)}, workspace)
        for key in ("type", "version", "subtype"):
            if scanned["metadata"][key] != observed_profile[key]:
                raise ValueError(f"Reader metadata disagrees with independent header: {key}")
        self.assert_catalog(expected, scanned["entries"], dds_semantics=semantic)
        if not case.get("reader_only"):
            output = workspace / ("libbsa" + native.suffix)
            self.bridge({"command": "pack", "target": case["target"], "entries": entries,
                         "output": str(output), "options": case["options"]}, workspace)
            inspection = self.check_structure(output)
            self.assert_writer_routes(inspection, case)
            produced = profile(output)
            if any(produced.get(key) != observed_profile.get(key) for key in ("type", "version", "subtype", "method")):
                raise ValueError("libbsa writer emitted a different profile")
            self.assert_catalog(expected, self.oracle_catalog(output, workspace, unpack_options), dds_semantics=semantic)
        return {"entries": len(entries), "profile": observed_profile,
                "provenance": "adapted-oracle" if case.get("adaptation") else "native-oracle",
                "options": case["options"]}

    def check_structure(self, archive):
        """Reject writer layout or lookup-identity defects independently of extraction."""
        from structure import inspect_archive
        self.phase = "writer-structure"
        inspection = inspect_archive(archive, strict_writer=True)
        findings = inspection["findings"]
        if findings:
            raise ValueError("Writer structure: " + "; ".join(findings[:20]))
        return inspection

    def assert_writer_routes(self, inspection, case):
        """Verify matrix labels describe encodings actually present in written bytes."""
        options, stats = case["options"], inspection["stats"]
        target = case["target"]
        if target.startswith("gnrl") and inspection["gnrl_record_flags"] != {
                "0x00100100": inspection["profile"]["entry_count"]}:
            raise ValueError("GNRL default record marker differs from the reference")
        if not options["sharing"] and stats["shared_payload_spans"]:
            raise ValueError("Disabled payload sharing still produced shared spans")
        if (options["sharing"] and not case.get("mixed_entries") and
                not options["embedded_names"] and not stats["shared_payload_spans"]):
            raise ValueError("Duplicate controlled inputs did not exercise enabled sharing")
        if target.startswith(("bsa", "gnrl")):
            if options["compression"] == "raw" and stats["compressed_payloads"]:
                raise ValueError("Raw policy unexpectedly emitted compressed payloads")
            if options["compression"] == "compressed" and not stats["compressed_payloads"]:
                raise ValueError("Compressed policy did not exercise a compressed payload")
            if case.get("mixed_entries") and not (stats["raw_payloads"] and stats["compressed_payloads"]):
                raise ValueError("Mixed overrides did not exercise both storage routes")
        if target.startswith("bsa") and bool(inspection["profile"]["embedded_names"]) != options["embedded_names"]:
            raise ValueError("Embedded-name policy differs from written archive flags")
        if target.startswith("dx10") and not stats["compressed_payloads"]:
            raise ValueError("Controlled textures did not exercise the target compression route")

    def retail_case(self, record, root, workspace):
        """Compare all retail entries, then independently check all bounded repack batches."""
        from fingerprints import canonical_path
        archive = root / record["path"]
        before = archive.stat()
        scan = self.bridge({"command": "scan", "archive": str(archive)}, workspace)
        for key in ("type", "version", "subtype"):
            if scan["metadata"][key] != record["profile"][key]:
                raise ValueError(f"Retail metadata disagrees with independent header: {key}")
        exceptions = []
        expected = self.oracle_catalog(archive, workspace, ["-mt:yes"], record["sha256"], exceptions)
        semantic = record["profile"]["subtype"] == "DX10"
        excluded = {entry["path"]: entry for exception in exceptions for entry in exception["entries"]}
        scanned = {canonical_path(entry["path"]): entry for entry in scan["entries"]}
        if len(scanned) != len(scan["entries"]):
            raise ValueError("Duplicate reader catalog keys")
        for path, guard in excluded.items():
            entry = scanned.get(path)
            if (entry is None or entry["size"] != guard["decoded_size"] or
                    (guard.get("decoded_sha256") is not None and entry["sha256"] != guard["decoded_sha256"])):
                raise ValueError("Reviewed entry's libbsa regression guard changed")
        self.assert_catalog(expected, [e for p, e in scanned.items() if p not in excluded], dds_semantics=semantic)
        p = record["profile"]
        if p["type"] == "bsa":
            target = "tes3" if p["version"] == 256 else f"bsa{p['version']}"
        else:
            target = ("gnrl" if p["subtype"] == "GNRL" else "dx10")
            target += str(1 if p["version"] in (7, 8) else p["version"])
            if p["version"] == 3:
                target += "-lz4" if p["method"] == 3 else "-zlib"
        repacked = self.bridge({"command": "repack", "archive": str(archive), "target": target,
                               "output_dir": str(workspace / "repacked"),
                               "batch_bytes": self.args.batch_mib * 1024**2,
                               "options": {"sharing": target != "tes3", "workers": self.args.workers,
                                           "compression": "default"}}, workspace)
        expected_by_path = {canonical_path(e["path"]): e for e in expected}
        # These entries have an explicitly accepted original-content evidence gap.
        # Their libbsa fingerprints verify repack preservation only, never an
        # independent original read, and must not enter the oracle cache.
        expected_by_path.update({path: scanned[path] for path in excluded})
        visited = []
        for batch in repacked["archives"]:
            output = Path(batch["path"])
            self.check_structure(output)
            written_profile = profile(output)
            expected_profile = dict(p, version=1) if p["version"] in (7, 8) else p
            if any(written_profile.get(key) != expected_profile.get(key)
                   for key in ("type", "version", "subtype", "method")):
                raise ValueError("Retail repack emitted a different target profile or codec")
            names = [canonical_path(name) for name in batch["entries"]]
            self.assert_catalog([expected_by_path[name] for name in names],
                                self.oracle_catalog(output, workspace, ["-mt:yes"]), dds_semantics=semantic)
            visited.extend(names)
            output.unlink()
        if len(visited) != len(set(visited)) or set(visited) != set(expected_by_path):
            raise ValueError("Retail repack did not exercise every entry exactly once")
        after = archive.stat()
        if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
            raise ValueError("Retail archive changed during comparison")
        return {"entries": len(expected_by_path), "batches": len(repacked["archives"]),
                "source_profile": p, "output_target": target, "oracle_exceptions": exceptions,
                "oracle_compared_entries": len(expected), "repack_verified_entries": len(visited)}


def run(args):
    """Execute all requested comparisons, retaining complete non-success verdicts."""
    from synthetic import cases
    if args.optional and not Path(args.oracle).is_file():
        print("SKIP: local BSArch executable is not supplied")
        return 77
    report = {"schema": SCHEMA, "mode": args.mode, "diagnostic": args.diagnostic, "started": time.time(),
              "source": source_identity(), "cases": [], "errors": []}
    try:
        if args.mode == "thorough" and not args.diagnostic:
            if not report["source"]["clean"]:
                raise ValueError("Strict release checking requires a clean checkout; use --diagnostic for development")
            # Reconfiguration and a successful dependency-aware build establish source
            # provenance; merely hashing an arbitrary preexisting binary would not.
            subprocess.run(["cmake", "-S", str(ROOT), "-B", args.build_dir], check=True)
            subprocess.run(["cmake", "--build", args.build_dir, "--config", "Release",
                            "--target", "libbsa_compat_bridge"], check=True)
        report["build"] = build_identity(args)
        harness = Harness(args)
        report["oracle"] = harness.oracle_id
        report["oracle_failures_sha256"] = harness.failure_registry_sha256
        controlled = cases()
        if args.case:
            controlled = [case for case in controlled if args.case in case["id"]]
            if not controlled:
                raise ValueError("No controlled cases match --case")
        corpus = []
        root = Path(args.corpus).resolve()
        if args.mode == "thorough":
            corpus = inventory(root)
            baseline = read_json(args.baseline)
            if baseline.get("schema") != SCHEMA:
                raise ValueError("Unsupported retail baseline schema")
            report["baseline_sha256"] = sha256(Path(args.baseline))
            report["baseline_path"] = str(Path(args.baseline).resolve())
            report["corpus_root"] = str(root)
            report["corpus"] = corpus
            report["errors"].extend(verify_baseline(baseline, corpus))
        report["expected_cases"] = len(controlled) + len(corpus)
        jobs = [("synthetic", c["id"], c) for c in controlled]
        jobs.extend(("retail", r["path"], r) for r in corpus)
        for kind, identity, item in jobs:
            started = time.monotonic()
            result = {"kind": kind, "id": identity, "status": "failed"}
            print(f"[{len(report['cases']) + 1}/{len(jobs)}] {kind}: {identity}", flush=True)
            try:
                # TemporaryDirectory only removes the uniquely created child, never the corpus.
                with tempfile.TemporaryDirectory(prefix="libbsa-compat-", dir=harness.scratch) as tmp:
                    workspace = Path(tmp)
                    details = (harness.synthetic_case(item, workspace) if kind == "synthetic"
                               else harness.retail_case(item, root, workspace))
                    result.update(details, status="accepted_oracle_failure" if details.get("oracle_exceptions") else "passed")
                    if details.get("oracle_exceptions"):
                        print("  ACCEPTED ORACLE FAILURE: " + ", ".join(e["id"] for e in details["oracle_exceptions"]), flush=True)
            except (Exception,) as error:
                result["error"] = str(error)
                result["phase"] = harness.phase
                print(f"  FAILED: {error}", flush=True)
            result["seconds"] = round(time.monotonic() - started, 3)
            report["cases"].append(result)
            write_json(args.report, report)
        if source_identity() != report["source"]:
            report["errors"].append("Source checkout changed during the run")
        if build_identity(args) != report["build"]:
            report["errors"].append("Tested build changed during the run")
        if sha256(harness.oracle) != report["oracle"]["sha256"]:
            report["errors"].append("Oracle changed during the run")
        if sha256(ORACLE_FAILURES) != report["oracle_failures_sha256"]:
            report["errors"].append("Oracle failure reviews changed during the run")
    except Exception as error:
        report["errors"].append(str(error))
    report["finished"] = time.time()
    passed = bool(report["cases"]) and not report["errors"] and all(
        c["status"] in ("passed", "accepted_oracle_failure") for c in report["cases"])
    report["status"] = ("passed_with_exceptions" if any(c["status"] == "accepted_oracle_failure" for c in report["cases"]) else "passed") if passed else "failed"
    report["release_errors"] = release_errors(report)
    report["release_eligible"] = not report["release_errors"]
    write_json(args.report, report)
    counts = {state: sum(c["status"] == state for c in report["cases"])
              for state in ("passed", "accepted_oracle_failure", "failed")}
    # This companion artifact intentionally excludes retail names, paths, headers and hashes.
    write_json(Path(args.report).with_suffix(".summary.json"),
               {"schema": SCHEMA, "source_commit": report["source"]["commit"],
                "mode": args.mode, "counts": counts, "status": report["status"],
                "release_eligible": report["release_eligible"],
                "lane": report.get("build", {}).get("lane")})
    print(json.dumps({"counts": counts, "errors": report["errors"],
                      "release_eligible": report["release_eligible"]}), flush=True)
    return 0 if passed and (args.mode != "thorough" or args.diagnostic or report["release_eligible"]) else 1


def verify_release(paths):
    """Require fresh, matching successful thorough reports for both public library lanes."""
    reports = [read_json(path) for path in paths]
    errors = []
    current = source_identity()
    if len(reports) != 2 or {r.get("build", {}).get("lane") for r in reports} != {"static", "shared"}:
        errors.append("Exactly one static and one shared report are required")
    for report in reports:
        errors.extend(release_errors(report))
        if report.get("source") != current:
            errors.append("Report does not describe the current source checkout")
        build = report.get("build", {})
        cache = Path(build.get("cmake_cache_path", ""))
        if not cache.is_file() or sha256(cache) != build.get("cmake_cache_sha256"):
            errors.append("Build configuration is absent or changed")
        for name, expected in build.get("dependencies", {}).items():
            if sha256(ROOT / name) != expected:
                errors.append("Dependency configuration changed")
        for item in [build.get("bridge", {}), build.get("library", {}), *build.get("runtime_dlls", [])]:
            path = Path(item.get("path", ""))
            if not path.is_file() or sha256(path) != item.get("sha256"):
                errors.append("Tested binary is absent or changed")
        oracle = Path(report.get("oracle", {}).get("path", ""))
        if not oracle.is_file() or sha256(oracle) != read_json(HERE / "oracle.json")["sha256"]:
            errors.append("Current oracle does not match the committed identity")
        baseline = Path(report.get("baseline_path", ""))
        if not baseline.is_file() or sha256(baseline) != report.get("baseline_sha256"):
            errors.append("Required corpus baseline is absent or changed")
    if len(reports) == 2:
        for key in ("source", "oracle", "baseline_sha256", "corpus", "oracle_failures_sha256"):
            if reports[0].get(key) != reports[1].get(key):
                errors.append(f"Release lane evidence differs: {key}")
    if reports and not errors:
        current_corpus = inventory(Path(reports[0]["corpus_root"]))
        if current_corpus != reports[0]["corpus"]:
            errors.append("Current corpus differs from the tested corpus")
    print("Release compatibility gate PASSED" if not errors else "Release compatibility gate FAILED: " + "; ".join(errors))
    return int(bool(errors))


def main(argv=None):
    """Expose explicit enrollment, optional synthetic checks, and strict release commands."""
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="mode", required=True)
    corpus_default = os.environ.get("LIBBSA_GAME_FIXTURES", str(ROOT / "tests/fixtures/local"))
    enrollment = commands.add_parser("enroll", help="Explicitly fingerprint a required corpus baseline")
    enrollment.add_argument("--corpus", default=corpus_default)
    enrollment.add_argument("--output", required=True)
    verifier = commands.add_parser("verify-release", help="Verify both strict release lane reports")
    verifier.add_argument("reports", nargs=2)
    for mode in ("synthetic", "thorough"):
        sub = commands.add_parser(mode)
        sub.add_argument("--bridge", required=True)
        sub.add_argument("--library", required=True)
        sub.add_argument("--build-dir", required=True)
        sub.add_argument("--report", required=True)
        sub.add_argument("--oracle", default=os.environ.get("LIBBSA_BSARCH", str(ROOT / "tests/fixtures/oracle/BSArch.exe")))
        sub.add_argument("--corpus", default=corpus_default)
        sub.add_argument("--baseline", default=str(HERE / "retail-baseline.json"))
        sub.add_argument("--scratch", default=os.environ.get("LIBBSA_COMPAT_SCRATCH", str(ROOT / "build/compat-work")))
        sub.add_argument("--cache", default=str(ROOT / "build/compat-cache"))
        sub.add_argument("--timeout", type=float, default=3600)
        sub.add_argument("--memory-gib", type=int, default=8)
        sub.add_argument("--scratch-gib", type=int, default=64)
        sub.add_argument("--batch-mib", type=int, default=256)
        sub.add_argument("--workers", type=int, default=4)
        sub.add_argument("--optional", action="store_true")
        sub.add_argument("--diagnostic", action="store_true", help="Run dirty checkout diagnostics; never qualifies release evidence")
        sub.add_argument("--case", help="Select synthetic case IDs by substring; never allowed in the thorough gate")
    args = parser.parse_args(argv)
    try:
        if args.mode == "enroll":
            records = inventory(Path(args.corpus))
            if not records:
                raise ValueError("Cannot enroll an empty corpus")
            write_json(args.output, {"schema": SCHEMA, "provenance": "Local retail archive identities only; no payload bytes", "archives": records})
            return 0
        if args.mode == "verify-release":
            return verify_release(args.reports)
        for value in (args.timeout, args.memory_gib, args.scratch_gib, args.batch_mib, args.workers):
            if value <= 0:
                raise ValueError("Resource limits and worker counts must be positive")
        if args.optional and args.mode != "synthetic":
            raise ValueError("The strict corpus gate cannot be optional")
        if args.case and args.mode != "synthetic":
            raise ValueError("The thorough gate cannot filter coverage")
        return run(args)
    except Exception as error:
        print(f"Compatibility runner failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
