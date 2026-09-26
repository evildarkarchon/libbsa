"""Small, author-owned interoperability inputs independent of libbsa's implementation.

Adapted archives are explicitly separate evidence from native BSArch output. This
module uses no libbsa parser, writer, hashes, path utilities, or DDS utilities.
"""

import hashlib
import struct
from pathlib import Path


_TARGETS = {
    "tes3": ("tes3", 0x100, ""),
    "bsa103": ("tes4", 103, ""),
    "bsa104": ("fo3", 104, ""),
    "bsa105": ("sse", 105, ""),
    "gnrl1": ("fo4", 1, "GNRL"),
    "gnrl2": ("sf1", 2, "GNRL"),
    "gnrl3-zlib": ("sf1", 3, "GNRL"),
    "gnrl3-lz4": ("sf1", 3, "GNRL"),
    "dx101": ("fo4dds", 1, "DX10"),
    "dx102": ("sf1dds", 2, "DX10"),
    "dx103-zlib": ("sf1dds", 3, "DX10"),
    "dx103-lz4": ("sf1dds", 3, "DX10"),
}


def cases() -> list[dict]:
    """Return a bounded deterministic matrix covering controls and their interactions.

    Oracle compression choices establish the required native archive profile;
    they need not reproduce libbsa's per-entry or archive-wide storage choices.
    Both directions compare decoded content rather than compressor output.
    """
    result = []
    variants = (
        ("serial", False, 1, "default", False, 0),
        ("parallel-shared", True, 4, "default", False, 0),
        ("raw-shared", True, 1, "raw", False, 65536),
        ("compressed-parallel", False, 4, "compressed", False, 16384),
        ("mixed-serial", True, 1, "default", True, 65536),
        ("mixed-parallel", True, 4, "compressed", True, 16384),
    )
    for target, (oracle_target, version, subtype) in _TARGETS.items():
        target_variants = variants
        if subtype == "DX10":
            target_variants = (
                ("serial", False, 1, "default", False, 0),
                ("parallel-shared", True, 4, "default", False, 0),
                ("serial-shared-small-chunks", True, 1, "default", False, 16384),
                ("parallel-small-chunks", False, 4, "default", False, 16384),
                ("serial-large-chunks", False, 1, "default", False, 65536),
                ("parallel-shared-large-chunks", True, 4, "default", False, 65536),
            )
        for name, sharing, workers, compression, mixed, chunk_bytes in target_variants:
            if target == "tes3" and name != "serial":
                continue
            item = {
                "id": f"{target}-{name}", "target": target,
                "oracle_target": oracle_target,
                "options": {"sharing": sharing, "workers": workers,
                            "compression": "default" if subtype == "DX10" else compression,
                            "embedded_names": mixed and target in ("bsa104", "bsa105"),
                            "chunk_bytes": chunk_bytes if subtype == "DX10" else 0},
                "mixed_entries": mixed,
                "expected_profile": {"version": version, "subtype": subtype},
            }
            if target.endswith("3-zlib"):
                item["adaptation"] = target
            item["oracle_args"] = oracle_arguments(item)
            result.append(item)
    # FO4 next-generation headers have the v1 layout. These recipes exercise
    # readers only; pretending a native BSArch v7/v8 producer exists would erase
    # a material distinction in the independent evidence.
    for subtype, prefix, oracle_target in (("GNRL", "gnrl", "fo4"),
                                            ("DX10", "dx10", "fo4dds")):
        for version in (7, 8):
            target = f"{prefix}{version}"
            item = {"id": f"{target}-adapted-reader", "target": target,
                    "oracle_target": oracle_target, "reader_only": True,
                    "adaptation": target, "mixed_entries": False,
                    "options": {"sharing": False, "workers": 1,
                                "compression": "default" if subtype == "DX10" else "compressed",
                                "embedded_names": False,
                                "chunk_bytes": 0},
                    "expected_profile": {"version": version, "subtype": subtype}}
            item["oracle_args"] = oracle_arguments(item)
            result.append(item)
    return result


