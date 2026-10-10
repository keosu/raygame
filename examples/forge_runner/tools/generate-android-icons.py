"""Export FORGE Android legacy, adaptive and themed icons. Requires Pillow.

Uses the same geometry as generate-assets.py; normal builds use committed PNGs.
"""
from pathlib import Path
from PIL import Image, ImageDraw

root = Path(__file__).resolve().parents[1]
res = root / "android/res"
legacy = Image.open(root / "assets/icon.png").convert("RGBA")
for density, size in (("mdpi", 48), ("hdpi", 72), ("xhdpi", 96), ("xxhdpi", 144), ("xxxhdpi", 192)):
    directory = res / f"mipmap-{density}"
    directory.mkdir(parents=True, exist_ok=True)
    legacy.resize((size, size), Image.Resampling.LANCZOS).save(directory / "ic_launcher.png")

# A 512px design fits into the adaptive icon's central 66/108 safe region.
foreground = Image.new("RGBA", (864, 864))
emblem = Image.new("RGBA", (512, 512))
draw = ImageDraw.Draw(emblem)
draw.polygon([(130, 170), (255, 97), (385, 170), (385, 324), (255, 407), (130, 324)], fill=(222, 184, 115))
draw.polygon([(164, 186), (258, 139), (350, 186), (350, 298), (256, 359), (164, 298)], fill=(16, 35, 41))
draw.polygon([(228, 198), (294, 174), (273, 239), (326, 232), (229, 322), (246, 262), (193, 272)], fill=(150, 245, 199))
foreground.alpha_composite(emblem, (176, 176))
foreground.resize((432, 432), Image.Resampling.LANCZOS).save(res / "mipmap-xxxhdpi/ic_launcher_foreground.png")
mono = Image.new("RGBA", (864, 864))
draw = ImageDraw.Draw(mono)
outer = [(x + 176, y + 176) for x, y in [(130, 170), (255, 97), (385, 170), (385, 324), (255, 407), (130, 324)]]
inner = [(x + 176, y + 176) for x, y in [(164, 186), (258, 139), (350, 186), (350, 298), (256, 359), (164, 298)]]
bolt = [(x + 176, y + 176) for x, y in [(228, 198), (294, 174), (273, 239), (326, 232), (229, 322), (246, 262), (193, 272)]]
draw.polygon(outer, fill="white")
draw.polygon(inner, fill=(0, 0, 0, 0))
draw.polygon(bolt, fill="white")
mono.resize((432, 432), Image.Resampling.LANCZOS).save(res / "mipmap-xxxhdpi/ic_launcher_monochrome.png")
for api in (26, 33):
    directory = res / f"mipmap-anydpi-v{api}"
    directory.mkdir(parents=True, exist_ok=True)
    themed = '    <monochrome android:drawable="@mipmap/ic_launcher_monochrome" />\n' if api == 33 else ""
    (directory / "ic_launcher.xml").write_text(
        '<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n'
        '    <background android:drawable="@color/icon_background" />\n'
        '    <foreground android:drawable="@mipmap/ic_launcher_foreground" />\n'
        + themed + '</adaptive-icon>\n', encoding="utf-8", newline="\n")
(res / "values").mkdir(exist_ok=True)
(res / "values/colors.xml").write_text(
    '<resources><color name="icon_background">#09161c</color></resources>\n', encoding="utf-8", newline="\n")
print("FORGE: generated Android legacy/adaptive/themed launcher icons")
