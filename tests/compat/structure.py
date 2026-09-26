"""Independent metadata-only structural inspection of written BSA/BA2 archives.

The reference is read-only TES5Edit/Core/wbBSArchive.pas: CreateHashTES3,
CreateHashTES4, CreateHashFO4, LoadFromFile and Save. No libbsa parsing, hashing,
or validation implementation is imported. Compression streams and DDS texel
interpretation are deliberately outside this inspector's claims; the runner's
independent extraction/fingerprint comparison supplies that evidence.
"""

from pathlib import Path
import struct
from types import MappingProxyType
import zlib


_BA2_HEADERS = MappingProxyType({1: 24, 2: 32, 3: 36, 7: 24, 8: 24})
_MASK32 = 0xFFFFFFFF


def _ba2_hash(name: bytes) -> int:
    """Compute the zero-initialized, uncomplemented Bethesda CRC lookup hash.

    The reference skips high bytes, folds ASCII, and maps slash to backslash.
    Python's CRC interface complements its state, so both interface boundaries
    are explicitly inverted to obtain the on-disk variant.
    """
    folded = name.lower().replace(b"/", b"\\")
    ascii_bytes = bytes(value for value in folded if value < 128)
    return zlib.crc32(ascii_bytes, _MASK32) ^ _MASK32


def _tes3_hash(name: bytes) -> int:
    """Compose TES3's first-half XOR and second-half rotating XOR as a uint64.

    ASCII case folds but separators do not. The composed integer determines sort
    order; serialization stores its high u32 first, unlike an ordinary LE u64.
    """
    name = name.lower()
    split = len(name) // 2
    first = 0
    for index, value in enumerate(name[:split]):
        first ^= (value << ((index * 8) & 31)) & _MASK32
    second = 0
    for index, value in enumerate(name[split:]):
        shifted = (value << ((index * 8) & 31)) & _MASK32
        second ^= shifted
        rotation = shifted & 31
        if rotation:
            second = ((second >> rotation) | (second << (32 - rotation))) & _MASK32
    return (first << 32) | second


def _tes4_hash(stem: bytes, extension: bytes = b"") -> int:
    """Compute the TES4 filename/folder hash, retaining extension-specific low bits."""
    stem, extension = stem.lower(), extension.lower()
    if not stem:
        return 0
    low = stem[-1] | (len(stem) << 16) | (stem[0] << 24)
    if len(stem) > 2:
        low |= stem[-2] << 8
    low |= {b".kf": 0x80, b".nif": 0x8000, b".dds": 0x8080,
            b".wav": 0x80000000}.get(extension[:4], 0)
    middle = 0
    for value in stem[1:-2]:
        middle = (middle * 0x1003F + value) & _MASK32
    ext_hash = 0
    for value in extension:
        ext_hash = (ext_hash * 0x1003F + value) & _MASK32
    return (((middle + ext_hash) & _MASK32) << 32) | low


def _split_extension(name: bytes) -> tuple[bytes, bytes]:
    """Split at the final dot, preserving the dot in the returned extension."""
    position = name.rfind(b".")
    return (name, b"") if position < 0 else (name[:position], name[position:])


class _Reader:
    """Read bounded metadata records without materializing archive payloads."""

    def __init__(self, source):
        """Observe archive size once through the already-open binary stream."""
        self.source = source
        source.seek(0, 2)
        self.size = source.tell()
        source.seek(0)

    def tell(self):
        return self.source.tell()

    def seek(self, offset):
        """Reject metadata seeks outside the observed archive extent."""
        if offset < 0 or offset > self.size:
            raise ValueError(f"metadata offset {offset} outside archive size {self.size}")
        self.source.seek(offset)

    def read(self, length, end=None):
        """Read exactly one bounded field within both archive and optional table bounds."""
        limit = self.size if end is None else min(end, self.size)
        offset = self.tell()
        if length < 0 or length > 65536 or offset > limit or length > limit - offset:
            raise ValueError(f"metadata range at {offset} length {length} exceeds bound {limit}")
        result = self.source.read(length)
        if len(result) != length:
            raise ValueError(f"truncated metadata at {offset}")
        return result

    def unpack(self, fields, end=None):
        """Read one explicitly little-endian fixed record."""
        return struct.unpack("<" + fields, self.read(struct.calcsize("<" + fields), end))

    def cstring(self, end):
        """Read a bounded NUL-terminated name with at most 64 KiB of scratch."""
        result = bytearray()
        while self.tell() < min(end, self.size) and len(result) < 65536:
            chunk = self.read(min(256, end - self.tell(), 65536 - len(result)), end)
            terminator = chunk.find(b"\0")
            if terminator >= 0:
                result.extend(chunk[:terminator])
                self.seek(self.tell() - len(chunk) + terminator + 1)
                return bytes(result)
            result.extend(chunk)
        raise ValueError("unterminated or unsupported overlong archive name")


