#!/usr/bin/env python3
"""Parser safety tests use synthetic inputs. Set TT_ORIGINAL_DIR for local
 * compatibility tests with your own game; no game bytes are stored here."""
import ctypes
import importlib.util
import os
from pathlib import Path
import random
import shutil
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("prepare_data", ROOT / "tools/prepare_data.py")
prepare = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prepare)


class OriginalDataTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="topten-test-")
        cls.folder = Path(cls.temp.name)
        lib = cls.folder / "reader.so"
        subprocess.run(["cc", "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-shared", "-fPIC",
                        str(ROOT / "src/dsimg.c"), str(ROOT / "src/pak.c"), str(ROOT / "src/original.c"), "-o", str(lib)], check=True)
        cls.lib = ctypes.CDLL(str(lib))
        for name in ("ds_load", "ds_load_dat", "pak_open"):
            getattr(cls.lib, name).argtypes = [ctypes.c_char_p]
            getattr(cls.lib, name).restype = ctypes.c_int
        cls.lib.ds_error.restype = ctypes.c_char_p
        cls.lib.ds_ptr_target.argtypes = [ctypes.c_uint]
        cls.lib.ds_ptr_target.restype = ctypes.c_uint
        cls.lib.ds_checksum.restype = ctypes.c_uint32
        cls.lib.pak_name.argtypes = [ctypes.c_int]
        cls.lib.pak_name.restype = ctypes.c_char_p

    @classmethod
    def tearDownClass(cls):
        cls.lib.ds_unload()
        cls.lib.pak_close()
        cls.temp.cleanup()

    def write(self, name, data):
        path = self.folder / name
        path.write_bytes(data)
        return os.fsencode(path)

    def assert_empty(self):
        self.assertEqual(ctypes.c_size_t.in_dll(self.lib, "ds_size").value, 0)
        self.assertFalse(ctypes.c_void_p.in_dll(self.lib, "ds_data").value)
        self.assertEqual(self.lib.ds_checksum(), 0)

    def test_missing_and_random_executables(self):
        self.assertEqual(self.lib.ds_load(os.fsencode(self.folder / "missing.exe")), -1)
        rng = random.Random(13)
        for size in (0, 1, 63, 64, 96, 512, 4096):
            with self.subTest(size=size):
                data = bytes(rng.randrange(256) for _ in range(size))
                self.assertEqual(self.lib.ds_load(self.write("bad.exe", data)), -1)
                self.assert_empty()

    def test_invalid_ne_offsets_and_segments(self):
        exe = bytearray(256)
        exe[:2] = b"MZ"
        for at in (255, 256, 0xFFFFFFFF):
            struct.pack_into("<I", exe, 60, at)
            self.assertEqual(self.lib.ds_load(self.write("bad.exe", exe)), -1)
        struct.pack_into("<I", exe, 60, 64)
        exe[64:66] = b"NE"
        struct.pack_into("<H", exe, 64 + 14, 1)
        struct.pack_into("<H", exe, 64 + 28, 1)
        struct.pack_into("<H", exe, 64 + 34, 64)
        for shift in (17, 65535):
            struct.pack_into("<H", exe, 64 + 50, shift)
            self.assertEqual(self.lib.ds_load(self.write("bad.exe", exe)), -1)
        struct.pack_into("<H", exe, 64 + 50, 0)
        struct.pack_into("<HHH", exe, 128, 65535, 19876, 0x101)
        self.assertEqual(self.lib.ds_load(self.write("bad.exe", exe)), -1)

    def test_invalid_dat_directory_and_entries(self):
        for data in (b"", b"bad", struct.pack("<HI", 65535, 0xFFFFFFFF), struct.pack("<HI", 0, 6)):
            self.assertEqual(self.lib.pak_open(self.write("bad.dat", data)), -1)
            self.assertEqual(self.lib.pak_count(), 0)
        valid = struct.pack("<HI", 1, 7) + b"x" + b"EXAMPLE.BIN\0".ljust(56, b"\0") + struct.pack("<II", 1, 6)
        self.assertEqual(self.lib.pak_open(self.write("tiny.dat", valid)), 0)
        self.assertEqual(self.lib.ds_load_dat(self.write("tiny.dat", valid)), 1)
        self.assertEqual(self.lib.pak_name(-1), b"")
        invalid = bytearray(valid)
        struct.pack_into("<II", invalid, 7 + 56, 0xFFFFFFFF, 0xFFFFFFFF)
        self.assertEqual(self.lib.pak_open(self.write("bad.dat", invalid)), -1)
        with self.assertRaises(ValueError):
            prepare.validate_dat(invalid)

    def test_invalid_prepared_footer(self):
        for base, size, count in ((6,19876,101), (6,0xFFFFFFFF,101), (6,19876,0xFFFFFFFF)):
            data = b"x" * 64 + prepare.FOOTER.pack(prepare.MAGIC, base, size, count, 0)
            self.assertEqual(self.lib.ds_load_dat(self.write("bad.dat", data)), -1)
            self.assert_empty()

    def test_filename_case_and_capacity(self):
        mixed = self.folder / "TeNnIs.ExE"
        mixed.write_bytes(b"synthetic")
        out = ctypes.create_string_buffer(1024)
        self.assertEqual(self.lib.original_path(out, len(out), os.fsencode(self.folder), b"TENNIS.EXE"), 0)
        self.assertEqual(Path(os.fsdecode(out.value)), mixed)
        self.assertEqual(self.lib.original_path(out, 4, os.fsencode(self.folder), b"TENNIS.EXE"), -1)

    @unittest.skipUnless(os.environ.get("TT_ORIGINAL_DIR"), "local original files not supplied")
    def test_original_exe_and_prepared_dat_are_identical(self):
        original = Path(os.environ["TT_ORIGINAL_DIR"])
        output = self.folder / "prepared" / "TENNIS.DAT"
        source = prepare.find_file(original, "TENNIS.DAT")
        exe = prepare.find_file(original, "TENNIS.EXE")
        before = source.read_bytes()
        data, pairs = prepare.extract(exe.read_bytes())
        prepare.prepare(original, output)
        self.assertEqual(source.read_bytes(), before)
        self.assertEqual(output.read_bytes()[:len(before)], before)
        with self.assertRaises(ValueError):
            prepare.prepare(original, source)
        with self.assertRaises(ValueError):
            prepare.prepare(original, output)
        self.assertEqual(self.lib.ds_load(os.fsencode(exe)), 0)
        expected = ctypes.string_at(ctypes.c_void_p.in_dll(self.lib, "ds_data").value, 65536)
        self.assertEqual(expected, data + bytes(65536 - len(data)))
        pointers = [self.lib.ds_ptr_target(i) for i in range(0, 65536, 2)]
        for source_at, target in struct.iter_unpack("<HH", pairs):
            self.assertEqual(pointers[source_at // 2], target)
        check = self.lib.ds_checksum()
        self.assertEqual(self.lib.ds_load_dat(os.fsencode(output)), 0)
        self.assertEqual(ctypes.string_at(ctypes.c_void_p.in_dll(self.lib, "ds_data").value, 65536), expected)
        self.assertEqual([self.lib.ds_ptr_target(i) for i in range(0, 65536, 2)], pointers)
        self.assertEqual(self.lib.ds_checksum(), check)
        self.assertEqual(self.lib.pak_open(os.fsencode(output)), 0)
        self.assertEqual(self.lib.pak_count(), struct.unpack_from("<H", before)[0])
        corrupt = bytearray(output.read_bytes())
        corrupt[len(before)] ^= 1
        self.assertEqual(self.lib.ds_load_dat(self.write("corrupt.dat", corrupt)), -1)
        self.assert_empty()
        self.assertEqual(self.lib.ds_load(os.fsencode(exe)), 0)
        self.assertEqual(self.lib.ds_load(self.write("truncated.exe", exe.read_bytes()[:64])), -1)
        self.assert_empty()
        for amount in (1, 4, 24, 100):
            self.assertEqual(self.lib.ds_load_dat(self.write("truncated.dat", output.read_bytes()[:-amount])), 1)
            self.assert_empty()  # treated as ordinary DAT: EXE is then required, no invalid tables installed


if __name__ == "__main__":
    unittest.main()
