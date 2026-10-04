#!/usr/bin/env python3
"""Extract TENNIS.DAT: header = u16 count, u32 dir_pos; dir = count * 64-byte entries
(NUL-terminated path in first 56 bytes, then u32 size, u32 offset)."""
import struct, sys, os
src, out = sys.argv[1], sys.argv[2]
d = open(src, 'rb').read()
n, dp = struct.unpack('<HI', d[:6])
for i in range(n):
    e = d[dp+i*64: dp+i*64+64]
    name = e[:56].split(b'\0')[0].decode('latin1').replace('\\', '/')
    size, off = struct.unpack('<II', e[56:64])
    p = os.path.join(out, name)
    os.makedirs(os.path.dirname(p) or '.', exist_ok=True)
    open(p, 'wb').write(d[off:off+size])
    print(f'{i:4d} {off:8d} {size:8d} {name}')
