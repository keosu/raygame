"""Rebuild the original four-frame demonstration sheets; requires Pillow."""
from pathlib import Path
from PIL import Image, ImageDraw
import math
import struct
import wave

root = Path(__file__).resolve().parents[1] / "assets"
root.mkdir(parents=True, exist_ok=True)
for alternate in (False, True):
    image = Image.new("RGBA", (192, 48), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    colors = ("#ee79a8", "#ffc065", "#56d7da", "#9c93e7") if alternate else ("#56d7da", "#ffc065", "#9c93e7", "#ee79a8")
    for frame, color in enumerate(colors):
        x = frame * 48
        draw.rounded_rectangle((x + 2, 2, x + 45, 45), radius=8, fill="#234756", outline=color, width=2)
        draw.polygon([(x + 10, 12), (x + 36, 24), (x + 10, 36)], fill=color)
        draw.ellipse((x + 16 + frame * 3, 19, x + 23 + frame * 3, 26), fill="#111e2a")
    image.save(root / ("sheet-alt.png" if alternate else "sheet.png"))

with wave.open(str(root / "chime.wav"), "wb") as sound:
    sound.setparams((1, 2, 22050, 0, "NONE", "not compressed"))
    sound.writeframes(b"".join(struct.pack("<h", int(12000 * math.sin(2 * math.pi * 660 * i / 22050) *
                     math.sin(math.pi * i / 6600) ** 2)) for i in range(6600)))
