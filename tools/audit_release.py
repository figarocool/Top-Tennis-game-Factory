#!/usr/bin/env python3
"""Audit a release for known original-data leakage, not general legal clearance.

Supply --original-exe only locally; the original is never included in output.
Checks whitelisted artwork, forbidden paths, legacy dump symbols and known data
bytes in uncompressed engine executables. Does not inspect every possible form
of copyright or historical content unless those files are passed explicitly.
"""
import argparse
from pathlib import Path
import struct
import sys
import zipfile

from prepare_data import extract

FORBIDDEN = {"TENNIS.EXE", "TENNIS.DAT", "TENNIS.OPT", "DSDATA.C", "DS.BIN", "DECOMP.C", "DECOMP_CLEAN.C",
             "GEN_DSDATA.PY", "GEN_CTRL.PY", "G2C.PY"}
VPK_ALLOWED = {"eboot.bin", "sce_sys/param.sfo", "sce_sys/icon0.png", "LICENSE.md", "THIRD_PARTY_NOTICES.md"}


def check_name(name):
    p = Path(name)
    if p.name.upper() in FORBIDDEN or p.suffix.lower() == ".rar" or any(part in {"orig", "data", "private_art", "ghidra_proj", "prepared"} for part in p.parts):
        raise ValueError(f"forbidden release content: {name}")


def check_bytes(data, name, original):
    check_name(name)
    if Path(name).suffix.lower() not in {".py", ".md"} and (b"ds_image_size" in data or b"ds_reloc_count" in data):
        raise ValueError(f"legacy compiled data symbol: {name}")
    if original is not None and original in data:
        raise ValueError(f"original executable data embedded in: {name}")


def audit(path, original):
    data = path.read_bytes()
    check_bytes(data, path.name, original)
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as archive:
            names = archive.namelist()
            for name in names:
                check_name(name)
                if not name.endswith("/"):
                    check_bytes(archive.read(name), name, original)
                if path.suffix.lower() == ".vpk" and name not in VPK_ALLOWED and not name.startswith("licenses/"):
                    raise ValueError(f"unexpected VPK entry: {name}")
    if data[:4] == b"\0PBP":
        if len(data) < 40:
            raise ValueError("truncated PBP")
        offsets = struct.unpack_from("<8I", data, 8)
        if offsets[0] < 40 or any(a > b for a, b in zip(offsets, offsets[1:])) or offsets[-1] > len(data):
            raise ValueError("invalid PBP sections")
        art = Path(__file__).resolve().parents[1] / "psp/art_public"
        for index, name in ((1,"ICON0.PNG"), (3,"PIC0.PNG"), (4,"PIC1.PNG")):
            if data[offsets[index]:offsets[index + 1]] != (art / name).read_bytes():
                raise ValueError(f"PBP artwork differs from public {name}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="+", type=Path)
    parser.add_argument("--original-exe", type=Path)
    args = parser.parse_args()
    try:
        original = extract(args.original_exe.read_bytes())[0] if args.original_exe else None
        for path in args.files:
            audit(path, original)
            print(f"OK: {path}")
    except (OSError, ValueError, zipfile.BadZipFile) as exc:
        print(f"FAILED: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
