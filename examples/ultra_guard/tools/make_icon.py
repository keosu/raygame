"""Builds the app icons from a real hero avatar (assets/avatars/<key>.png).

Usage:
  python examples/ultra_guard/tools/make_icon.py [avatar.png]
Defaults to Tiga, first in the 2026 popularity poll. Writes:
  assets/icon.png                    window icon (256 px)
  resources/windows/ultra_guard.ico  Windows executable icon
  android/res/...                    legacy, adaptive and monochrome launcher icons
Requires Pillow.
"""
import math
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

root = Path(__file__).resolve().parents[1]
source = Path(sys.argv[1]) if len(sys.argv) > 1 else root / "assets" / "avatars" / "tiga.png"
avatar = Image.open(source).convert("RGBA")
S = 1024  # master size; every output is a downscale


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))


def rounded_mask(size, radius):
    mask = Image.new("L", (size, size), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, size - 1, size - 1), radius=radius, fill=255)
    return mask


def backdrop(size):
    """Night-sky gradient with a warm burst of light behind the hero."""
    image = Image.new("RGBA", (size, size))
    draw = ImageDraw.Draw(image)
    for y in range(size):
        draw.line([(0, y), (size, y)], fill=lerp((22, 30, 92, 255), (96, 22, 70, 255), y / size))
    centre = (size / 2, size * 0.44)
    rays = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    rd = ImageDraw.Draw(rays)
    for i in range(18):
        a = i * math.pi / 9
        spread = 0.07
        far = size * 1.1
        pts = [centre,
               (centre[0] + math.cos(a - spread) * far, centre[1] + math.sin(a - spread) * far),
               (centre[0] + math.cos(a + spread) * far, centre[1] + math.sin(a + spread) * far)]
        rd.polygon(pts, fill=(255, 220, 150, 34))
    image.alpha_composite(rays)
    glow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    gd = ImageDraw.Draw(glow)
    for r, alpha in ((0.46, 60), (0.34, 90), (0.22, 120)):
        gd.ellipse((centre[0] - size * r, centre[1] - size * r, centre[0] + size * r, centre[1] + size * r),
                   fill=(255, 196, 120, alpha))
    image.alpha_composite(glow.filter(ImageFilter.GaussianBlur(size * 0.06)))
    ring = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    r = size * 0.36
    ImageDraw.Draw(ring).ellipse((centre[0] - r, centre[1] - r, centre[0] + r, centre[1] + r),
                                 outline=(255, 206, 96, 230), width=int(size * 0.018))
    image.alpha_composite(ring)
    return image


def hero_layer(size, scale):
    """The avatar, its feet faded so the bust melts into the backdrop."""
    side = int(size * scale)
    head = avatar.resize((side, side), Image.LANCZOS)
    fade = Image.new("L", (side, side), 255)
    fd = ImageDraw.Draw(fade)
    for y in range(int(side * 0.78), side):
        fd.line([(0, y), (side, y)], fill=int(255 * (1 - (y - side * 0.78) / (side * 0.22))))
    alpha = Image.composite(head.getchannel("A"), Image.new("L", (side, side), 0), fade)
    head.putalpha(alpha)
    layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    layer.alpha_composite(head, ((size - side) // 2, int(size * 0.06)))
    shadow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    shadow.putalpha(layer.getchannel("A").point(lambda v: v * 0.55))
    shadow = shadow.filter(ImageFilter.GaussianBlur(size * 0.02))
    out = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    out.alpha_composite(shadow, (0, int(size * 0.012)))
    out.alpha_composite(layer)
    return out


icon = backdrop(S)
icon.alpha_composite(hero_layer(S, 0.98))
gloss = Image.new("RGBA", (S, S), (0, 0, 0, 0))
ImageDraw.Draw(gloss).ellipse((-S * 0.3, -S * 0.75, S * 1.3, S * 0.35), fill=(255, 255, 255, 26))
icon.alpha_composite(gloss)
border = Image.new("RGBA", (S, S), (0, 0, 0, 0))
ImageDraw.Draw(border).rounded_rectangle((6, 6, S - 7, S - 7), radius=int(S * 0.2), outline=(255, 214, 120, 200),
                                         width=int(S * 0.012))
icon.alpha_composite(border)
shaped = Image.new("RGBA", (S, S), (0, 0, 0, 0))
shaped.paste(icon, (0, 0), rounded_mask(S, int(S * 0.2)))

(root / "assets").mkdir(exist_ok=True)
shaped.resize((256, 256), Image.LANCZOS).save(root / "assets" / "icon.png")
print("wrote", root / "assets" / "icon.png")
windows = root / "resources" / "windows"
windows.mkdir(parents=True, exist_ok=True)
shaped.resize((256, 256), Image.LANCZOS).save(windows / "ultra_guard.ico",
                                              sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
print("wrote", windows / "ultra_guard.ico")

# Android: legacy PNGs per density, adaptive layers (Android 8+) and a themed monochrome
# layer (Android 13+). The adaptive foreground keeps the bust inside the 66% safe zone.
res = root / "android" / "res"
for folder, px in {"mdpi": 48, "hdpi": 72, "xhdpi": 96, "xxhdpi": 144, "xxxhdpi": 192}.items():
    (res / f"mipmap-{folder}").mkdir(parents=True, exist_ok=True)
    shaped.resize((px, px), Image.LANCZOS).save(res / f"mipmap-{folder}" / "ic_launcher.png")
canvas = 432
foreground = Image.new("RGBA", (canvas, canvas), (0, 0, 0, 0))
inner = int(canvas * 0.76)  # the head itself stays inside the 66% safe circle
bust = hero_layer(inner, 1.0)
foreground.alpha_composite(bust, ((canvas - inner) // 2, (canvas - inner) // 2))
foreground.save(res / "mipmap-xxxhdpi" / "ic_launcher_foreground.png")
mono = Image.new("RGBA", (canvas, canvas), (255, 255, 255, 0))
mono.putalpha(foreground.getchannel("A"))
mono.save(res / "mipmap-xxxhdpi" / "ic_launcher_monochrome.png")
layers = {"v26": "", "v33": '\n    <monochrome android:drawable="@mipmap/ic_launcher_monochrome" />'}
for version, extra in layers.items():
    (res / f"mipmap-anydpi-{version}").mkdir(parents=True, exist_ok=True)
    (res / f"mipmap-anydpi-{version}" / "ic_launcher.xml").write_text(
        '<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n'
        '    <background android:drawable="@color/icon_background" />\n'
        f'    <foreground android:drawable="@mipmap/ic_launcher_foreground" />{extra}\n'
        '</adaptive-icon>\n', encoding="utf-8", newline="\n")
(res / "values").mkdir(parents=True, exist_ok=True)
(res / "values" / "colors.xml").write_text('<resources><color name="icon_background">#2a1c5c</color></resources>\n',
                                           encoding="utf-8", newline="\n")
print("wrote", res)