class _Inspection:
    """Collect checked metadata plus strict findings or historical observations."""

    def __init__(self, reader, strict):
        """Own per-inspection state; cap diagnostics while retaining checked-entry counts."""
        self.reader = reader
        self.strict = strict
        self.result = {"profile": {"type": "unknown", "version": None,
                                   "subtype": None, "entry_count": 0},
                       "paths": [], "findings": [], "observations": [], "checks": [],
                       "stats": {"compressed_payloads": 0, "raw_payloads": 0,
                                 "shared_payload_spans": 0, "embedded_name_payloads": 0}}
        self.seen = set()
        self.omitted = {"findings": 0, "observations": 0}

    def problem(self, message, historical=False):
        """Keep established retail anomalies advisory unless certifying writer output."""
        key = "observations" if historical and not self.strict else "findings"
        if len(self.result[key]) < 1000:
            self.result[key].append(message)
        else:
            self.omitted[key] += 1

    def finish(self):
        """Make capped diagnostics explicit instead of silently discarding their count."""
        for key, count in self.omitted.items():
            if count:
                self.result[key].append(f"{count} additional {key} omitted from diagnostic detail")
        return self.result

    def name(self, raw):
        """Decode names for reports while checking ASCII-canonical path uniqueness."""
        normalized = raw.lower().replace(b"\\", b"/")
        if (not normalized or b"\0" in normalized or b":" in normalized
                or any(part in (b"", b".", b"..") for part in normalized.split(b"/"))):
            self.problem(f"invalid archive path {raw!r}")
        if normalized in self.seen:
            self.problem(f"duplicate canonical archive path {raw!r}")
        self.seen.add(normalized)
        try:
            display = normalized.decode("utf-8")
        except UnicodeDecodeError:
            # Hashing always retains original bytes. The fallback is reporting
            # only and never changes record identity or merges high-byte keys.
            display = normalized.decode("cp1252", errors="backslashreplace")
        self.result["paths"].append(display)
        return display

    def count(self, count, available, stride, label):
        """Validate count arithmetic before allocating or iterating declared records."""
        if available < 0 or count > available // stride:
            raise ValueError(f"{label} count {count} cannot fit its metadata extent")

    def spans(self, payloads, metadata, sharing=True, eligible=False):
        """Check complete span bounds, metadata intersections and exact-only sharing.

        Empty spans may sit at the filename table start, matching a writer cursor
        that does not advance for empty entries. Positive spans are sorted once;
        no archive bytes are loaded or compared here.
        """
        positive = []
        for offset, length, label, decode in payloads:
            if offset > self.reader.size or length > self.reader.size - offset:
                self.problem(f"payload {label} range {offset}+{length} exceeds archive size")
                continue
            if offset < metadata[0][1]:
                self.problem(f"payload {label} offset intersects header/record metadata")
            if length:
                for start, end in metadata:
                    if offset < end and start < offset + length:
                        self.problem(f"payload {label} overlaps metadata [{start}, {end})")
                positive.append((offset, offset + length, label, decode))
        previous = None
        shared = 0
        for current in sorted(positive, key=lambda item: (item[0], item[1])):
            if previous and current[0] < previous[1]:
                exact = current[:2] == previous[:2]
                if not exact or not sharing:
                    self.problem(f"payload overlap between {previous[2]} and {current[2]}")
                elif eligible and current[3] != previous[3]:
                    self.problem(f"shared payload has incompatible decode sizes: {previous[2]} and {current[2]}")
                else:
                    shared += 1
            if previous is None or current[1] > previous[1]:
                previous = current
        self.result["shared_payload_spans"] = shared
        self.result["stats"]["shared_payload_spans"] = shared
        self.result["checks"].append("payload bounds, metadata separation and span exclusivity")

    def ba2(self):
        """Inspect all BA2 record and filename tables, lookup identity and mip spans."""
        r = self.reader
        r.seek(4)
        version, subtype, count, names_start = r.unpack("I4sIQ")
        if version not in _BA2_HEADERS or subtype not in (b"GNRL", b"DX10"):
            raise ValueError(f"unsupported BA2 version/subtype {version}/{subtype!r}")
        header_size = _BA2_HEADERS[version]
        method = "zlib"
        profile = {"type": "ba2", "version": version, "subtype": subtype.decode("ascii"),
                   "entry_count": count, "header_size": header_size}
        if version in (2, 3):
            profile["extension_words"] = list(r.unpack("II"))
        if version == 3:
            code, = r.unpack("I")
            profile["compression_method"] = code
            method = "lz4_block" if code == 3 else "zlib"
        profile["method"] = method
        self.result["profile"] = profile
        minimum = 36 if subtype == b"GNRL" else 48
        self.count(count, r.size - header_size, minimum, "BA2 file")
        identities = []
        payloads = []
        record_markers = {}
        for index in range(count):
            label = f"record {index}"
            if subtype == b"GNRL":
                name_hash, extension, dir_hash, flags, offset, packed, raw, tail = r.unpack("I4sIIQIII")
                if tail != 0xBAADF00D:
                    self.problem(f"{label} invalid BA2 record sentinel", historical=True)
                marker = f"0x{flags:08x}"
                record_markers[marker] = record_markers.get(marker, 0) + 1
                self.result["stats"]["compressed_payloads" if packed else "raw_payloads"] += 1
                payloads.append((offset, packed or raw, label, (raw, packed, method)))
            else:
                name_hash, extension, dir_hash, flags, chunks, chunk_width, height, width, mips, dxgi, cube = r.unpack("I4sIBBHHHBBH")
                if chunk_width != 24:
                    self.problem(f"{label} invalid BA2 chunk record width {chunk_width}", historical=True)
                if not width or not height or not mips or mips > max(width, height).bit_length():
                    self.problem(f"{label} invalid DX10 dimensions or mip count")
                if chunks == 0:
                    self.problem(f"{label} has no DX10 chunks")
                self.count(chunks, r.size - r.tell(), 24, "DX10 chunk")
                next_mip = 0
                complete_chains = 0
                for chunk_index in range(chunks):
                    offset, packed, raw, first, last, tail = r.unpack("QIIHHI")
                    chunk_label = f"{label} chunk {chunk_index}"
                    if tail != 0xBAADF00D:
                        self.problem(f"{chunk_label} invalid BA2 chunk sentinel", historical=True)
                    if raw == 0:
                        self.problem(f"{chunk_label} has an empty decoded payload", historical=True)
                    if first != next_mip or last < first or last >= mips:
                        self.problem(f"{chunk_label} invalid or noncontiguous mip range {first}..{last}")
                    next_mip = last + 1
                    if next_mip == mips:
                        complete_chains += 1
                        next_mip = 0
                    self.result["stats"]["compressed_payloads" if packed else "raw_payloads"] += 1
                    payloads.append((offset, packed or raw, chunk_label, (raw, packed, method)))
                # BSArch may aggregate all faces into one mip-range sequence;
                # libbsa may serialize one complete sequence for each cube face.
                # Only the cube flag permits six sequences, never arbitrary repeats.
                allowed_chains = (1, 6) if cube & 1 else (1,)
                if next_mip != 0 or complete_chains not in allowed_chains:
                    self.problem(f"{label} chunks do not cover the complete mip chain")
            identities.append((name_hash, extension, dir_hash))
        records_end = r.tell()
        if names_start < records_end:
            raise ValueError("BA2 filename table overlaps header/record metadata")
        r.seek(names_start)
        for index, (name_hash, extension, dir_hash) in enumerate(identities):
            length, = r.unpack("H")
            raw_name = r.read(length)
            display = self.name(raw_name)
            folded = raw_name.lower().replace(b"/", b"\\")
            split = folded.rfind(b"\\")
            directory, filename = (b"", folded) if split < 0 else (folded[:split], folded[split + 1:])
            stem, ext = _split_extension(filename)
            if name_hash != _ba2_hash(stem):
                self.problem(f"{display}: BA2 filename stem hash mismatch", historical=True)
            if dir_hash != _ba2_hash(directory):
                self.problem(f"{display}: BA2 directory hash mismatch", historical=True)
            expected_ext = ext[1:5].ljust(4, b"\0")
            if extension != expected_ext:
                self.problem(f"{display}: BA2 lowercase extension FourCC mismatch", historical=True)
        names_end = r.tell()
        self.result["checks"].extend(("BA2 fixed header and complete record/name table extents",
                                      "BA2 filename stem, directory and extension lookup identity"))
        if subtype == b"DX10":
            self.result["checks"].append("BA2 DX10 chunk widths, sentinels and complete mip ranges")
        else:
            self.result["checks"].append("BA2 GNRL record sentinels")
            self.result["gnrl_record_flags"] = record_markers
        self.spans(payloads, [(0, records_end), (names_start, names_end)], eligible=subtype == b"DX10")

    def tes3(self):
        """Inspect TES3 relative tables, hash-half serialization and unshared payloads."""
        r = self.reader
        r.seek(4)
        relative_hashes, count = r.unpack("II")
        self.result["profile"] = {"type": "bsa", "version": 0x100, "subtype": "TES3",
                                  "method": "none", "entry_count": count, "header_size": 12}
        hash_start = relative_hashes + 12
        self.count(count, r.size - 12, 20, "TES3 file")
        names_start = 12 + count * 12
        data_start = hash_start + count * 8
        if hash_start < names_start or data_start > r.size:
            raise ValueError("TES3 hash/name table range is outside metadata bounds")
        records = [r.unpack("II") for _ in range(count)]
        offsets = [r.unpack("I")[0] for _ in range(count)]
        names = []
        for offset in offsets:
            if offset >= hash_start - names_start:
                raise ValueError("TES3 name offset is outside filename table")
            r.seek(names_start + offset)
            raw_name = r.cstring(hash_start)
            names.append(raw_name)
            self.name(raw_name)
        r.seek(hash_start)
        previous = None
        for index, name in enumerate(names):
            first_half, second_half = r.unpack("II")
            stored = (first_half << 32) | second_half
            if stored != _tes3_hash(name):
                self.problem(f"record {index}: TES3 name hash mismatch", historical=True)
            if previous is not None and stored < previous:
                self.problem(f"record {index}: TES3 hash order is descending", historical=True)
            previous = stored
        payloads = [(data_start + offset, size, f"record {index}", None)
                    for index, (size, offset) in enumerate(records)]
        self.result["stats"]["raw_payloads"] = count
        self.result["checks"].extend(("TES3 header, name-offset and hash-table extents",
                                      "TES3 hash-half identity and composed-hash order"))
        self.spans(payloads, [(0, data_start)], sharing=False)

    def tes4(self):
        """Inspect TES4-family folder geometry, lookup hashes and payload prefixes."""
        r = self.reader
        r.seek(4)
        version, folder_start, flags, folder_count, file_count, folder_names_size, file_names_size, file_flags = r.unpack("8I")
        if version not in (103, 104, 105):
            raise ValueError(f"unsupported TES4-family BSA version {version}")
        self.result["profile"] = {"type": "bsa", "version": version, "subtype": "TES4",
                                  "method": "lz4_frame" if version == 105 else "zlib",
                                  "entry_count": file_count, "header_size": 36, "flags": flags,
                                  "embedded_names": version in (104, 105) and bool(flags & 0x100)}
        if flags & 3 != 3:
            raise ValueError("unsupported BSA without both folder and file name tables")
        if folder_start < 36:
            raise ValueError("BSA folder table overlaps the fixed header")
        r.seek(folder_start)
        folder_width = 24 if version == 105 else 16
        self.count(folder_count, r.size - folder_start, folder_width, "BSA folder")
        self.count(file_count, r.size - folder_start, 16, "BSA file")
        folders = []
        for _ in range(folder_count):
            folder_hash, count = r.unpack("QI")
            if version == 105:
                reserved, offset = r.unpack("IQ")
            else:
                offset, = r.unpack("I")
            folders.append((folder_hash, count, offset))
        if sum(folder[1] for folder in folders) != file_count:
            raise ValueError("BSA folder file counts disagree with header file count")
        entries = []
        previous_folder = None
        consumed_folder_names = 0
        for folder_index, (folder_hash, count, offset) in enumerate(folders):
            block_start = r.tell()
            if offset != block_start + file_names_size:
                self.problem(f"folder {folder_index}: stored folder offset has incorrect filename-table bias", historical=True)
            name_size, = r.unpack("B")
            folder_name = r.read(name_size)
            consumed_folder_names += name_size
            if not folder_name or folder_name[-1:] != b"\0":
                raise ValueError("BSA folder name lacks its length-counted NUL terminator")
            folder_name = folder_name[:-1]
            if folder_hash != _tes4_hash(folder_name):
                self.problem(f"folder {folder_index}: BSA folder hash mismatch", historical=True)
            if previous_folder is not None and folder_hash < previous_folder:
                self.problem(f"folder {folder_index}: BSA folder hash order is descending", historical=True)
            previous_folder = folder_hash
            self.count(count, r.size - r.tell(), 16, "BSA folder file")
            previous_file = None
            for _ in range(count):
                file_hash, stored_size, file_offset = r.unpack("QII")
                if previous_file is not None and file_hash < previous_file:
                    self.problem(f"folder {folder_index}: BSA file hash order is descending", historical=True)
                previous_file = file_hash
                entries.append((folder_name, file_hash, stored_size, file_offset))
        if consumed_folder_names != folder_names_size:
            self.problem("BSA declared folder-name length disagrees with stored names", historical=True)
        names_start = r.tell()
        names_limit = names_start + file_names_size
        if names_limit > r.size:
            raise ValueError("BSA filename table extent exceeds archive size")
        named_entries = []
        for index, (folder_name, file_hash, stored_size, file_offset) in enumerate(entries):
            file_name = r.cstring(names_limit)
            if b"/" in file_name or b"\\" in file_name:
                self.problem(f"record {index}: BSA leaf filename contains a directory separator")
            raw_name = folder_name + b"\\" + file_name if folder_name else file_name
            display = self.name(raw_name)
            if file_hash != _tes4_hash(*_split_extension(file_name)):
                self.problem(f"{display}: BSA file hash mismatch", historical=True)
            named_entries.append((raw_name, display, stored_size, file_offset))
        metadata_end = r.tell()
        if metadata_end - names_start != file_names_size:
            self.problem("BSA declared filename-table length disagrees with stored names", historical=True)
        payloads = []
        for raw_name, display, stored_size, offset in named_entries:
            raw_size = stored_size & ~0x40000000
            compressed = bool(flags & 4) != bool(stored_size & 0x40000000)
            self.result["stats"]["compressed_payloads" if compressed else "raw_payloads"] += 1
            payloads.append((offset, raw_size, display, None))
            if offset > r.size or raw_size > r.size - offset or offset < metadata_end:
                continue  # Span validation below reports the authoritative bounds error.
            r.seek(offset)
            end = offset + raw_size
            if version in (104, 105) and flags & 0x100:
                self.result["stats"]["embedded_name_payloads"] += 1
                name_length, = r.unpack("B", end)
                embedded = r.read(name_length, end)
                if embedded.lower().replace(b"/", b"\\") != raw_name.lower().replace(b"/", b"\\"):
                    self.problem(f"{display}: BSA embedded name differs from lookup name", historical=True)
            if compressed:
                r.unpack("I", end)
        self.result["checks"].extend(("BSA fixed/folder/file/name table extents and counts",
                                      "BSA folder-offset filename-table bias",
                                      "BSA folder/file hash identity and order",
                                      "BSA embedded-name and decompressed-size prefix bounds"))
        self.spans(payloads, [(0, metadata_end)])


