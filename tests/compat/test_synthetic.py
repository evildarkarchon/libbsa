"""Tests for independent fixture recipes and offset-preserving adaptations."""

import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path


class SyntheticTests(unittest.TestCase):
    """Catch dropped payloads, invalid DDS layouts, and incomplete offset relocation."""

    def setUp(self):
        """Load the fixture implementation without depending on package installation."""
        path = Path(__file__).with_name("synthetic.py")
        self.assertTrue(path.exists(), "independent synthetic recipe module is missing")
        spec = importlib.util.spec_from_file_location("synthetic", path)
        self.synthetic = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.synthetic)

    def test_gnrl_adaptation_preserves_two_payloads_and_filename_table(self):
        """A missed second record relocation would point into preceding payload bytes."""
        with tempfile.TemporaryDirectory() as temporary:
            source, destination = Path(temporary)/"v2.ba2", Path(temporary)/"v3.ba2"
            original = bytearray(122)
            struct.pack_into("<4sI4sIQII", original, 0, b"BTDX", 2, b"GNRL", 2, 112, 1, 0)
            struct.pack_into("<QIII", original, 48, 104, 0, 3, 0xBAADF00D)
            struct.pack_into("<QIII", original, 84, 107, 0, 5, 0xBAADF00D)
            original[104:] = b"ABC12345\x03\x00one\x03\x00two"
            source.write_bytes(original)
            self.synthetic.adapt_archive(source, destination, "gnrl3-zlib")
            result = destination.read_bytes()
            self.assertEqual(struct.unpack_from("<I", result, 4)[0], 3)
            self.assertEqual(struct.unpack_from("<I", result, 32)[0], 0)
            self.assertEqual(struct.unpack_from("<Q", result, 16)[0], 116)
            self.assertEqual(struct.unpack_from("<Q", result, 52)[0], 108)
            self.assertEqual(struct.unpack_from("<Q", result, 88)[0], 111)
            self.assertEqual(result[108:111], b"ABC")
            self.assertEqual(result[111:116], b"12345")
            self.assertEqual(result[116:], b"\x03\x00one\x03\x00two")
            self.assertEqual(source.read_bytes(), original)

    def test_dx10_adaptation_moves_every_chunk_across_multiple_records(self):
        """Relocate variable record tables with two chunks followed by one chunk."""
        with tempfile.TemporaryDirectory() as temporary:
            source, destination = Path(temporary)/"v2.ba2", Path(temporary)/"v3.ba2"
            original = bytearray(167)
            struct.pack_into("<4sI4sIQII", original, 0, b"BTDX", 2, b"DX10", 2, 157, 1, 0)
            original[45] = 2
            struct.pack_into("<H", original, 46, 24)
            struct.pack_into("<QIIHHI", original, 56, 152, 0, 1, 0, 0, 0xBAADF00D)
            struct.pack_into("<QIIHHI", original, 80, 153, 0, 2, 1, 1, 0xBAADF00D)
            original[117] = 1
            struct.pack_into("<H", original, 118, 24)
            struct.pack_into("<QIIHHI", original, 128, 155, 0, 2, 0, 0, 0xBAADF00D)
            original[152:] = b"ABCDE\x03\x00one\x03\x00two"
            source.write_bytes(original)
            self.synthetic.adapt_archive(source, destination, "dx103-zlib")
            result = destination.read_bytes()
            self.assertEqual(struct.unpack_from("<Q", result, 16)[0], 161)
            self.assertEqual([struct.unpack_from("<Q", result, offset)[0]
                              for offset in (60, 84, 132)], [156, 157, 159])
            self.assertEqual(result[156:161], b"ABCDE")
            self.assertEqual(result[161:], b"\x03\x00one\x03\x00two")

    def test_reader_only_versions_change_no_layout_bytes(self):
        """FO4 v7/v8 retain v1 layout, so only the four version bytes may change."""
        with tempfile.TemporaryDirectory() as temporary:
            source, destination = Path(temporary)/"v1.ba2", Path(temporary)/"adapted.ba2"
            original = struct.pack("<4sI4sIQ", b"BTDX", 1, b"GNRL", 0, 24)
            source.write_bytes(original)
            for target, version in (("gnrl7", 7), ("gnrl8", 8)):
                self.synthetic.adapt_archive(source, destination, target)
                result = destination.read_bytes()
                self.assertEqual(result[:4] + result[8:], original[:4] + original[8:])
                self.assertEqual(struct.unpack_from("<I", result, 4)[0], version)

    def test_adaptation_rejects_wrong_profile_and_truncation_without_output(self):
        """Malformed source tables must never become apparently valid adapted evidence."""
        with tempfile.TemporaryDirectory() as temporary:
            source, destination = Path(temporary)/"bad.ba2", Path(temporary)/"result.ba2"
            for version, subtype in ((1, b"GNRL"), (2, b"DX10"), (2, b"GNRL")):
                source.write_bytes(struct.pack("<4sI4sIQII", b"BTDX", version, subtype, 1, 32, 1, 0))
                with self.assertRaises(ValueError):
                    self.synthetic.adapt_archive(source, destination, "gnrl3-zlib")
                self.assertFalse(destination.exists())
            with self.assertRaises(ValueError):
                self.synthetic.adapt_archive(source, source, "gnrl3-zlib")

    def test_recipes_include_duplicate_distinct_empty_and_mixed_policy_entries(self):
        """Generated source content must exercise sharing and incompressible paths."""
        case = next(item for item in self.synthetic.cases()
                    if item["target"] == "gnrl1" and item.get("mixed_entries"))
        with tempfile.TemporaryDirectory() as temporary:
            entries = self.synthetic.prepare_case(case, temporary)
            contents = [Path(entry["source"]).read_bytes() for entry in entries]
            self.assertIn(b"", contents)
            self.assertLess(len(set(contents)), len(contents))
            self.assertGreater(len(set(contents)), 3)
            self.assertEqual({entry.get("compression", "inherit") for entry in entries},
                             {"raw", "compressed", "inherit"})
            self.assertTrue(any(len(Path(entry["path"]).suffix) > 5 for entry in entries))

    def test_dds_cube_has_six_distinct_faces_and_complete_mips(self):
        """Missing face or small-mip BC blocks would invalidate oracle texture evidence."""
        case = next(item for item in self.synthetic.cases() if item["target"] == "dx101")
        with tempfile.TemporaryDirectory() as temporary:
            entries = self.synthetic.prepare_case(case, temporary)
            cube = next(Path(entry["source"]).read_bytes() for entry in entries
                        if "cube" in entry["path"])
            self.assertEqual(cube[:4], b"DDS ")
            self.assertEqual(struct.unpack_from("<III", cube, 12), (128, 128, 8192))
            self.assertEqual(struct.unpack_from("<I", cube, 28)[0], 8)
            self.assertEqual(struct.unpack_from("<I", cube, 112)[0], 0xFE00)
            # Hand-derived BC1 mip sizes: 8192+2048+512+128+32+8+8+8.
            self.assertEqual(len(cube), 128 + 6 * 10936)
            faces = [cube[128+i*10936:128+(i+1)*10936] for i in range(6)]
            self.assertEqual(len(set(faces)), 6)

    def test_texture_recipes_use_supported_target_formats_and_controls(self):
        """FO4 cannot preserve sRGB, and DX10 exposes no compression override."""
        for target in ("dx101", "dx107", "dx108", "dx102", "dx103-zlib", "dx103-lz4"):
            selected = [case for case in self.synthetic.cases() if case["target"] == target]
            for case in selected:
                self.assertEqual(case["options"]["compression"], "default")
                with tempfile.TemporaryDirectory() as temporary:
                    entries = self.synthetic.prepare_case(case, temporary)
                    self.assertTrue(all("compression" not in entry for entry in entries))
                    formats = set()
                    for entry in entries:
                        data = Path(entry["source"]).read_bytes()
                        formats.add(struct.unpack_from("<I", data, 128)[0]
                                    if data[84:88] == b"DX10" else 71)
                    if target in ("dx101", "dx107", "dx108"):
                        self.assertEqual(formats, {71, 98})
                    else:
                        self.assertEqual(formats, {71, 72, 95, 96, 98, 99})


if __name__ == "__main__":
    unittest.main()
