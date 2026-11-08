#!/usr/bin/env python3
"""mkicon.py: Solstice's icons (original art): the Sun over a horizon with the Earth's tilted axis; and a page for
the settings file.  Writes src/icons.r.  ~/serialenv/bin/python tools/mkicon.py src/icons.r"""
import math, sys
def canvas(): return [[0] * 32 for _ in range(32)]
def px(c, x, y):
    if 0 <= x < 32 and 0 <= y < 32: c[y][x] = 1
def line(c, x0, y0, x1, y1):
    n = max(abs(x1 - x0), abs(y1 - y0)) or 1
    for i in range(n + 1): px(c, round(x0 + (x1 - x0) * i / n), round(y0 + (y1 - y0) * i / n))
def app():
    c = canvas()
    cx, cy, r = 15.5, 15.5, 7.5
    for y in range(32):                       # the Sun: a filled disc
        for x in range(32):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r * r: px(c, x, y)
    for i in range(12):                       # twelve rays, like a clock face of months
        a = i * math.pi / 6
        line(c, round(cx + 10 * math.cos(a)), round(cy + 10 * math.sin(a)), round(cx + 14 * math.cos(a)), round(cy + 14 * math.sin(a)))
    for y in range(32):                       # the Earth's tilted axis through it, as a white cut
        x = round(cx + (y - cy) * math.tan(math.radians(23.4)))
        if abs(y - cy) < r and 0 <= x < 32: c[y][x] = 0
    return c
def mask(c):
    out = [[0] * 32 for _ in range(32)]
    st = [(x, y) for x in range(32) for y in (0, 31)] + [(x, y) for y in range(32) for x in (0, 31)]
    while st:
        x, y = st.pop()
        if not (0 <= x < 32 and 0 <= y < 32) or out[y][x] or c[y][x]: continue
        out[y][x] = 1; st += [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)]
    m = [[0 if out[y][x] else 1 for x in range(32)] for y in range(32)]
    for y in range(32):                       # close the gaps between the rays so the icon selects as one
        for x in range(32):
            if (x - 15.5) ** 2 + (y - 15.5) ** 2 <= 14.5 ** 2: m[y][x] = 1
    return m
def hexrows(c): return "".join("".join("%02X" % int("".join(str(b) for b in row[i:i + 8]), 2) for i in range(0, 32, 8)) for row in c)
a = app(); h = hexrows(a) + hexrows(mask(a))
lines = "\n".join('\t$"%s"' % h[i:i + 64] for i in range(0, len(h), 64))
open(sys.argv[1], "w").write("/* icons.r: written by tools/mkicon.py; original art. */\ndata 'ICN#' (128, \"Solstice\") {\n%s\n};\n" % lines)
for row in a: print("".join("#" if b else "." for b in row))
