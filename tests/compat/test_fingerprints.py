"""Independent oracle-comparison tests using hand-authored DDS headers and bytes."""

import hashlib
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest


MODULE = Path(__file__).with_name("fingerprints.py")
spec = importlib.util.spec_from_file_location("fingerprints", MODULE)
fp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fp)


def dds(payload=b"12345678", *, fourcc=b"DXT1", dxgi=71, width=4, height=4,
        mips=1, depth=0, dimension=3, array=1, cube=False, alpha=0):
    """Build a DDS envelope without deriving expected layouts through the comparator."""
    header = bytearray(128)
    header[:4] = b"DDS "
    flags = 0x1007 | (0x20000 if mips > 1 else 0) | (0x800000 if depth else 0)
    struct.pack_into("<7I", header, 4, 124, flags, height, width, 0, depth, mips)
    struct.pack_into("<II4s", header, 76, 32, 4, fourcc)
    struct.pack_into("<II", header, 108, 0x1000, (0xFE00 if cube else 0) | (0x200000 if depth else 0))
    if fourcc == b"DX10":
        header.extend(struct.pack("<5I", dxgi, dimension, 4 if cube else 0, array, alpha))
    return bytes(header) + payload


def record(data, path="textures/example.dds"):
    """Produce the streaming C++ bridge's wire fields from independently chosen bytes."""
    result = {"path": path, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()}
    if data.startswith(b"DDS "):
        result.update(dds_header_hex=data[:148 if data[84:88] == b"DX10" else 128].hex(),
                      payload_sha256_128=hashlib.sha256(data[128:]).hexdigest(),
                      payload_sha256_148=hashlib.sha256(data[148:]).hexdigest())
    return result


