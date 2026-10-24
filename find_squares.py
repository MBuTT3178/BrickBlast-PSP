import os
import struct

sprites_dir = r"C:\Users\amv31\Downloads\block_blast_apk_dump\sprites"

def get_png_size(path):
    with open(path, "rb") as f:
        data = f.read(24)
        if data[:8] == b"\x89PNG\r\n\x1a\n":
            w, h = struct.unpack(">II", data[16:24])
            return w, h
    return None

squares = []
for f in os.listdir(sprites_dir):
    if f.endswith(".png"):
        p = os.path.join(sprites_dir, f)
        res = get_png_size(p)
        if res:
            w, h = res
            if w == h and 32 <= w <= 256:
                squares.append((w, h, os.path.getsize(p), f))

print(f"Square sprites ({len(squares)}):")
for w, h, sz, f in sorted(squares, key=lambda x: (x[0], x[2]))[:40]:
    print(f"  {w}x{h} ({sz} bytes): {f}")
