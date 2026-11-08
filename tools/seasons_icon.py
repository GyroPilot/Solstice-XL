"""The LisaSolstice four-seasons icon redrawn for the classic Mac: 32 x 32, square pixels.  Writes
seasons_mac_32.png, seasons_mac_32_x8.png and seasons_icn.r (an 'ICN#' resource: icon then mask)."""
from PIL import Image
import math
W = H = 32
px = [[0] * W for _ in range(H)]
def put(x, y, v=1):
    if 0 <= x < W and 0 <= y < H: px[y][x] = v
def disc(cx, cy, r, test=lambda dx, dy: True, v=1):
    for y in range(H):
        for x in range(W):
            dx, dy = x + 0.5 - cx, y + 0.5 - cy
            if dx * dx + dy * dy <= r * r and test(dx, dy): put(x, y, v)
def ring(cx, cy, r):
    for y in range(H):
        for x in range(W):
            d = math.hypot(x + 0.5 - cx, y + 0.5 - cy)
            if r - 0.5 <= d <= r + 0.5: put(x, y)
cx = cy = 16.0
er, off = 4.6, 10.6
for ex, ey in ((cx - off, cy), (cx + off, cy), (cx, cy - off), (cx, cy + off)):
    sx, sy = cx - ex, cy - ey
    ring(ex, ey, er)
    disc(ex, ey, er, lambda dx, dy, sx=sx, sy=sy: dx * sx + dy * sy < 0)   # night half, away from the Sun
disc(cx, cy, 2.2)
for k in (1, 3, 5, 7):                       # diagonal rays only, clear of the four Earths
    t = math.pi * k / 4
    for d in (3.6, 4.5):
        put(int(math.floor(cx + d * math.cos(t))), int(math.floor(cy + d * math.sin(t))))
im = Image.new('L', (W, H), 255)
for y in range(H):
    for x in range(W):
        if px[y][x]: im.putpixel((x, y), 0)
im.save('seasons_mac_32.png'); im.resize((W * 8, H * 8), Image.NEAREST).save('seasons_mac_32_x8.png')
# mask: the icon with each Earth and the Sun filled
mask = [row[:] for row in px]
out = [[0] * W for _ in range(H)]
stack = [(x, y) for x in range(W) for y in (0, H - 1)] + [(x, y) for y in range(H) for x in (0, W - 1)]
while stack:
    x, y = stack.pop()
    if not (0 <= x < W and 0 <= y < H) or out[y][x] or px[y][x]: continue
    out[y][x] = 1; stack += [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)]
mask = [[0 if out[y][x] else 1 for x in range(W)] for y in range(H)]
def hexrows(b):
    words = ''.join('%08X' % int(''.join(map(str, row)), 2) for row in b)
    return ['\t$"%s"' % words[i:i + 64] for i in range(0, len(words), 64)]
with open('seasons_icn.r', 'w') as f:
    f.write("/* The LisaSolstice four-seasons icon for the Mac (from the banana, 2 Nov 2026): icon, then mask. */\n")
    f.write("data 'ICN#' (128, \"Solstice\") {\n" + "\n".join(hexrows(px) + hexrows(mask)) + "\n};\n")
for row in px: print(''.join('#' if v else '.' for v in row))
