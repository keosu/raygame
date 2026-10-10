"""Export the original SVG artwork to desktop and Android icons.

Only needed when editing artwork: pip install Pillow==12.3.0 resvg-py==0.2.6
Normal xmake/CI builds use the committed outputs and do not require Python.
"""
from io import BytesIO
from pathlib import Path
import xml.etree.ElementTree as ET

from PIL import Image, ImageDraw
from resvg_py import svg_to_bytes

ROOT = Path(__file__).resolve().parents[1]
EXAMPLES = ROOT / "examples"
GAMES = ("space_shooter", "bouncing_balls", "neon_breakout", "bullet_hell")
BACKGROUNDS = ("#0b1a30", "#0b252d", "#1b1a32", "#221432")
ET.register_namespace("", "http://www.w3.org/2000/svg")


def save(image, path, size):
    path.parent.mkdir(parents=True, exist_ok=True)
    image.resize((size, size), Image.Resampling.LANCZOS).save(path)


previews = Image.new("RGB", (1024, 296), "#0d1220")
labels = ImageDraw.Draw(previews)
for index, (name, background) in enumerate(zip(GAMES, BACKGROUNDS)):
    game = EXAMPLES / name
    source = game / "artwork/icon.svg"
    svg = source.read_text(encoding="utf-8")
    image = Image.open(BytesIO(svg_to_bytes(svg_string=svg, width=1024, height=1024))).convert("RGBA")
    save(image, game / "assets/icon.png", 256)
    windows = game / "resources/windows"
    windows.mkdir(parents=True, exist_ok=True)
    image.save(windows / f"{name}.ico", sizes=[(s, s) for s in (16, 20, 24, 32, 48, 64, 128, 256)])
    (windows / f"{name}.rc").write_text(f'1 ICON "{name}.ico"\n', encoding="utf-8", newline="\n")

    res = game / "android/res"
    for density, size in (("mdpi", 48), ("hdpi", 72), ("xhdpi", 96), ("xxhdpi", 144), ("xxxhdpi", 192)):
        save(image, res / f"mipmap-{density}/ic_launcher.png", size)
    tree = ET.fromstring(svg)
    for element in list(tree):
        if element.get("id") == "background":
            tree.remove(element)
    foreground = Image.open(BytesIO(svg_to_bytes(svg_string=ET.tostring(tree, encoding="unicode"), width=864, height=864))).convert("RGBA")
    save(foreground, res / "mipmap-xxxhdpi/ic_launcher_foreground.png", 432)
    mono = Image.new("RGBA", foreground.size, "white")
    mono.putalpha(foreground.getchannel("A"))
    save(mono, res / "mipmap-xxxhdpi/ic_launcher_monochrome.png", 432)
    for api in (26, 33):
        path = res / f"mipmap-anydpi-v{api}/ic_launcher.xml"
        path.parent.mkdir(parents=True, exist_ok=True)
        themed = '    <monochrome android:drawable="@mipmap/ic_launcher_monochrome" />\n' if api == 33 else ""
        path.write_text('<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n'
                        '    <background android:drawable="@color/icon_background" />\n'
                        '    <foreground android:drawable="@mipmap/ic_launcher_foreground" />\n'
                        + themed + '</adaptive-icon>\n', encoding="utf-8", newline="\n")
    colors = res / "values/colors.xml"
    colors.parent.mkdir(parents=True, exist_ok=True)
    colors.write_text(f'<resources><color name="icon_background">{background}</color></resources>\n', encoding="utf-8", newline="\n")
    tile = image.resize((224, 224), Image.Resampling.LANCZOS)
    previews.paste(tile, (index * 256 + 16, 16), tile)
    labels.text((index * 256 + 22, 254), name.replace("_", " ").upper(), fill="#d7e5ef")

preview = ROOT / "build/icons-preview.png"
preview.parent.mkdir(parents=True, exist_ok=True)
previews.save(preview)
print("Exported four SVG designs to ICO, window PNG and Android legacy/adaptive/themed icons.")