def oracle_arguments(case: dict) -> list[str]:
    """Return explicit BSArch packing controls, excluding source and destination.

    The released CLI cannot select v3 zlib directly, so those cases first use
    native v2 zlib. Texture v2 requires an explicit zlib codec. GNRL v3 LZ4
    requires its explicit codec even when the libbsa direction requests raw
    storage; the two directions still compare the same decoded content.
    """
    target, options = case["target"], case["options"]
    oracle_target = case.get("oracle_target", _TARGETS.get(target, (None,))[0])
    if oracle_target is None:
        raise ValueError(f"unknown oracle target: {target}")
    args = [f"-{oracle_target}",
            "-share:yes" if options["sharing"] else "-share:no",
            "-mt:yes" if options["workers"] > 1 else "-mt:no", "-split:0"]
    if target != "tes3":
        if target.endswith("-lz4"):
            args.append("-z:lz4")
        elif target in ("dx102", "dx103-zlib"):
            args.append("-z:zlib")
        elif options["compression"] != "raw":
            args.append("-z:zlib" if target.endswith("3-zlib") else "-z")
        if options.get("embedded_names"):
            # Explicit flags preserve path/name tables and the normal BSA
            # retain-names flag while enabling embedded payload names.
            flags = 0x183 | (0 if options["compression"] == "raw" else 4)
            args.append(f"-af:0x{flags:x}")
    return args


def _noise(label: str, size: int) -> bytes:
    """Derive reproducible high-entropy bytes without an implementation-dependent PRNG."""
    return hashlib.shake_256(label.encode("ascii")).digest(size)