def inspect_archive(path: Path, strict_writer: bool = False) -> dict:
    """Return independently checked archive profile, complete paths and diagnostics.

    Strict writer mode treats lookup/hash/order/sentinel discrepancies as findings.
    Tolerant mode records these historical inconsistencies as observations, while
    unsafe/truncated ranges remain findings. Unsupported formats fail explicitly.
    Scratch reads are capped at 64 KiB; retained metadata grows with entry count.
    TES3's version value 256 denotes its signature, since it has no version field.
    File/open and malformed-metadata errors are returned in findings, not thrown.
    """
    inspection = None
    try:
        with Path(path).open("rb") as source:
            reader = _Reader(source)
            inspection = _Inspection(reader, strict_writer)
            magic = reader.read(4)
            if magic == b"BTDX":
                inspection.ba2()
            elif magic == b"BSA\0":
                inspection.tes4()
            elif magic == b"\0\x01\0\0":
                inspection.tes3()
            else:
                raise ValueError(f"unsupported archive magic {magic!r}")
    except (OSError, ValueError, struct.error) as error:
        if inspection is None:
            return {"profile": {"type": "unknown", "version": None, "subtype": None, "entry_count": 0},
                    "paths": [], "findings": [f"archive inspection failed: {error}"],
                    "observations": [], "checks": []}
        inspection.problem(f"metadata inspection failed: {error}")
    return inspection.finish()
