"""Independent, bounded file fingerprints for the optional compatibility runner.

No libbsa or DirectXTex code participates in comparison. DDS normalization follows
Microsoft's DDS_HEADER, DDS_PIXELFORMAT, DDS_HEADER_DXT10 and DXGI_FORMAT docs:
https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header
https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header-dxt10
https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-pixelformat
https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format

Only equivalent envelopes are normalized. Unsupported layouts are explicit
comparison failures, never raw-hash successes masquerading as texture proof.
"""

import hashlib
from pathlib import Path
import re
import struct
from types import MappingProxyType


_ASCII_LOWER = str.maketrans("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "abcdefghijklmnopqrstuvwxyz")
_FOURCC = MappingProxyType({
    b"DXT1": (71, 0), b"DXT2": (74, 2), b"DXT3": (74, 0),
    b"DXT4": (77, 2), b"DXT5": (77, 0), b"ATI1": (80, 0),
    b"BC4U": (80, 0), b"BC4S": (81, 0), b"ATI2": (83, 0),
    b"BC5U": (83, 0), b"BC5S": (84, 0),
})
# Exact flag/mask tuples prevent a channel-swizzled texture from normalizing to
# the same DXGI format merely because it has the same number of bits per pixel.
_MASK_FORMAT = MappingProxyType({
    (0x40, 24, 0xFF, 0xFF00, 0xFF0000, 0): "legacy_rgb24",
    (0x40, 24, 0xFF0000, 0xFF00, 0xFF, 0): "legacy_bgr24",
    (0x41, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000): 28,
    (0x41, 32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000): 87,
    (0x40, 32, 0xFF0000, 0xFF00, 0xFF, 0): 88,
    (0x41, 32, 0x3FF, 0xFFC00, 0x3FF00000, 0xC0000000): 24,
    (0x40, 16, 0xF800, 0x7E0, 0x1F, 0): 85,
    (0x41, 16, 0x7C00, 0x3E0, 0x1F, 0x8000): 86,
    (0x41, 16, 0xF00, 0xF0, 0xF, 0xF000): 115,
    (0x20000, 8, 0xFF, 0, 0, 0): 61,
    (0x20000, 16, 0xFFFF, 0, 0, 0): 56,
    (0x20001, 16, 0xFF, 0, 0, 0xFF00): 49,
    (0x2, 8, 0, 0, 0, 0xFF): 65,
})


def canonical_path(path: str) -> str:
    """Normalize an archive key using ASCII folding and separators, rejecting aliases.

    This never applies host filesystem resolution or Unicode case folding. Empty,
    rooted, dot-component and stream-like keys raise ValueError.
    """
    if not isinstance(path, str) or not path or "\x00" in path or ":" in path:
        raise ValueError("invalid archive path")
    normalized = path.replace("\\", "/").translate(_ASCII_LOWER)
    if any(part in ("", ".", "..") for part in normalized.split("/")):
        raise ValueError(f"invalid archive path {path!r}")
    return normalized


def fingerprint_file(path: Path, archive_path: str) -> dict:
    """Hash a file in bounded chunks and capture DDS evidence for bridge comparison.

    The two suffix hashes cover bytes after offsets 128 and 148, respectively;
    comparable selects the actual header boundary. The original archive spelling
    is retained so duplicate keys can be diagnosed before canonicalization.
    """
    canonical_path(archive_path)
    whole = hashlib.sha256()
    suffix128 = hashlib.sha256()
    suffix148 = hashlib.sha256()
    header = bytearray()
    size = 0
    with Path(path).open("rb") as source:
        while chunk := source.read(1024 * 1024):
            whole.update(chunk)
            if len(header) < 148:
                header.extend(chunk[:148 - len(header)])
            suffix128.update(chunk[max(0, 128 - size):])
            suffix148.update(chunk[max(0, 148 - size):])
            size += len(chunk)
    result = {"path": archive_path, "size": size, "sha256": whole.hexdigest()}
    if header[:4] == b"DDS ":
        # Retain format metadata only: the twenty bytes following a legacy
        # header are pixel data, not the optional DX10 extension.
        boundary = 148 if header[84:88] == b"DX10" else 128
        result.update(dds_header_hex=header[:boundary].hex(),
                      payload_sha256_128=suffix128.hexdigest(),
                      payload_sha256_148=suffix148.hexdigest())
    return result


def _digest(record: dict, field: str) -> str:
    """Require a complete SHA-256 wire value instead of substituting missing evidence."""
    value = record.get(field)
    if not isinstance(value, str) or re.fullmatch(r"[0-9a-fA-F]{64}", value) is None:
        raise ValueError(f"missing or invalid {field}")
    return value.lower()


def _surface(format_id: int | str, width: int, height: int) -> tuple[int, int]:
    """Return tight row and 2D slice bytes for explicitly understood DXGI formats."""
    if format_id in ("legacy_rgb24", "legacy_bgr24"):
        # DDS has valid 24-bit legacy masks with no equivalent DXGI format.
        # Preserve those identities rather than inventing an alpha channel.
        return width * 3, width * height * 3
    if 70 <= format_id <= 84 or 94 <= format_id <= 99:
        block = 8 if format_id in (70, 71, 72, 79, 80, 81) else 16
        row = max(1, (width + 3) // 4) * block
        return row, row * max(1, (height + 3) // 4)
    if 1 <= format_id <= 4:
        bits = 128
    elif 5 <= format_id <= 8:
        bits = 96
    elif 9 <= format_id <= 22:
        bits = 64
    elif 23 <= format_id <= 47 or format_id in (67, 87, 88, 89, 90, 91, 92, 93):
        bits = 32
    elif 48 <= format_id <= 59 or format_id in (85, 86, 115):
        bits = 16
    elif 60 <= format_id <= 65:
        bits = 8
    elif format_id == 66:
        bits = 1
    elif format_id in (68, 69):
        row = ((width + 1) // 2) * 4
        return row, row * height
    else:
        # Planar/video formats require a plane-aware interpretation. Guessing a
        # bits-per-pixel value would incorrectly certify their byte ordering.
        raise ValueError(f"unsupported DDS DXGI format {format_id}")
    row = (width * bits + 7) // 8
    return row, row * height


def _dds(record: dict, size: int) -> tuple[dict, int]:
    """Parse DDS semantic identity and bounded ordered subresource geometry.

    Reserved fields and redundant pitch/linear-size hints do not define texture
    identity. Mip geometry and exact payload length independently establish the
    layout; padded uncompressed rows are deliberately unsupported.
    """
    encoded = record.get("dds_header_hex")
    if not isinstance(encoded, str) or len(encoded) > 296:
        raise ValueError("missing or invalid DDS header evidence")
    try:
        header = bytes.fromhex(encoded)
    except ValueError as error:
        raise ValueError("invalid DDS header hex") from error
    if size < 128 or len(header) < 128 or header[:4] != b"DDS ":
        raise ValueError("truncated or invalid DDS header")
    words = struct.unpack_from("<31I", header, 4)
    if words[0] != 124 or words[18] != 32:
        raise ValueError("invalid DDS structure size")
    flags, height, width, pitch, depth, count = words[1:7]
    pixel_flags, fourcc, bits, red, green, blue, alpha_mask = words[19:26]
    caps2 = words[27]
    if width == 0 or height == 0:
        raise ValueError("invalid zero DDS dimensions")
    if caps2 & ~(0xFE00 | 0x200000):
        raise ValueError("unsupported DDS caps2 flags")
    code = struct.pack("<I", fourcc)
    alpha = 0
    layers = 1
    cube = bool(caps2 & 0x200)
    dimension = 4 if caps2 & 0x200000 else 3
    header_size = 128
    if pixel_flags & 4:
        if pixel_flags & ~5:
            raise ValueError("unsupported DDS FOURCC flags")
        if code == b"DX10":
            if size < 148 or len(header) < 148:
                raise ValueError("truncated DDS DX10 header")
            format_id, dimension, misc, array_size, alpha = struct.unpack_from("<5I", header, 128)
            if dimension not in (2, 3, 4) or array_size == 0:
                raise ValueError("invalid DDS dimension or array size")
            if misc & ~4 or alpha > 4:
                raise ValueError("unsupported DDS resource or alpha flags")
            cube = bool(misc & 4)
            if caps2 & 0xFE00 and not cube:
                raise ValueError("contradictory DDS cube flags")
            if caps2 & 0x200000 and dimension != 4:
                raise ValueError("contradictory DDS volume flags")
            if dimension == 4 and array_size != 1:
                raise ValueError("DDS volumes cannot be arrays")
            layers = array_size * (6 if cube else 1)
            header_size = 148
        else:
            if code not in _FOURCC:
                raise ValueError(f"unsupported DDS FOURCC {code!r}")
            format_id, alpha = _FOURCC[code]
    else:
        key = (pixel_flags, bits, red, green, blue, alpha_mask)
        if key not in _MASK_FORMAT:
            raise ValueError(f"unsupported DDS pixel masks {key!r}")
        format_id = _MASK_FORMAT[key]

    if cube:
        if dimension != 3 or width != height:
            raise ValueError("DDS cubes must be square 2D textures")
        if header_size == 128:
            if caps2 & 0xFC00 != 0xFC00:
                raise ValueError("unsupported partial DDS cube faces")
            layers = 6
    elif caps2 & 0xFC00:
        raise ValueError("DDS cube faces set without cube flag")
    if dimension == 2 and height != 1:
        raise ValueError("DDS 1D textures require height one")
    if dimension == 4:
        if not flags & 0x800000 or depth == 0:
            raise ValueError("DDS volume requires nonzero depth and depth flag")
    else:
        if depth > 1 or flags & 0x800000:
            raise ValueError("non-volume DDS carries volume depth")
        depth = 1

    # Microsoft advises readers not to depend on every redundant DDS caps flag.
    # The actual dimensions/count and full byte extent are checked instead.
    mip_count = count or 1
    if mip_count > max(width, height, depth).bit_length():
        raise ValueError("DDS mip count exceeds the full dimension chain")
    mips = []
    total = 0
    for level in range(mip_count):
        w, h, d = max(1, width >> level), max(1, height >> level), max(1, depth >> level)
        row, slice_bytes = _surface(format_id, w, h)
        if level == 0 and flags & 8 and pitch not in (0, row):
            raise ValueError("unsupported DDS padded or inconsistent row pitch")
        byte_count = slice_bytes * d
        mips.append({"width": w, "height": h, "depth": d,
                     "row_bytes": row, "slice_bytes": slice_bytes, "bytes": byte_count})
        total += byte_count
    if size - header_size != total * layers:
        raise ValueError(f"DDS payload size mismatch: expected {total * layers}, got {size - header_size}")
    # DDS stores the mip chain for each layer/face in order. A repeated chain
    # descriptor is exact and avoids allocating attacker-controlled arraySize rows.
    return {"format": format_id, "dimension": dimension, "cube": cube,
            "layers": layers, "alpha_mode": alpha, "mips": mips}, header_size


def comparable(record: dict, *, dds_semantics: bool = True) -> dict:
    """Validate a file/bridge record and return its independent comparison identity.

    Ordinary files compare exact size and SHA-256. DDS compares all understood
    semantic metadata, ordered subresource geometry and SHA-256 of the complete
    ordered pixel payload. Set dds_semantics=False for opaque BSA/GNRL entry
    bytes, whose DDS envelopes must be preserved exactly. Invalid/missing
    evidence raises ValueError.
    """
    if not isinstance(record, dict):
        raise ValueError("fingerprint record must be an object")
    path = canonical_path(record.get("path"))
    size = record.get("size")
    if type(size) is not int or size < 0:
        raise ValueError("missing or invalid file size")
    digest = _digest(record, "sha256")
    if dds_semantics and (path.endswith(".dds") or "dds_header_hex" in record):
        metadata, boundary = _dds(record, size)
        return {"path": path, "dds": metadata,
                "payload_sha256": _digest(record, f"payload_sha256_{boundary}")}
    return {"path": path, "size": size, "sha256": digest}


def compare_catalogs(expected: list, actual: list, *, dds_semantics: bool = True) -> list[str]:
    """Compare complete catalogs, returning every missing/extra/invalid/different key.

    Duplicate records are errors even if equal or canonically aliased. No entry
    failure is converted into a skip or hidden by dictionary replacement.
    """
    errors = []
    catalogs = []
    for label, records in (("expected", expected), ("actual", actual)):
        catalog = {}
        seen = set()
        if not isinstance(records, list):
            errors.append(f"{label} catalog must be a list")
            catalogs.append(catalog)
            continue
        for index, record in enumerate(records):
            try:
                if not isinstance(record, dict):
                    raise ValueError("fingerprint record must be an object")
                path = canonical_path(record.get("path"))
                if path in seen:
                    errors.append(f"{label} duplicate archive path: {path}")
                seen.add(path)
                identity = comparable(record, dds_semantics=dds_semantics)
                catalog[path] = identity
            except ValueError as error:
                key = record.get("path", index) if isinstance(record, dict) else index
                errors.append(f"{label} invalid entry {key!r}: {error}")
        catalogs.append(catalog)
    left, right = catalogs
    errors.extend(f"missing actual entry: {key}" for key in sorted(left.keys() - right.keys()))
    errors.extend(f"extra actual entry: {key}" for key in sorted(right.keys() - left.keys()))
    for key in sorted(left.keys() & right.keys()):
        if left[key] != right[key]:
            differing = sorted(field for field in left[key].keys() | right[key].keys()
                               if left[key].get(field) != right[key].get(field))
            errors.append(f"different entry {key}: {', '.join(differing)}")
    return errors
