"""Hand-authored archive envelopes test the independent structural oracle."""

import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest


MODULE = Path(__file__).with_name("structure.py")
spec = importlib.util.spec_from_file_location("structure", MODULE)
oracle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(oracle)


def ba2(*, version=1, texture=False, records=1, shared=False):
    """Make tiny archives with fixed, independently derived hashes for d/a and d/b."""
    header_size = {1: 24, 2: 32, 3: 36, 7: 24, 8: 24}[version]
    record_size = 48 if texture else 36
    raw_size = 8 if texture else 4
    names = [b"d/a.dds" if texture else b"d/a.bin", b"d/b.dds" if texture else b"d/b.bin"][:records]
    payload_start = header_size + record_size * records
    names_start = payload_start + raw_size * (1 if shared else records)
    data = bytearray(struct.pack("<4sI4sIQ", b"BTDX", version, b"DX10" if texture else b"GNRL", records, names_start))
    if version in (2, 3):
        data.extend(struct.pack("<II", 1, 0))
    if version == 3:
        data.extend(struct.pack("<I", 3))
    for index in range(records):
        name_hash = (0x3AB551CE, 0xA3BC0074)[index]
        offset = payload_start + (0 if shared else index * raw_size)
        if texture:
            data.extend(struct.pack("<I4sIBBHHHBBH", name_hash, b"dds\0", 0x4ADFA541, 0, 1, 24, 4, 4, 1, 71, 0x800))
            data.extend(struct.pack("<QIIHHI", offset, 0, 8, 0, 0, 0xBAADF00D))
        else:
            data.extend(struct.pack("<I4sIIQIII", name_hash, b"bin\0", 0x4ADFA541, 0x100100, offset, 0, 4, 0xBAADF00D))
    data.extend(b"x" * (names_start - payload_start))
    for name in names:
        data.extend(struct.pack("<H", len(name)) + name)
    return data


def tes3(*, records=1):
    """Use literal two-half TES3 hashes, stored high half then low half on disk."""
    names = [b"b", b"a"][:records]
    hash_start = 12 + records * 12 + records * 2
    data = bytearray(struct.pack("<III", 0x100, hash_start - 12, records))
    for index in range(records):
        data.extend(struct.pack("<II", 1, index))
    for index in range(records):
        data.extend(struct.pack("<I", index * 2))
    for name in names:
        data.extend(name + b"\0")
    for low in (0x80000018, 0x80000030)[:records]:
        data.extend(struct.pack("<II", 0, low))
    data.extend(b"x" * records)
    return data


def tes4(*, version=104, records=1, embedded=False):
    """Build BSA folder d with extensionless a/b hashes calculable by inspection."""
    folder_bytes = 24 if version == 105 else 16
    names_length = 2 * records
    block_start = 36 + folder_bytes
    payload_start = block_start + 3 + 16 * records + names_length
    data = bytearray(struct.pack("<4s8I", b"BSA\0", version, 36, 3 | (0x100 if embedded else 0), 1, records, 2, names_length, 0))
    data.extend(struct.pack("<QI", 0x64010064, records))
    data.extend(struct.pack("<IQ", 0, block_start + names_length) if version == 105 else struct.pack("<I", block_start + names_length))
    data.extend(b"\x02d\0")
    stored_size = 5 if embedded else 1
    for index in range(records):
        data.extend(struct.pack("<QII", (0x61010061, 0x62010062)[index], stored_size, payload_start + stored_size * index))
    for name in (b"a", b"b")[:records]:
        data.extend(name + b"\0")
    for name in (b"a", b"b")[:records]:
        data.extend((b"\x03d\\" + name if embedded else b"") + b"x")
    return data


