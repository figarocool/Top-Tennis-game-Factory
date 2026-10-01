"""Decoder for .CBE 'compiled bitmaps' (x86 code that stores into mode-X planes).
Layout: 2 header bytes, then code: C6 /0 (mov byte [si+d],imm8), C7 /0 (mov word),
D0 C0 83 D6 00 EE (next plane: rol al,1; adc si,0; out dx,al), CB (retf).
Rows are 104 bytes apart (416 px wide virtual screen). Origin = top-left of sprite; header = (width_in_bytes, height)."""
import struct
STRIDE = 104

def decode(data):
    """Return (header, [(plane_group, disp, value)])"""
    hdr = data[:2]
    i, g, out = 2, 0, []
    while i < len(data):
        op = data[i]
        if op in (0xC6, 0xC7):
            m = data[i+1]
            if m == 0x44:   d = struct.unpack('<b', data[i+2:i+3])[0]; i += 3
            elif m == 0x84: d = struct.unpack('<h', data[i+2:i+4])[0]; i += 4
            elif m == 0x04: d = 0; i += 2
            else: raise ValueError(f'modrm {m:02x} at {i}')
            if op == 0xC6: out.append((g, d, data[i])); i += 1
            else:
                out.append((g, d, data[i])); out.append((g, d+1, data[i+1])); i += 2
        elif data[i:i+6] == b'\xD0\xC0\x83\xD6\x00\xEE':
            g += 1; i += 6
        elif op == 0xCB: break
        else: raise ValueError(f'opcode {op:02x} at {i}')
    return hdr, out

def to_pixels(data):
    """-> (w, h, {(x,y): colour}) with top-left of bounding box as origin, plus (ox, oy) origin offsets"""
    hdr, ops = decode(data)
    pts = {}
    for g, d, v in ops:
        row, col = divmod(d + 128, STRIDE)   # caller adds +0x80 to SI (1018:02ca), so real offset = d+128
        pts[(col*4 + g, row)] = v
    return hdr, pts