class FingerprintTests(unittest.TestCase):
    def test_cached_legacy_header_contains_no_pixel_bytes(self):
        """A legacy header ends at128; caching148 would retain20 retail pixel bytes."""
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "texture.dds"
            path.write_bytes(dds(payload=b"abcdefgh" * 8, width=16, height=8))
            evidence = fp.fingerprint_file(path, "texture.dds")
            self.assertEqual(len(bytes.fromhex(evidence["dds_header_hex"])), 128)

    def test_uninterpreted_dds_preserves_exact_bytes(self):
        """BSA/GNRL DDS files are opaque bytes, including invalid texture envelopes."""
        value = record(b"not a DDS", "opaque.dds")
        self.assertEqual(fp.compare_catalogs([value], [value], dds_semantics=False), [])
        legacy, modern = record(dds()), record(dds(fourcc=b"DX10"))
        self.assertTrue(fp.compare_catalogs([legacy], [modern], dds_semantics=False))

    """Guard against false agreement caused by dropped bytes, paths, or DDS semantics."""

    def test_same_size_wrong_contents_fail(self):
        """A size-only comparator would falsely accept this payload substitution."""
        self.assertTrue(fp.compare_catalogs([record(b"one", "a.bin")], [record(b"two", "a.bin")]))

    def test_missing_and_extra_paths_report_both(self):
        """Equal file counts cannot substitute for complete path-set agreement."""
        errors = fp.compare_catalogs([record(b"a", "missing")], [record(b"a", "extra")])
        self.assertTrue(any("missing" in error for error in errors))
        self.assertTrue(any("extra" in error for error in errors))

    def test_canonical_paths_match_ascii_case_and_separators(self):
        """Archive keys use ASCII folding and slash normalization."""
        self.assertEqual([], fp.compare_catalogs([record(b"a", "A\\B.bin")], [record(b"a", "a/b.bin")]))

    def test_duplicates_are_not_hidden_by_normalization(self):
        """Dictionary overwrites must never hide an extra colliding record."""
        for entries in ([record(b"a", "a"), record(b"a", "a")],
                        [record(b"a", "A\\b"), record(b"a", "a/b")]):
            with self.subTest(entries=entries):
                self.assertTrue(any("duplicate" in e for e in fp.compare_catalogs(entries, entries)))

    def test_unicode_is_not_casefolded(self):
        """Unicode case folding would create archive aliases that ASCII rules do not."""
        self.assertTrue(fp.compare_catalogs([record(b"a", "Straße")], [record(b"a", "STRASSE")]))

    def test_invalid_archive_paths_are_reported(self):
        """Rooted, parent and empty-component paths cannot become extraction keys."""
        for path in ("", "../a", "/a", "C:/a", "a//b", "a/./b", "a/../b", "a\x00b"):
            with self.subTest(path=path):
                self.assertTrue(fp.compare_catalogs([record(b"a", path)], []))

    def test_legacy_bc1_and_dx10_unorm_compare_equal(self):
        """Different equivalent DDS envelopes must preserve the same texture result."""
        self.assertEqual([], fp.compare_catalogs([record(dds())], [record(dds(fourcc=b"DX10"))]))

    def test_dds_same_size_wrong_payload_fails(self):
        """Matching DDS metadata must not hide substituted texel block bytes."""
        self.assertTrue(fp.compare_catalogs([record(dds())], [record(dds(b"87654321"))]))

    def test_srgb_and_typeless_are_not_unorm(self):
        """Identical blocks have different interpretation under these DXGI formats."""
        for dxgi in (70, 72):
            with self.subTest(dxgi=dxgi):
                self.assertTrue(fp.compare_catalogs([record(dds())], [record(dds(fourcc=b"DX10", dxgi=dxgi))]))

    def test_alpha_modes_are_not_erased(self):
        """Unknown, straight, premultiplied and opaque alpha remain distinct."""
        for alpha in (1, 2, 3, 4):
            with self.subTest(alpha=alpha):
                self.assertTrue(fp.compare_catalogs([record(dds())], [record(dds(fourcc=b"DX10", alpha=alpha))]))

    def test_legacy_premultiplied_alpha_maps_to_dx10(self):
        """DXT2 is BC2 with premultiplied alpha, not plain DXT3."""
        source = record(dds(b"x" * 16, fourcc=b"DXT2"))
        self.assertEqual([], fp.compare_catalogs([source], [record(dds(b"x" * 16, fourcc=b"DX10", dxgi=74, alpha=2))]))
        self.assertTrue(fp.compare_catalogs([source], [record(dds(b"x" * 16, fourcc=b"DXT3"))]))

    def test_dimensions_are_not_hidden_by_equal_block_sizes(self):
        """BC block rounding makes these equal-sized but semantically different."""
        self.assertTrue(fp.compare_catalogs([record(dds())], [record(dds(width=3))]))

    def test_complete_mip_geometry_is_preserved(self):
        """A 4x4 BC1 chain has three separate eight-byte levels."""
        result = fp.comparable(record(dds(b"x" * 24, mips=3)))
        self.assertEqual([8, 8, 8], [m["bytes"] for m in result["dds"]["mips"]])
        self.assertEqual([4, 2, 1], [m["width"] for m in result["dds"]["mips"]])

    def test_cube_array_and_2d_array_are_distinct(self):
        """Six array layers are not a cube even when their byte totals match."""
        cube = record(dds(b"x" * 48, fourcc=b"DX10", cube=True))
        array = record(dds(b"x" * 48, fourcc=b"DX10", array=6))
        self.assertTrue(fp.compare_catalogs([cube], [array]))
        self.assertEqual([], fp.compare_catalogs([cube], [record(dds(b"x" * 48, cube=True))]))
        self.assertEqual(12, fp.comparable(record(dds(b"x" * 96, fourcc=b"DX10", cube=True, array=2)))["dds"]["layers"])

    def test_volume_mips_shrink_depth(self):
        """Volume levels shrink all three dimensions instead of repeating array layers."""
        texture = dds(b"x" * 56, fourcc=b"DX10", width=4, height=4, depth=4, dimension=4, mips=3)
        result = fp.comparable(record(texture))
        self.assertEqual([32, 16, 8], [m["bytes"] for m in result["dds"]["mips"]])

    def test_dxgi_family_sizes(self):
        """Hand-calculated lengths cover BC, scalar, packed color, and floating formats."""
        for fmt, size in ((2, 256), (6, 192), (10, 128), (28, 64), (49, 32),
                          (61, 16), (65, 16), (71, 8), (80, 8), (84, 16),
                          (87, 64), (95, 16), (96, 16), (98, 16), (99, 16), (115, 32)):
            with self.subTest(fmt=fmt):
                self.assertEqual(size, fp.comparable(record(dds(b"x" * size, fourcc=b"DX10", dxgi=fmt)))["dds"]["mips"][0]["bytes"])

    def test_legacy_rgba_matches_equivalent_dx10(self):
        """Mask interpretation must agree with the exact channel order."""
        legacy = bytearray(dds(b"x" * 64))
        struct.pack_into("<8I", legacy, 76, 32, 0x41, 0, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000)
        self.assertEqual([], fp.compare_catalogs([record(legacy)], [record(dds(b"x" * 64, fourcc=b"DX10", dxgi=28))]))
        self.assertTrue(fp.compare_catalogs([record(legacy)], [record(dds(b"x" * 64, fourcc=b"DX10", dxgi=87))]))

    def test_legacy_bgr24_retains_channel_order(self):
        """Legacy 24-bit RGB is valid without inventing a 32-bit DXGI counterpart."""
        bgr = bytearray(dds(b"x" * 48))
        struct.pack_into("<8I", bgr, 76, 32, 0x40, 0, 24, 0xFF0000, 0xFF00, 0xFF, 0)
        self.assertEqual(48, fp.comparable(record(bgr))["dds"]["mips"][0]["bytes"])
        rgb = bytearray(bgr)
        struct.pack_into("<3I", rgb, 92, 0xFF, 0xFF00, 0xFF0000)
        self.assertTrue(fp.compare_catalogs([record(bgr)], [record(rgb)]))

    def test_padded_rows_and_huge_array_do_not_pass_as_tight_layout(self):
        """The parser neither guesses row padding nor expands attacker-sized arrays."""
        padded = bytearray(dds(b"x" * 64, fourcc=b"DX10", dxgi=28))
        struct.pack_into("<I", padded, 8, 0x100F)
        struct.pack_into("<I", padded, 20, 20)
        for invalid in (padded, dds(fourcc=b"DX10", array=0xFFFFFFFF)):
            with self.subTest(invalid=invalid[:148].hex()), self.assertRaises(ValueError):
                fp.comparable(record(invalid))

    def test_invalid_dds_fails_explicitly_even_against_itself(self):
        """Invalid or unsupported textures cannot pass by using a raw-hash fallback."""
        invalid = [dds()[:-1], dds() + b"x", dds(fourcc=b"DX10")[:140],
                   dds(width=0), dds(mips=4), dds(fourcc=b"WUT?"),
                   dds(fourcc=b"DX10", dxgi=0), dds(fourcc=b"DX10", dxgi=103),
                   dds(fourcc=b"DX10", array=0), dds(fourcc=b"DX10", alpha=7),
                   dds(fourcc=b"DX10", dimension=2),
                   dds(fourcc=b"DX10", dimension=4, depth=4, array=2)]
        for data in invalid:
            with self.subTest(data=data[:148].hex()):
                with self.assertRaises(ValueError):
                    fp.comparable(record(data))
                self.assertTrue(fp.compare_catalogs([record(data)], [record(data)]))

    def test_malformed_header_sizes_and_cube_faces_fail(self):
        """Truncated structures and partial legacy cubes have no normalized fallback."""
        for offset, value in ((4, 120), (76, 28), (112, 0x600)):
            damaged = bytearray(dds())
            struct.pack_into("<I", damaged, offset, value)
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                fp.comparable(record(damaged))

    def test_missing_fingerprint_fields_fail(self):
        """Bridge records must carry actual digest evidence, never an empty default."""
        for key in ("size", "sha256", "dds_header_hex", "payload_sha256_128"):
            damaged = record(dds())
            del damaged[key]
            with self.subTest(key=key):
                self.assertTrue(fp.compare_catalogs([damaged], [damaged]))

    def test_file_streaming_produces_bridge_compatible_fields(self):
        """Disk hashing and the C++ wire representation use identical byte boundaries."""
        with tempfile.TemporaryDirectory() as root:
            path = Path(root) / "input"
            for data, key in ((b"binary" * 250000, "assets/a.bin"), (dds(), "Textures/A.dds"),
                              (dds(fourcc=b"DX10"), "textures/a.dds")):
                with self.subTest(key=key):
                    path.write_bytes(data)
                    actual = fp.fingerprint_file(path, key)
                    self.assertEqual(record(data, key), actual)


if __name__ == "__main__":
    unittest.main()
