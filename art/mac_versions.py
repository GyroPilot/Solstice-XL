"""The solargraph for Solstice for Mac: the same method Tim chose for the Lisa (4: blue channel stretched 2-98
percent, Floyd-Steinberg), but for square Mac pixels.  512 x 325 fits a Plus screen under the menu bar."""
from PIL import Image
import numpy as np
src = Image.open('solargraph_tim_original.jpg').convert('RGB')
for w in (512, 640):
    h = round(w * src.height / src.width)
    b = np.asarray(src.resize((w, h), Image.LANCZOS).getchannel(2), dtype=float)
    p1, p2 = np.percentile(b, 2), np.percentile(b, 98)
    b = np.clip((b - p1) / (p2 - p1) * 255, 0, 255).astype('uint8')
    im = Image.fromarray(b, 'L').convert('1', dither=Image.FLOYDSTEINBERG)
    im.save('solargraph_mac_%dx%d.png' % (w, h)); print(w, h)