def _dds(label: str, *, format_code: int = 71, cube: bool = False) -> bytes:
    """Build a 128-square, eight-mip BC texture with complete face-major data.

    BC1 uses the legacy DXT1 header; other formats use a DX10 2D header. Each
    subresource has distinguishable deterministic blocks, including all tiny
    mips which still occupy one full BC block. No image decoder is involved.
    """
    block_size = 8 if format_code in (71, 72) else 16
    header = bytearray(128)
    struct.pack_into("<4sIIIIIII", header, 0, b"DDS ", 124, 0xA1007,
                     128, 128, 1024 * block_size, 0, 8)
    struct.pack_into("<II4s", header, 76, 32, 4,
                     b"DXT1" if format_code == 71 else b"DX10")
    struct.pack_into("<II", header, 108, 0x401008, 0xFE00 if cube else 0)
    if format_code != 71:
        header.extend(struct.pack("<IIIII", format_code, 3, 4 if cube else 0, 1, 0))
    payload = bytearray()
    for face in range(6 if cube else 1):
        for mip in range(8):
            dimension = max(1, 128 >> mip)
            blocks = max(1, (dimension + 3) // 4) ** 2
            block = bytearray(_noise(f"{label}:face{face}:mip{mip}", block_size))
            # Select a defined BC7 mode instead of a reserved all-zero prefix;
            # BC6 mode 00 is also defined. Tests compare blocks, but valid block
            # encodings keep these author-owned textures useful outside tests.
            if format_code in (98, 99):
                block[0] = (block[0] & 0x80) | 0x40
            elif format_code in (95, 96):
                block[0] &= 0xFC
            payload.extend(block * blocks)
    return bytes(header + payload)


def prepare_case(case: dict, directory) -> list[dict]:
    """Write small synthetic source files and return explicit archive-path entries.

    Source files use host separators; entry paths deliberately vary slash and
    case spelling. The caller owns a fresh disposable directory and its cleanup.
    No existing file is overwritten. Returned sources are absolute strings.
    """
    root = Path(directory).resolve()
    texture = case["expected_profile"]["subtype"] == "DX10"
    if texture:
        bc1 = _dds("bc1-chain")
        recipes = [("Textures/MixedCase/chain.dds", bc1),
                   ("textures\\duplicate\\copy.dds", bc1),
                   ("textures/cube.dds", _dds("cube", cube=True)),
                   ("textures/bc7.dds", _dds("bc7", format_code=98))]
        if case["target"] in ("dx102", "dx103-zlib", "dx103-lz4"):
            recipes.extend([("textures/bc6-unsigned.dds", _dds("bc6u", format_code=95)),
                            ("textures/bc6-signed.dds", _dds("bc6s", format_code=96)),
                            ("textures/bc1-srgb.dds", _dds("bc1srgb", format_code=72)),
                            ("textures/bc7-srgb.dds", _dds("bc7-srgb", format_code=99))])
    else:
        repeated = b"Author-owned independent archive interoperability input.\n" * 1024
        random_bytes = _noise("incompressible-payload", 32768)
        recipes = [("Meshes/MixedCase/repeated.nif", repeated),
                   ("meshes\\duplicate\\copy.nif", repeated),
                   ("meshes/distinct.nif", repeated[:-1] + b"!"),
                   ("misc/noise.bin", random_bytes),
                   ("misc/noise-copy.bin", random_bytes),
                   ("misc/tiny.bin", b"\x00\xff\x7f"),
                   ("misc/empty.bin", b""),
                   ("strings/oracle.strings", b"Independent strings test input.")]
        if case["target"].startswith("gnrl"):
            recipes.append(("scripts/nested/long-extension.extension", b"Long extension metadata probe."))
    entries = []
    for index, (archive_path, data) in enumerate(recipes):
        source = root.joinpath(*archive_path.replace("\\", "/").split("/"))
        source.parent.mkdir(parents=True, exist_ok=True)
        with source.open("xb") as stream:
            stream.write(data)
        entry = {"path": archive_path, "source": str(source)}
        # DX10's public writer routes compression by profile and does not expose
        # the archive/per-entry overrides available to general archives.
        if case.get("mixed_entries") and case["target"] != "tes3" and not texture:
            entry["compression"] = ("raw", "compressed", "inherit")[index % 3]
        entries.append(entry)
    return entries


def adapt_archive(source, destination, target: str) -> None:
    """Relocate a bounded independent BSArch fixture to a missing native profile.

    Supports v2→v3 method-0 zlib for GNRL/DX10 and v1→FO4 v7/v8. All absolute
    payload/chunk offsets and the filename-table offset are preserved relative
    to the relocated data. Source files remain untouched. Invalid profiles,
    truncated tables, out-of-bounds data, or unsafe destinations raise ValueError
    before any output is written. These are adapted, never native, oracle bytes.
    """
    source, destination = Path(source).resolve(), Path(destination).resolve()
    if source == destination:
        raise ValueError("source and destination must be different files")
    if source.stat().st_size > 64 * 1024 * 1024:
        raise ValueError("adaptation is limited to small synthetic archives")
    original = source.read_bytes()
    if len(original) < 24 or original[:4] != b"BTDX":
        raise ValueError("source is not a complete BA2 header")
    version, subtype, count, names = struct.unpack_from("<I4sIQ", original, 4)
    targets = {"gnrl3-zlib": (2, b"GNRL", 3, 4),
               "dx103-zlib": (2, b"DX10", 3, 4),
               "gnrl7": (1, b"GNRL", 7, 0), "gnrl8": (1, b"GNRL", 8, 0),
               "dx107": (1, b"DX10", 7, 0), "dx108": (1, b"DX10", 8, 0)}
    if target not in targets:
        raise ValueError(f"unsupported adaptation target: {target}")
    expected_version, expected_subtype, new_version, shift = targets[target]
    if version != expected_version or subtype != expected_subtype:
        raise ValueError("source profile does not match adaptation recipe")
    header_size = 32 if version == 2 else 24
    if len(original) < header_size or not header_size <= names <= len(original):
        raise ValueError("source header or filename table is out of bounds")
    offsets = []
    cursor = header_size
    for _ in range(count):
        record_size = 36 if subtype == b"GNRL" else 24
        if cursor + record_size > names:
            raise ValueError("source entry table is truncated")
        if subtype == b"GNRL":
            offsets.append(cursor + 16)
            cursor += 36
        else:
            chunks = original[cursor + 13]
            chunk_size = struct.unpack_from("<H", original, cursor + 14)[0]
            if chunks == 0 or chunk_size != 24:
                raise ValueError("source texture chunk layout is unsupported")
            cursor += 24
            if cursor + 24 * chunks > names:
                raise ValueError("source chunk table is truncated")
            offsets.extend(cursor + 24 * index for index in range(chunks))
            cursor += 24 * chunks
    for location in offsets:
        offset, packed, unpacked = struct.unpack_from("<QII", original, location)
        size = packed or unpacked
        # Shared regions are legal; independent relocation must move every
        # referring record, rather than deduplicating offset fields themselves.
        if offset < cursor or offset + size > names:
            raise ValueError("source payload range is outside the data section")
    name_cursor = names
    for _ in range(count):
        if name_cursor + 2 > len(original):
            raise ValueError("source filename length is truncated")
        size = struct.unpack_from("<H", original, name_cursor)[0]
        name_cursor += 2 + size
        if name_cursor > len(original):
            raise ValueError("source filename is truncated")
    result = bytearray(original[:32] + b"\x00" * 4 + original[32:]) if shift else bytearray(original)
    struct.pack_into("<I", result, 4, new_version)
    struct.pack_into("<Q", result, 16, names + shift)
    for location in offsets:
        offset = struct.unpack_from("<Q", original, location)[0]
        struct.pack_into("<Q", result, location + shift, offset + shift)
    destination.write_bytes(result)