class StructuralOracleTests(unittest.TestCase):
    """Prove lookup/extent corruption fails without relying on decoded payloads."""

    def setUp(self):
        """Give each envelope check its own disposable host file."""
        self.root = tempfile.TemporaryDirectory()
        self.addCleanup(self.root.cleanup)

    def inspect(self, data, strict=True):
        """Inspect a disposable archive, leaving all reference inputs untouched."""
        path = Path(self.root.name) / "input.archive"
        path.write_bytes(data)
        return oracle.inspect_archive(path, strict_writer=strict)

    def test_all_ba2_header_widths_and_subtypes(self):
        """BA2 7/8 retain the 24-byte header; versions are not a size ladder."""
        for version in (1, 2, 3, 7, 8):
            for texture in (False, True):
                with self.subTest(version=version, texture=texture):
                    result = self.inspect(ba2(version=version, texture=texture))
                    self.assertEqual([], result["findings"])
                    self.assertEqual(version, result["profile"]["version"])
                    self.assertEqual("DX10" if texture else "GNRL", result["profile"]["subtype"])
                    self.assertEqual(["d/a.dds" if texture else "d/a.bin"], result["paths"])

    def test_wrong_ba2_lookup_fields_fail_without_payload_changes(self):
        """BSArch can unpack using table order while game hash lookup is broken."""
        for texture in (False, True):
            for offset in (24, 28, 32):
                damaged = ba2(texture=texture)
                damaged[offset] ^= 1
                with self.subTest(texture=texture, offset=offset):
                    self.assertTrue(self.inspect(damaged)["findings"])

    def test_uppercase_ba2_extension_is_not_a_lookup_equivalent(self):
        """The game compares the stored extension FourCC exactly."""
        damaged = ba2()
        damaged[28:31] = b"BIN"
        self.assertTrue(any("extension" in f for f in self.inspect(damaged)["findings"]))

    def test_gnrl_record_markers_are_observed_without_banning_explicit_overrides(self):
        """Default-route proof can inspect marker values while advanced overrides survive."""
        data = ba2(records=2)
        struct.pack_into("<I", data, 72, 0)
        result = self.inspect(data)
        self.assertEqual([], result["findings"])
        self.assertEqual({"0x00100100": 1, "0x00000000": 1}, result["gnrl_record_flags"])

    def test_retail_lookup_anomalies_are_observations(self):
        """Historical lookup disagreement does not become a blanket retail rejection."""
        damaged = ba2()
        damaged[24] ^= 1
        result = self.inspect(damaged, strict=False)
        self.assertEqual([], result["findings"])
        self.assertTrue(result["observations"])

    def test_ba2_payload_bounds_and_metadata_intersections(self):
        """Offsets into headers, filename tables or beyond EOF must fail."""
        for texture, field in ((False, 40), (True, 48)):
            original = ba2(texture=texture)
            names = struct.unpack_from("<Q", original, 16)[0]
            for bad in (0, 20, names, len(original) + 1, 0xFFFFFFFFFFFFFFFF):
                damaged = bytearray(original)
                struct.pack_into("<Q", damaged, field, bad)
                with self.subTest(texture=texture, bad=bad):
                    self.assertTrue(self.inspect(damaged)["findings"])

    def test_ba2_sentinel_and_chunk_record_width(self):
        """Ignored sentinel and chunkHeaderSize fields still need independent checks."""
        for texture, field in ((False, 56), (True, 68), (True, 38)):
            damaged = ba2(texture=texture)
            damaged[field] ^= 1
            with self.subTest(texture=texture, field=field):
                self.assertTrue(self.inspect(damaged)["findings"])

    def test_dx10_empty_or_out_of_range_mip_chunks_fail(self):
        """A chunk must carry nonempty data and cover the texture's mip chain."""
        for field, fmt, value in ((60, "I", 0), (64, "H", 1), (66, "H", 1), (44, "B", 0)):
            damaged = ba2(texture=True)
            struct.pack_into("<" + fmt, damaged, field, value)
            with self.subTest(field=field):
                self.assertTrue(self.inspect(damaged)["findings"])

    def test_cube_accepts_aggregate_or_six_complete_face_chains(self):
        """BSArch aggregates faces; libbsa may emit a complete mip chain per face."""
        aggregate = ba2(texture=True)
        struct.pack_into("<H", aggregate, 46, 0x801)
        struct.pack_into("<I", aggregate, 60, 48)
        struct.pack_into("<Q", aggregate, 16, 120)
        aggregate[72:80] = b"x" * 48
        self.assertEqual([], self.inspect(aggregate)["findings"])

        # Six one-mip face records: the record table ends at 192, and each
        # independent 4x4 BC1 face contributes eight bytes to the payload area.
        faces = bytearray(aggregate[:48])
        faces[37] = 6
        struct.pack_into("<Q", faces, 16, 240)
        for index in range(6):
            faces.extend(struct.pack("<QIIHHI", 192 + 8 * index, 0, 8, 0, 0, 0xBAADF00D))
        faces.extend(b"x" * 48 + b"\x07\0d/a.dds")
        self.assertEqual([], self.inspect(faces)["findings"])

        noncube = bytearray(faces)
        struct.pack_into("<H", noncube, 46, 0x800)
        self.assertTrue(any("mip" in f for f in self.inspect(noncube)["findings"]))
        two_faces = bytearray(faces[:48])
        two_faces[37] = 2
        struct.pack_into("<Q", two_faces, 16, 112)
        for index in range(2):
            two_faces.extend(struct.pack("<QIIHHI", 96 + 8 * index, 0, 8, 0, 0, 0xBAADF00D))
        two_faces.extend(b"x" * 16 + b"\x07\0d/a.dds")
        self.assertTrue(any("mip" in f for f in self.inspect(two_faces)["findings"]))

    def test_payload_route_counts_and_embedded_name_evidence(self):
        """Route assertions use independently observed fields, including BSA inversion."""
        data = ba2(records=2, shared=True)
        struct.pack_into("<I", data, 48, 4)
        result = self.inspect(data)
        self.assertEqual(1, result["stats"]["compressed_payloads"])
        self.assertEqual(1, result["stats"]["raw_payloads"])
        self.assertEqual(1, result["stats"]["shared_payload_spans"])
        bsa = self.inspect(tes4(embedded=True))
        self.assertTrue(bsa["profile"]["embedded_names"])
        self.assertEqual(1, bsa["stats"]["embedded_name_payloads"])
        self.assertEqual(1, self.inspect(tes3())["stats"]["raw_payloads"])

        inverted = tes4()
        struct.pack_into("<I", inverted, 12, 7)
        struct.pack_into("<I", inverted, 63, 0x40000001)
        raw = self.inspect(inverted)
        self.assertEqual([], raw["findings"])
        self.assertEqual(1, raw["stats"]["raw_payloads"])
        compressed = tes4()
        struct.pack_into("<I", compressed, 12, 7)
        struct.pack_into("<I", compressed, 63, 5)
        compressed[-1:] = struct.pack("<I", 1) + b"x"
        encoded = self.inspect(compressed)
        self.assertEqual([], encoded["findings"])
        self.assertEqual(1, encoded["stats"]["compressed_payloads"])

    def test_ba2_partial_overlap_fails_exact_sharing_passes(self):
        """Shared stored spans are permitted; partially overlapping spans are not."""
        for texture, field, step in ((False, 76, 4), (True, 96, 8)):
            self.assertEqual([], self.inspect(ba2(texture=texture, records=2, shared=True))["findings"])
            damaged = ba2(texture=texture, records=2)
            offset = struct.unpack_from("<Q", damaged, field)[0]
            struct.pack_into("<Q", damaged, field, offset - step + 1)
            with self.subTest(texture=texture):
                self.assertTrue(any("overlap" in f for f in self.inspect(damaged)["findings"]))

    def test_truncated_names_and_impossible_counts_fail(self):
        """Metadata length checks run before allocations based on declared counts."""
        for original in (ba2(), ba2(texture=True), tes3(), tes4()):
            for cut in (3, 15, len(original) - 2):
                with self.subTest(size=len(original), cut=cut):
                    self.assertTrue(self.inspect(original[:cut])["findings"])
        damaged = ba2()
        struct.pack_into("<I", damaged, 12, 0xFFFFFFFF)
        self.assertTrue(self.inspect(damaged)["findings"])

    def test_ba2_duplicate_canonical_names_fail(self):
        """Filename aliases must not be hidden by complete-list comparison."""
        damaged = ba2(records=2)
        damaged[-7:] = b"D/A.BIN"
        self.assertTrue(any("duplicate" in f for f in self.inspect(damaged)["findings"]))

    def test_tes3_hash_half_order_and_sorted_records(self):
        """TES3 writes two little-endian halves, not a little-endian uint64."""
        self.assertEqual([], self.inspect(tes3(records=2))["findings"])
        damaged = tes3()
        damaged[26:34] = damaged[30:34] + damaged[26:30]
        self.assertTrue(any("hash" in f for f in self.inspect(damaged)["findings"]))

    def test_tes3_long_name_hash_rotates_and_wraps_shift_positions(self):
        """This literal vector exercises both half-sums beyond one four-byte group."""
        data = (struct.pack("<7I", 0x100, 23, 1, 1, 0, 0, 0)[:24]
                + b"abcdefghij\0" + struct.pack("<II", 0x64636204, 0xDAFC5A19) + b"x")
        self.assertEqual([], self.inspect(data)["findings"])

    def test_tes3_unsorted_correct_hashes_fail(self):
        """Correct hash/name pairs still require composed-hash ordering."""
        damaged = tes3(records=2)
        damaged[36:40] = b"a\0b\0"
        damaged[40:56] = damaged[48:56] + damaged[40:48]
        self.assertTrue(any("order" in f for f in self.inspect(damaged)["findings"]))

    def test_tes3_name_offsets_and_data_relative_offsets(self):
        """Each directory offset binds to a name and payload offsets use data origin."""
        for field, value in ((4, 0), (16, 99), (20, 1)):
            damaged = tes3()
            struct.pack_into("<I", damaged, field, value)
            with self.subTest(field=field):
                self.assertTrue(self.inspect(damaged)["findings"])

    def test_tes3_overlapping_entries_fail_even_exact_aliases(self):
        """TES3's write contract disables payload sharing."""
        damaged = tes3(records=2)
        struct.pack_into("<I", damaged, 24, 0)
        self.assertTrue(any("overlap" in f for f in self.inspect(damaged)["findings"]))

    def test_all_tes4_versions_and_embedded_prefixes(self):
        """SSE expands folder records, while FO3/SSE may embed payload names."""
        for version in (103, 104, 105):
            with self.subTest(version=version):
                self.assertEqual([], self.inspect(tes4(version=version, records=2))["findings"])
        for version in (104, 105):
            with self.subTest(version=version, embedded=True):
                self.assertEqual([], self.inspect(tes4(version=version, embedded=True))["findings"])

    def test_tes4_folder_and_file_hashes_fail_independently(self):
        """Listing and extraction alone do not verify either BSA lookup hash."""
        for field in (36, 55):
            damaged = tes4()
            damaged[field] ^= 1
            with self.subTest(field=field):
                self.assertTrue(any("hash" in f for f in self.inspect(damaged)["findings"]))

    def test_tes4_middle_and_extension_hashes_have_independent_literal_evidence(self):
        """abcd.dds has a middle-character high sum and DDS-specific low hash bits."""
        data = tes4()
        struct.pack_into("<I", data, 28, 9)
        struct.pack_into("<I", data, 48, 61)
        struct.pack_into("<Q", data, 55, 0x8DDBAA276104E3E4)
        struct.pack_into("<I", data, 67, 80)
        data[71:73] = b"abcd.dds\0"
        self.assertEqual([], self.inspect(data)["findings"])

    def test_tes4_folder_offsets_include_filename_table_bias(self):
        """The stored folder offset includes totalFileNameLength; it is not direct."""
        damaged = tes4()
        struct.pack_into("<I", damaged, 48, 52)
        self.assertTrue(any("folder offset" in f for f in self.inspect(damaged)["findings"]))

    def test_tes4_name_lengths_and_payload_prefix_bounds(self):
        """Writer-side name totals and embedded prefixes cannot escape their extents."""
        for field, value in ((24, 9), (28, 9), (52, 250)):
            damaged = tes4()
            if field == 52:
                damaged[field] = value
            else:
                struct.pack_into("<I", damaged, field, value)
            with self.subTest(field=field):
                self.assertTrue(self.inspect(damaged)["findings"])
        damaged = tes4(embedded=True)
        damaged[-5] = 255
        self.assertTrue(self.inspect(damaged)["findings"])

    def test_tes4_unsorted_hashes_fail_even_with_correct_names(self):
        """Swap complete file records and names to preserve hashes while breaking order."""
        damaged = tes4(records=2)
        damaged[55:87] = damaged[71:87] + damaged[55:71]
        damaged[87:91] = b"b\0a\0"
        self.assertTrue(any("order" in f for f in self.inspect(damaged)["findings"]))

    def test_unknown_profile_is_an_explicit_failure(self):
        """A partially recognized archive must never report structural success."""
        damaged = ba2()
        struct.pack_into("<I", damaged, 4, 4)
        for data in (damaged, b"not an archive"):
            with self.subTest(data=data[:24]):
                self.assertTrue(self.inspect(data)["findings"])


if __name__ == "__main__":
    unittest.main()
