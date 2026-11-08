#!/usr/bin/env python3
"""mkpictures.py: the Pictures from History (Tim's choice, 3 Nov 2026, the same six as LisaSolstice REV C) for the
Mac: each original from art/history scaled to 250 pixels high for square pixels, grey, contrast stretched 2-98 %;
the two engravings by a threshold (line art), the postcard by Floyd-Steinberg dither.  Writes src/pictures.c (kPicBits, kPicW, kPicH, kPicRow, kPicCap1, kPicCap2) and
art/history/mac_preview.png.   ~/serialenv/bin/python tools/mkpictures.py"""
from PIL import Image, ImageOps
PICS = [  # file, caption line 1, line 2 (the banana's wording, word for word), method.  Oldest site first (LisaSolstice REV C).
    ("L_wakeman_newgrange_1903.png", "Newgrange, Ireland, about 3200 BC: the passage and chamber, plan and side.",
     "At midwinter sunrise a beam runs up the passage.  Wakeman, 1903.", "threshold"),
    ("K_2013_solstice_newgrange_dentp.jpg", "Midwinter sunlight in the Newgrange chamber, December 2013.",
     "Photo: Dentp, CC BY-SA 4.0 (Wikimedia Commons).", "dither"),
    ("P_stonehenge_cloudy_sunset_pfau.jpg", "Stonehenge, about 2500 BC: its great stones frame the midwinter sunset.",
     "Photo: Jeffrey Pfau, CC BY-SA 3.0 (Wikimedia Commons).", "dither"),
    ("B_serpent_mound_postcard_original.jpeg", "Ohio's Great Serpent Mound on an old postcard, 70 miles east of Cincinnati.",
     "Its head points to the summer solstice sunset.", "dither"),
    ("A_yule_log_original.jpeg", "Bringing in the Yule log: a wood engraving of the 1800s.",
     "It burned the old year away; a piece lit next year's log.", "threshold"),
    ("H_heavens_engraving_original.jpg", "The heavens in an engraving of the 1600s: the Earth in the middle,",
     "then the air, the Moon, the Sun and the stars.", "threshold"),
]
H = 250
out, previews = [], []
for f, c1, c2, how in PICS:
    im = Image.open("art/history/" + f).convert("RGB").convert("L")
    w = round(im.width * H / im.height)
    im = ImageOps.autocontrast(im.resize((w, H), Image.LANCZOS), cutoff=2)
    # engravings are line art: a threshold keeps the lines; the photographic postcard is dithered
    bw = im.point(lambda v: 255 if v > 150 else 0).convert("1", dither=Image.NONE) if how == "threshold" else im.convert("1", dither=Image.FLOYDSTEINBERG)
    row = (w + 15) // 16 * 2
    data = bytearray(row * H)
    for y in range(H):
        for x in range(w):
            if bw.getpixel((x, y)) == 0: data[y * row + x // 8] |= 0x80 >> (x & 7)
    out.append((data, w, row, c1, c2)); previews.append(bw)
def cstr(s): return '"' + s.encode("mac_roman").decode("latin-1").replace("\\", "\\\\").replace('"', '\\"') + '"'
with open("src/pictures.c", "w") as f:
    f.write("/* pictures.c: Pictures from History, by tools/mkpictures.py (public-domain engravings and an old postcard). */\n")
    for i, (data, w, row, c1, c2) in enumerate(out):
        f.write("static const unsigned char kPic%d[%d] = {\n" % (i, len(data)))
        for j in range(0, len(data), 24): f.write("    " + ",".join("0x%02X" % b for b in data[j:j + 24]) + ",\n")
        f.write("};\n")
    f.write("const unsigned char *const kPicBits[] = { %s };\n" % ", ".join("kPic%d" % i for i in range(len(out))))
    f.write("const short kPicW[] = { %s };\n" % ", ".join(str(o[1]) for o in out))
    f.write("const short kPicRow[] = { %s };\n" % ", ".join(str(o[2]) for o in out))
    f.write("const short kPicH = %d, kNumPics = %d;\n" % (H, len(out)))
    f.write("const char *const kPicCap1[] = { %s };\n" % ", ".join(cstr(o[3]) for o in out))
    f.write("const char *const kPicCap2[] = { %s };\n" % ", ".join(cstr(o[4]) for o in out))
sheet = Image.new("1", (sum(p.width for p in previews) + 20 * len(previews), H + 20), 1)
x = 10
for p in previews: sheet.paste(p, (x, 10)); x += p.width + 20
sheet.save("art/history/mac_preview.png")
print([(o[1], len(o[0])) for o in out])
