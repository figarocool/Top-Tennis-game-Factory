#!/usr/bin/env python3
"""Prepare a user's own TENNIS.DAT for this engine; never emit distributable game data.

The original DAT is preserved byte for byte as the prefix of a new DAT. The
extension holds data read from the user's EXE, not executable code. No downloads,
extra dependencies, compiled-in game data or changes to the original files.
"""
import argparse
from pathlib import Path
import struct
import sys

MAGIC = b"TTDSv1\r\n"
FOOTER = struct.Struct("<8sIIII")
DS_SIZE = 19876
DS_CHECK = 0x3994E248
RELOC_CHECK = 0x265717BD


def fnv(data):
    h = 2166136261
    for byte in data:
        h = ((h ^ byte) * 16777619) & 0xFFFFFFFF
    return h


def extract(exe):
    def check(at, length):
        if at < 0 or length < 0 or at + length > len(exe):
            raise ValueError("truncated or invalid TENNIS.EXE / EXE troncato o non valido")

    def u16(at):
        check(at, 2)
        return struct.unpack_from("<H", exe, at)[0]

    check(0, 64)
    if len(exe) > 4 * 1024 * 1024 or exe[:2] != b"MZ":
        raise ValueError("invalid TENNIS.EXE")
    ne = struct.unpack_from("<I", exe, 0x3C)[0]
    check(ne, 64)
    if exe[ne:ne + 2] != b"NE":
        raise ValueError("expected the original NE executable / serve l'EXE NE originale")
    index, count, shift = u16(ne + 14), u16(ne + 28), u16(ne + 50)
    table = ne + u16(ne + 34)
    if not 1 <= index <= count or shift > 16:
        raise ValueError("invalid NE segment table")
    check(table, count * 8)
    entry = table + (index - 1) * 8
    offset = u16(entry) << shift
    size = u16(entry + 2) or 65536
    flags = u16(entry + 4)
    check(offset, size + 2)
    if flags & 0x101 != 0x101:
        raise ValueError("invalid NE data segment")
    data = exe[offset:offset + size]
    if size != DS_SIZE or fnv(data) != DS_CHECK:
        raise ValueError("unsupported original version / versione originale non supportata")
    number = u16(offset + size)
    records = offset + size + 2
    check(records, number * 8)
    rel = {}
    for i in range(number):
        kind, flags, source, target_seg, target = struct.unpack_from("<BBHHH", exe, records + i * 8)
        if flags & 3 or target_seg != index or kind not in (3, 5):
            continue
        if source % 2 or source + (4 if kind == 3 else 2) > size or not 0 < target < size or source in rel:
            raise ValueError("invalid or duplicate relocation")
        rel[source] = target
    pairs = b"".join(struct.pack("<HH", source, target) for source, target in sorted(rel.items()))
    if len(rel) != 101 or fnv(pairs) != RELOC_CHECK:
        raise ValueError("unsupported relocation layout")
    return data, pairs


def validate_dat(dat):
    if len(dat) >= FOOTER.size and dat[-FOOTER.size:-FOOTER.size + 8] == MAGIC:
        raise ValueError("DAT already prepared; use the original DAT / DAT gia preparato: usa l'originale")
    if not 6 <= len(dat) <= 0xFFFFFFFF:
        raise ValueError("invalid TENNIS.DAT size")
    count, directory = struct.unpack_from("<HI", dat)
    if not count or directory < 6 or directory + count * 64 > len(dat):
        raise ValueError("invalid TENNIS.DAT directory")
    for i in range(count):
        size, offset = struct.unpack_from("<II", dat, directory + i * 64 + 56)
        if offset < 6 or offset + size > directory:
            raise ValueError("invalid TENNIS.DAT entry")


def find_file(folder, name):
    for path in folder.iterdir():
        if path.name.casefold() == name.casefold() and path.is_file():
            return path
    raise ValueError(f"{name} not found / non trovato in {folder}")


def prepare(folder, output):
    source = find_file(folder, "TENNIS.DAT")
    exe_path = find_file(folder, "TENNIS.EXE")
    if output.resolve() in (source.resolve(), exe_path.resolve()):
        raise ValueError("output must be a new file / l'output deve essere un nuovo file")
    if output.exists():
        raise ValueError(f"refusing to overwrite / non sovrascrivo: {output}")
    with exe_path.open("rb") as stream:
        exe = stream.read(4 * 1024 * 1024 + 1)
    data, pairs = extract(exe)
    # The supported game DAT is small; reject huge inputs before reading them.
    if source.stat().st_size > 64 * 1024 * 1024:
        raise ValueError("TENNIS.DAT is too large")
    dat = source.read_bytes()
    validate_dat(dat)
    payload = data + pairs
    footer = FOOTER.pack(MAGIC, len(dat), len(data), len(pairs) // 4, fnv(payload))
    output.parent.mkdir(parents=True, exist_ok=True)
    # Exclusive create also prevents an overwrite if the path appeared meanwhile.
    with output.open("xb") as stream:
        stream.write(dat)
        stream.write(payload)
        stream.write(footer)
    return len(dat), len(payload) + len(footer)


def main():
    parser = argparse.ArgumentParser(description="Prepare your own Top Tennis data / Prepara i dati della tua copia originale")
    parser.add_argument("original_folder", type=Path, help="folder with your original TENNIS.EXE and TENNIS.DAT")
    parser.add_argument("--output", type=Path, required=True, help="new TENNIS.DAT, e.g. prepared/TENNIS.DAT")
    args = parser.parse_args()
    try:
        size, extra = prepare(args.original_folder, args.output)
    except (OSError, ValueError, struct.error) as exc:
        print(f"Error / Errore: {exc}", file=sys.stderr)
        return 1
    print(f"Prepared / Preparato: {args.output} ({size} + {extra} bytes)")
    print("Copy this TENNIS.DAT to the game folder; TENNIS.EXE is no longer needed there.")
    print("Copia questo TENNIS.DAT nella cartella del gioco: li non serve piu TENNIS.EXE.")
    print("Keep the result for your own use; do not upload or redistribute original game data.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
