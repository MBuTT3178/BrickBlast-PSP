#!/usr/bin/env python3
import struct
import zlib

W = 480
H = 272
SCALE = 2 # 960x544 for crystal-clear high-DPI display on GitHub

buf = [[(8, 14, 36) for _ in range(W)] for _ in range(H)]

def set_pixel(x, y, color):
    if 0 <= x < W and 0 <= y < H:
        buf[y][x] = color

def fill_rect(x, y, w, h, color):
    for j in range(max(0, y), min(H, y + h)):
        for i in range(max(0, x), min(W, x + w)):
            buf[j][i] = color

def draw_border(x, y, w, h, thick, color):
    fill_rect(x, y, w, thick, color)
    fill_rect(x, y + h - thick, w, thick, color)
    fill_rect(x, y, thick, h, color)
    fill_rect(x + w - thick, y, thick, h, color)

def col_lighten(col, p):
    return tuple(min(255, c + (255 - c) * p // 100) for c in col)

def col_darken(col, p):
    return tuple(max(0, c * (100 - p) // 100) for c in col)

font8x8 = [
    [0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00], # 32 SPC
    [0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00], # 33 !
    [0x6C,0x6C,0x00,0x00,0x00,0x00,0x00,0x00], # 34 "
    [0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00], # 35 #
    [0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00], # 36 $
    [0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00], # 37 %
    [0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00], # 38 &
    [0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00], # 39 '
    [0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00], # 40 (
    [0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00], # 41 )
    [0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00], # 42 *
    [0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00], # 43 +
    [0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30], # 44 ,
    [0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00], # 45 -
    [0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00], # 46 .
    [0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00], # 47 /
    [0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00], # 48 0
    [0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00], # 49 1
    [0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00], # 50 2
    [0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00], # 51 3
    [0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00], # 52 4
    [0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00], # 53 5
    [0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00], # 54 6
    [0x7E,0x06,0x0C,0x18,0x18,0x18,0x18,0x00], # 55 7
    [0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00], # 56 8
    [0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00], # 57 9
    [0x00,0x00,0x18,0x00,0x00,0x18,0x00,0x00], # 58 :
    [0x00,0x00,0x18,0x00,0x00,0x18,0x18,0x30], # 59 ;
    [0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00], # 60 <
    [0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00], # 61 =
    [0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00], # 62 >
    [0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00], # 63 ?
    [0x3C,0x66,0x6E,0x6E,0x60,0x62,0x3C,0x00], # 64 @
    [0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00], # 65 A
    [0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00], # 66 B
    [0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00], # 67 C
    [0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00], # 68 D
    [0x7E,0x60,0x60,0x78,0x60,0x60,0x7E,0x00], # 69 E
    [0x7E,0x60,0x60,0x78,0x60,0x60,0x60,0x00], # 70 F
    [0x3C,0x66,0x60,0x6E,0x66,0x66,0x3E,0x00], # 71 G
    [0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00], # 72 H
    [0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00], # 73 I
    [0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00], # 74 J
    [0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00], # 75 K
    [0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00], # 76 L
    [0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00], # 77 M
    [0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00], # 78 N
    [0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00], # 79 O
    [0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00], # 80 P
    [0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00], # 81 Q
    [0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00], # 82 R
    [0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00], # 83 S
    [0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00], # 84 T
    [0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00], # 85 U
    [0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00], # 86 V
    [0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00], # 87 W
    [0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00], # 88 X
    [0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00], # 89 Y
    [0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00], # 90 Z
]

def draw_char(x, y, ch, color, scale=1):
    c = ord(ch.upper())
    if c < 32 or c > 90: return
    glyph = font8x8[c - 32]
    for row in range(8):
        bits = glyph[row]
        for col in range(8):
            if bits & (0x80 >> col):
                if scale == 1:
                    set_pixel(x + col, y + row, color)
                else:
                    fill_rect(x + col * scale, y + row * scale, scale, scale, color)

def draw_str(x, y, text, color, scale=1):
    cx = x
    for ch in text:
        draw_char(cx, y, ch, color, scale)
        cx += 8 * scale

def draw_socket(x, y, sz):
    fill_rect(x, y, sz, sz, (20, 30, 68))
    draw_border(x, y, sz, sz, 1, (14, 20, 48))

def draw_gem_block(x, y, sz, base_col):
    fill_rect(x, y, sz, sz, base_col)
    light = col_lighten(base_col, 55)
    dark = col_darken(base_col, 60)
    # top & left highlight
    fill_rect(x, y, sz, 2, light)
    fill_rect(x, y, 2, sz, light)
    # bottom & right shadow
    fill_rect(x, y + sz - 2, sz, 2, dark)
    fill_rect(x + sz - 2, y, 2, sz, dark)
    # inner gloss reflection
    fill_rect(x + 4, y + 4, 3, 3, (255, 255, 255))

# Palette matching piece_colors from main.c
PIECE_COLS = [
    (0, 210, 255),   # 0: Cyan
    (255, 140, 0),   # 1: Orange
    (220, 40, 220),  # 2: Magenta
    (255, 210, 0),   # 3: Yellow
    (40, 220, 80),   # 4: Green
    (40, 100, 255),  # 5: Blue
    (255, 50, 60),   # 6: Red
    (140, 50, 240),  # 7: Purple
]

# 1. Background gradient
for y in range(H):
    t = y * 100 // H
    r = (12 * (100 - t) + 8 * t) // 100
    g = (20 * (100 - t) + 14 * t) // 100
    b = (52 * (100 - t) + 36 * t) // 100
    for x in range(W):
        buf[y][x] = (r, g, b)

# 2. Header
# Logo
draw_str(14, 6, "BRICK", (255, 255, 255), 1)
draw_str(58, 6, "BLAST", (0, 240, 255), 1)

# Streak + Pips
draw_str(14, 20, "STREAK x3", (255, 180, 40), 1)
# 3 pips
for p in range(3):
    px = 96 + p * 8
    fill_rect(px, 22, 5, 5, (255, 180, 40))
    draw_border(px, 22, 5, 5, 1, (255, 255, 255))

# Center Score
score_str = "3850"
sw = len(score_str) * 24
sx = 240 - sw // 2
draw_str(sx + 2, 6, score_str, (10, 16, 42), 3) # drop shadow
draw_str(sx, 4, score_str, (255, 255, 255), 3)    # main score

# Right HUD
draw_str(346, 6, "# BEST", (255, 215, 30), 1)
draw_str(402, 6, "14200", (255, 255, 255), 1)
draw_str(346, 20, "COINS", (255, 215, 30), 1)
draw_str(402, 20, "450", (255, 215, 30), 1)

# 3. 8x8 Grid Board
BOARD_X = 16
BOARD_Y = 40
BOARD_BORDER = 4
BOARD_TOTAL = 222
GRID_N = 8
CELL_SZ = 25
CELL_GAP = 2

draw_border(BOARD_X, BOARD_Y, BOARD_TOTAL, BOARD_TOTAL, BOARD_BORDER, (30, 48, 88))
fill_rect(BOARD_X + BOARD_BORDER, BOARD_Y + BOARD_BORDER, BOARD_TOTAL - 8, BOARD_TOTAL - 8, (14, 22, 54))

# Setup lively board layout
board = [
    [0, 1, 1, 0, 0, 2, 2, 0],
    [3, 3, 1, 0, 0, 2, 2, 0],
    [3, 3, 4, 4, 4, 0, 5, 5],
    [0, 6, 6, 4, 0, 0, 5, 5],
    [0, 6, 6, 0, 7, 7, 7, 0],
    [8, 8, 8, 8, 8, 8, 8, 8], # Row 5: line clear flash!
    [1, 1, 0, 2, 2, 3, 3, 0],
    [1, 1, 0, 2, 2, 3, 3, 0],
]

for r in range(GRID_N):
    for c in range(GRID_N):
        cx = BOARD_X + BOARD_BORDER + c * (CELL_SZ + CELL_GAP)
        cy = BOARD_Y + BOARD_BORDER + r * (CELL_SZ + CELL_GAP)
        val = board[r][c]
        if r == 5:
            # White line clear flash
            fill_rect(cx, cy, CELL_SZ, CELL_SZ, (255, 255, 255))
        elif val > 0:
            draw_gem_block(cx, cy, CELL_SZ, PIECE_COLS[(val - 1) % len(PIECE_COLS)])
        else:
            draw_socket(cx, cy, CELL_SZ)

# Ghost preview of 2x2 Cyan piece at (r=2, c=3)
ghost_col = (0, 210, 255)
for gr in range(2):
    for gc in range(2):
        gx = BOARD_X + BOARD_BORDER + (3 + gc) * (CELL_SZ + CELL_GAP)
        gy = BOARD_Y + BOARD_BORDER + (2 + gr) * (CELL_SZ + CELL_GAP)
        draw_border(gx, gy, CELL_SZ, CELL_SZ, 2, ghost_col)
        # inner soft tint
        for j in range(gy + 2, gy + CELL_SZ - 2):
            for i in range(gx + 2, gx + CELL_SZ - 2):
                buf[j][i] = (20, 60, 100)

# Combo fanfare pill at bottom
banner = "DOUBLE! STREAK 3"
len_b = len(banner)
text_scale = 1
banner_w = len_b * 8 * text_scale
bx = BOARD_X + (BOARD_TOTAL - banner_w) // 2
by = BOARD_Y + BOARD_TOTAL - 18
fill_rect(bx - 8, by - 4, banner_w + 16, 16, (10, 16, 44))
draw_border(bx - 8, by - 4, banner_w + 16, 16, 2, (255, 140, 20))
draw_str(bx + 1, by + 1, banner, (12, 16, 36), 1)
draw_str(bx, by, banner, (255, 215, 30), 1)

# Floating score & coin popups centered over the clear
draw_str(BOARD_X + 76, BOARD_Y + 112, "+350", (10, 16, 40), 2)
draw_str(BOARD_X + 75, BOARD_Y + 110, "+350", (0, 240, 255), 2)
draw_str(BOARD_X + 71, BOARD_Y + 132, "+18 COINS", (255, 215, 30), 1)

# 4. Right Tray Panel
PANEL_X = 254
PANEL_Y = 40
PANEL_W = 212
PANEL_H = 222
draw_border(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, 2, (30, 48, 88))
fill_rect(PANEL_X + 2, PANEL_Y + 2, PANEL_W - 4, PANEL_H - 4, (14, 22, 54))

slot_w = PANEL_W - 16
slot_h = 48
slot_x = PANEL_X + 8

# Slot 0: Selected 2x2 Cyan square with glowing border
sy0 = PANEL_Y + 10
fill_rect(slot_x, sy0, slot_w, slot_h, (10, 16, 38))
draw_border(slot_x - 1, sy0 - 1, slot_w + 2, slot_h + 2, 2, (0, 240, 255))
# draw 2x2 cyan
for br in range(2):
    for bc in range(2):
        draw_gem_block(slot_x + 85 + bc * 14, sy0 + 10 + br * 14, 13, (0, 210, 255))

# Slot 1: Unselected 3x1 bar (Orange)
sy1 = PANEL_Y + 65
fill_rect(slot_x, sy1, slot_w, slot_h, (10, 16, 38))
draw_border(slot_x, sy1, slot_w, slot_h, 1, (30, 48, 88))
for bc in range(3):
    draw_gem_block(slot_x + 78 + bc * 14, sy1 + 17, 13, col_darken((255, 140, 0), 20))

# Slot 2: Unselected 3x3 L piece (Magenta)
sy2 = PANEL_Y + 120
fill_rect(slot_x, sy2, slot_w, slot_h, (10, 16, 38))
draw_border(slot_x, sy2, slot_w, slot_h, 1, (30, 48, 88))
for br, bc in [(0,0), (1,0), (2,0), (2,1), (2,2)]:
    draw_gem_block(slot_x + 78 + bc * 14, sy2 + 5 + br * 14, 13, col_darken((220, 40, 220), 20))

# Hints at bottom
fill_rect(PANEL_X + 8, PANEL_Y + 180, PANEL_W - 16, 1, (30, 48, 88))
draw_str(PANEL_X + 16, PANEL_Y + 188, "D-PAD: MOVE", (120, 140, 175), 1)
draw_str(PANEL_X + 16, PANEL_Y + 202, "L/R: SELECT  X: DROP", (255, 255, 255), 1)

# Encode to 2x scaled PNG (960x544)
out_w = W * SCALE
out_h = H * SCALE
raw_lines = bytearray()
for y in range(out_h):
    raw_lines.append(0) # filter None
    src_y = y // SCALE
    row = buf[src_y]
    for x in range(out_w):
        src_x = x // SCALE
        r, g, b = row[src_x]
        raw_lines.extend((r, g, b, 255))

compressed = zlib.compress(bytes(raw_lines), 9)

def chunk(tag, data):
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

png_bytes = bytearray(b"\x89PNG\r\n\x1a\n")
ihdr = struct.pack(">IIBBBBB", out_w, out_h, 8, 6, 0, 0, 0)
png_bytes.extend(chunk(b"IHDR", ihdr))
png_bytes.extend(chunk(b"IDAT", compressed))
png_bytes.extend(chunk(b"IEND", b""))

out_path = "screenshot.png"
with open(out_path, "wb") as f:
    f.write(png_bytes)

print(f"Generated {out_path} ({out_w}x{out_h}, {len(png_bytes)} bytes)")
