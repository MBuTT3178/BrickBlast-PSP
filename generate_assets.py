#!/usr/bin/env python3
import zlib
import struct

def make_png(width, height, get_pixel_func):
    raw = bytearray()
    for y in range(height):
        raw.append(0) # filter type 0: None
        for x in range(width):
            r, g, b, a = get_pixel_func(x, y, width, height)
            raw.extend((r, g, b, a))
    
    compressed = zlib.compress(bytes(raw), 9)
    
    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xffffffff)
    
    png = bytearray(b"\x89PNG\r\n\x1a\n")
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    png.extend(chunk(b"IHDR", ihdr))
    png.extend(chunk(b"IDAT", compressed))
    png.extend(chunk(b"IEND", b""))
    return bytes(png)

# ICON0.PNG: 144 x 80
# Sleek dark badge with colorful 2x2 gem blocks in center
def icon_pixel(x, y, w, h):
    # Dark rounded background
    dx = min(x, w - 1 - x)
    dy = min(y, h - 1 - y)
    if dx < 2 and dy < 2:
        return (0, 0, 0, 0) # rounded corner
    if dx < 3 or dy < 3:
        return (40, 45, 65, 255) # border
    
    # Center area: 4 gem blocks (Cyan, Pink, Green, Gold)
    # Center is at x=72, y=40. Each block is 16x16
    cx, cy = 72, 40
    bx = (x - (cx - 18))
    by = (y - (cy - 18))
    
    if 0 <= bx < 36 and 0 <= by < 36:
        col = bx // 18
        row = by // 18
        lx = bx % 18
        ly = by % 18
        
        # 1px border between blocks
        if lx == 0 or lx == 17 or ly == 0 or ly == 17:
            return (18, 18, 28, 255)
        
        # Color based on quad
        palette = [
            (0, 215, 255),   # Top-left: Cyan
            (255, 75, 130),  # Top-right: Pink
            (50, 220, 85),   # Bottom-left: Green
            (255, 175, 30),  # Bottom-right: Gold
        ]
        base = palette[row * 2 + col]
        
        # Highlight top-left of block
        if lx < 3 or ly < 3:
            return tuple(min(255, c + 50) for c in base) + (255,)
        # Shadow bottom-right
        if lx >= 15 or ly >= 15:
            return tuple(max(0, c - 60) for c in base) + (255,)
        # Inner gloss dot
        if 4 <= lx <= 6 and 4 <= ly <= 6:
            return (255, 255, 255, 255)
        return base + (255,)
    
    # Background gradient
    bg_val = 16 + (y * 12 // h)
    return (bg_val, bg_val + 2, bg_val + 10, 255)

# PIC1.PNG: 480 x 272 Wallpaper
def pic1_pixel(x, y, w, h):
    # Deep gradient background
    base_b = 25 + (y * 30 // h)
    base_g = 14 + (y * 15 // h)
    base_r = 12 + (y * 12 // h)
    
    # Subtle geometric grid pattern
    if (x % 24 == 0) or (y % 24 == 0):
        return (base_r + 10, base_g + 12, base_b + 18, 255)
    
    return (base_r, base_g, base_b, 255)

with open("/mnt/c/Users/amv31/Downloads/block_blast_psp/ICON0.PNG", "wb") as f:
    f.write(make_png(144, 80, icon_pixel))
print("ICON0.PNG written (144x80)")

with open("/mnt/c/Users/amv31/Downloads/block_blast_psp/PIC1.PNG", "wb") as f:
    f.write(make_png(480, 272, pic1_pixel))
print("PIC1.PNG written (480x272)")
