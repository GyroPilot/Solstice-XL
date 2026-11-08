#!/usr/bin/env python3
"""mksplash.py: Tim's solargraph (art/solargraph_mac_512x325.png, 1-bit, made by the banana's art/mac_versions.py)
-> src/splash.c, kSplash: 512 x 325, rowBytes 64, black = 1.   ~/serialenv/bin/python tools/mksplash.py"""
from PIL import Image
im = Image.open("art/solargraph_mac_512x325.png").convert("1")
W, H = im.size; assert (W, H) == (512, 325)
rows = []
for y in range(H):
    b = bytearray(W // 8)
    for x in range(W):
        if im.getpixel((x, y)) == 0: b[x >> 3] |= 0x80 >> (x & 7)
    rows.append(b)
with open("src/splash.c", "w") as f:
    f.write("/* splash.c: Tim's solargraph (June to December solstice, pop-can pinhole camera, Cincinnati), 512 x 325, by tools/mksplash.py */\n")
    f.write("const unsigned char kSplash[%d] = {\n" % (64 * H))
    for b in rows: f.write("".join("0x%02X," % v for v in b) + "\n")
    f.write("};\n")
print("src/splash.c", 64 * H, "bytes")
