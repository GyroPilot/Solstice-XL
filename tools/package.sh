#!/bin/bash
# package.sh VERSION: build/Solstice.dsk -> dist/ (.dc42, .bin, view PNGs) and dist/extfs/"Solstice VERSION (test)" (unpacked
# app + READ ME, ExtFS layout for put_mac_files.sh). Run build.sh and test/run_qemu.sh 160 first (the PNGs come from test/out).
set -e; V="$1"; [ -n "$V" ] || { echo "usage: package.sh 0.1d2"; exit 1; }
cd "$(dirname "$0")/.."; PY=~/serialenv/bin/python; M=/Volumes/RetroDev/src/moof-setup
$PY - "$V" <<'EOF'
import struct, sys, shutil
from PIL import Image, ImageDraw
V = sys.argv[1]
img = open("build/Solstice.dsk", "rb").read(); assert len(img) == 819200
c = 0
for (w,) in struct.iter_unpack(">H", img):
    c = (c + w) & 0xFFFFFFFF; c = ((c >> 1) | (c << 31)) & 0xFFFFFFFF
n = ("Solstice " + V).encode()
hdr = bytes([len(n)]) + n + bytes(63 - len(n)) + struct.pack(">IIIIBBH", len(img), 0, c, 0, 1, 0x22, 0x0100)
open("dist/Solstice %s.dc42" % V, "wb").write(hdr + img)
shutil.copy("build/Solstice.bin", "dist/Solstice %s.bin" % V)
crops = []
for f, name in [("screen_145s.png", "The Solargraph"), ("screen_75s.png", "Days to the Next Season"), ("screen_85s.png", "The Earth's Orbit"), ("screen_95s.png", "The Earth's Tilt"), ("screen_105s.png", "Where the Sun Is Now"), ("screen_115s.png", "Your Town"), ("screen_125s.png", "A Fun Fact"), ("screen_165s.png", "Solstice Traditions"), ("screen_175s.png", "Pictures from History 1"), ("screen_185s.png", "Pictures from History 2"), ("screen_195s.png", "Pictures from History 3"), ("screen_205s.png", "Pictures from History 4"), ("screen_215s.png", "Pictures from History 5"), ("screen_225s.png", "Pictures from History 6"), ("screen_155s.png", "World Map")]:
    im = Image.open("test/out/" + f).convert("RGB").crop((276, 26, 878, 426)); im.save("dist/%s.png" % name); crops.append((im, name))
w, h = crops[0][0].size
rows = (len(crops) + 1) // 2; sheet = Image.new("RGB", (w * 2 + 30, (h + 24) * rows + 10), (128, 128, 128)); d = ImageDraw.Draw(sheet)
for i, (im, name) in enumerate(crops):
    x, y = 10 + (i % 2) * (w + 10), 10 + (i // 2) * (h + 24); sheet.paste(im, (x, y)); d.text((x + 4, y + h + 6), name, fill=(0, 0, 0))
sheet.save("dist/Solstice screens.png")
EOF
O="dist/extfs/Solstice $V (test)"; rm -rf dist/extfs; $PY $M/hfs_extract.py build/Solstice.dsk "" "$O" >/dev/null
$PY -c "
import sys; p=sys.argv[1]; b=bytearray(open(p,'rb').read()); b[8]=0x20; b[9]=0; open(p,'wb').write(b)" "$O/.finf/Solstice"   # hasBundle
printf "Solstice $V (test), $(date '+%-d %B %Y')\r\rHow long until the next solstice or equinox, and where the Sun and the Earth are now.\rFor System 6 and 7, Mac Plus and up. No network needed.\r\rDouble-click Solstice. It opens on Tim's solargraph; any key goes on.\rSeasons menu (the same as LisaSolstice on the Lisa): Days to the Next Season (D),\rThe Earth's Orbit (O), The Earth's Tilt (E), Where the Sun Is Now (W), Your Town (Y),\rA Fun Fact (F), Solstice Traditions (L),\rPictures from History (P), The Solargraph (G), and World Map (M). The letters work alone or with Command.\rSet Your Town and Set Your Time Zone are in the Seasons menu (Cincinnati, UTC-5 by default).\r\rThe .dc42 is the same application on an 800K Disk Copy 4.2 disk image.\rWorld map: Natural Earth (public domain).\rPictures from History: photos from Wikimedia Commons, the Newgrange chamber by Dentp (CC BY-SA 4.0)\rand Stonehenge by Jeffrey Pfau (CC BY-SA 3.0); their copies here are under the same licences.\rNot supported; Tim O'Connor's lab.\r" > "$O/READ ME - Solstice.txt"
printf 'TEXTttxt' > "$O/.finf/READ ME - Solstice.txt"; head -c 24 /dev/zero >> "$O/.finf/READ ME - Solstice.txt"
rm -f dist/Solstice\ 0.1d1.*; ls -la dist "$O"
