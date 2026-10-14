"""Build resources/ultra_guard.ico from the game's rendered Ultraman portrait.

Usage:
  bin/ultra_guard.exe --capture build/ultra-guard-shots   (exports portraits/ultraman.png)
  python examples/ultra_guard/tools/make_icon.py build/ultra-guard-shots/portraits/ultraman.png
Requires Pillow.
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

root = Path(__file__).resolve().parents[1]
portrait = Image.open(sys.argv[1]).convert("RGBA")
size = 256

background = Image.new("RGBA", (size, size), (0, 0, 0, 0))
glow = Image.new("L", (size, size), 0)
ImageDraw.Draw(glow).ellipse((8, 8, size - 8, size - 8), fill=255)
disc = Image.new("RGBA", (size, size))
for y in range(size):
    t = y / size
    color = (int(30 + 40 * t), int(40 + 20 * t), int(110 - 40 * t), 255)
    ImageDraw.Draw(disc).line([(0, y), (size, y)], fill=color)
background.paste(disc, (0, 0), glow)
ring = Image.new("RGBA", (size, size), (0, 0, 0, 0))
ImageDraw.Draw(ring).ellipse((8, 8, size - 8, size - 8), outline=(255, 204, 84, 255), width=10)
background.alpha_composite(ring)

head = portrait.resize((236, 236), Image.LANCZOS)
mask = Image.new("L", (size, size), 0)
ImageDraw.Draw(mask).ellipse((14, 14, size - 14, size - 14), fill=255)
mask = mask.filter(ImageFilter.GaussianBlur(1))
layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
layer.alpha_composite(head, (10, 4))
clipped = Image.new("RGBA", (size, size), (0, 0, 0, 0))
clipped.paste(layer, (0, 0), Image.composite(layer.split()[3], Image.new("L", (size, size), 0), mask))
background.alpha_composite(clipped)

out = root / "resources"
out.mkdir(exist_ok=True)
background.save(out / "windows" / "ultra_guard.ico", sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
print("wrote", out / "windows" / "ultra_guard.ico")

# Android launcher icons: legacy PNGs per density, plus adaptive (Android 8+) and
# themed monochrome (Android 13+) layers. The adaptive foreground keeps the head inside
# the 66% safe zone of its 108dp canvas.
res = root / "android" / "res"
for folder, px in {"mdpi": 48, "hdpi": 72, "xhdpi": 96, "xxhdpi": 144, "xxxhdpi": 192}.items():
    (res / f"mipmap-{folder}").mkdir(parents=True, exist_ok=True)
    background.resize((px, px), Image.LANCZOS).save(res / f"mipmap-{folder}" / "ic_launcher.png")
canvas = 432
foreground = Image.new("RGBA", (canvas, canvas), (0, 0, 0, 0))
inner = int(canvas * 0.66)
foreground.alpha_composite(portrait.resize((inner, inner), Image.LANCZOS), ((canvas - inner) // 2, (canvas - inner) // 2))
foreground.save(res / "mipmap-xxxhdpi" / "ic_launcher_foreground.png")
mono = Image.new("RGBA", (canvas, canvas), (255, 255, 255, 0))
mono.putalpha(foreground.split()[3])
mono.save(res / "mipmap-xxxhdpi" / "ic_launcher_monochrome.png")
layers = {"v26": "", "v33": '\n    <monochrome android:drawable="@mipmap/ic_launcher_monochrome" />'}
for version, extra in layers.items():
    (res / f"mipmap-anydpi-{version}").mkdir(parents=True, exist_ok=True)
    (res / f"mipmap-anydpi-{version}" / "ic_launcher.xml").write_text(
        '<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n'
        '    <background android:drawable="@color/icon_background" />\n'
        f'    <foreground android:drawable="@mipmap/ic_launcher_foreground" />{extra}\n'
        '</adaptive-icon>\n', encoding="utf-8")
(res / "values").mkdir(parents=True, exist_ok=True)
(res / "values" / "colors.xml").write_text('<resources><color name="icon_background">#1d2a5a</color></resources>\n',
                                           encoding="utf-8")
print("wrote", res)
