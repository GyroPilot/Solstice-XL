#!/usr/bin/env python3
"""mkworld.py: the world map for Solstice, from Natural Earth's 1:110m land (public domain, naturalearthdata.com).
    ~/serialenv/bin/python mkworld.py ne_110m_land.geojson ../src/worldmap.c [preview.png]
Equirectangular, 360 x 180 (one pixel per degree), 1 bit, black land on white sea, rowBytes 46 (even, as QuickDraw
wants).  Longitude -180 at the left, latitude +90 at the top."""
import json, sys
from PIL import Image, ImageDraw
W, H, RB = 360, 180, 46
src, out = sys.argv[1], sys.argv[2]
im = Image.new("1", (W * 4, H * 4), 1)                     # draw at 4x, then shrink, for clean coasts
d = ImageDraw.Draw(im)
def ring(coords): return [((lon + 180) * 4, (90 - lat) * 4) for lon, lat in coords]
for f in json.load(open(src))["features"]:
    g = f["geometry"]
    polys = [g["coordinates"]] if g["type"] == "Polygon" else g["coordinates"]
    for poly in polys:
        d.polygon(ring(poly[0]), fill=0)
        for hole in poly[1:]: d.polygon(ring(hole), fill=1)
im = im.convert("L").resize((W, H), Image.BOX).point(lambda v: 0 if v < 140 else 255).convert("1")
if len(sys.argv) > 3: im.resize((W * 2, H * 2)).save(sys.argv[3])
rows = []
for y in range(H):
    bits = [0 if im.getpixel((x, y)) else 1 for x in range(W)] + [0] * (RB * 8 - W)
    rows.append(bytes(int("".join(map(str, bits[i:i + 8])), 2) for i in range(0, RB * 8, 8)))
data = b"".join(rows)
with open(out, "w") as f:
    f.write("/* worldmap.c: written by tools/mkworld.py from Natural Earth 1:110m land (public domain).\n"
            " * 360 x 180, one pixel per degree, 1 = land; rowBytes 46. */\n")
    f.write("const unsigned char kWorldMap[%d] = {\n" % len(data))
    for i in range(0, len(data), 16):
        f.write("    " + ", ".join("0x%02X" % b for b in data[i:i + 16]) + ",\n")
    f.write("};\n")
print("wrote", out, len(data), "bytes")
