"""Build the sample "neon" skin from the game's own art.

Usage: python examples/ultra_guard/tools/make_sample_skin.py [template_dir]

Without a template directory, runs `bin/ultra_guard.exe --export-skin build/ultra-guard-skin`
in the raygame root first (build the example before). Writes assets/skins/neon/: a night-graded ground per map
theme, hue-shifted kaiju strips and a neon palette; avatars are left to the built-in art.
Requires numpy and Pillow. Re-run tools/make_font.py afterwards for the skin name.
"""
import shutil
import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image

root = Path(__file__).resolve().parents[1]
repo = root.parents[1]
template = Path(sys.argv[1]) if len(sys.argv) > 1 else repo / "build" / "ultra-guard-skin"
if len(sys.argv) <= 1:
    shutil.rmtree(template, ignore_errors=True)
    subprocess.run([str(repo / "bin" / "ultra_guard.exe"), "--export-skin", str(template)], check=True)

out = root / "assets" / "skins" / "neon"
shutil.rmtree(out, ignore_errors=True)
(out / "maps").mkdir(parents=True)
(out / "kaiju").mkdir()

# Night grade: darker, bluer ground with scattered warm window lights.
themes = {"tokyo": "city", "fuji": "mountain", "nebula": "nebula"}
rng = np.random.default_rng(78)
for level, theme in themes.items():
    image = Image.open(template / "maps" / f"{level}.png").convert("RGB").resize((1080, 648), Image.LANCZOS)
    a = np.asarray(image).astype(np.float32) / 255
    luma = a @ np.array([0.3, 0.59, 0.11], np.float32)
    night = np.stack([luma * 0.35 + a[..., 0] * 0.15, luma * 0.42 + a[..., 1] * 0.18, luma * 0.55 + a[..., 2] * 0.35], -1)
    night += np.array([0.03, 0.02, 0.09], np.float32)
    for _ in range(260):
        x, y = rng.integers(0, 1080), rng.integers(0, 648)
        color = np.array([1.0, 0.45, 0.85] if rng.random() < 0.5 else [0.35, 0.9, 1.0], np.float32)
        y0, y1, x0, x1 = max(0, y - 6), min(648, y + 7), max(0, x - 6), min(1080, x + 7)
        yy, xx = np.ogrid[y0:y1, x0:x1]
        d = np.exp(-((yy - y) ** 2 + (xx - x) ** 2) / 6.0)[..., None]
        night[y0:y1, x0:x1] += d * color * 0.35
    Image.fromarray((np.clip(night, 0, 1) * 255).astype(np.uint8)).save(out / "maps" / f"{theme}.png", optimize=True)

# Kaiju: rotate the hue and lift saturation so every monster gets a neon counterpart.
for strip in sorted((template / "kaiju").glob("*.png")):
    rgba = Image.open(strip).convert("RGBA")
    hsv = np.asarray(rgba.convert("RGB").convert("HSV")).astype(np.int16)
    hsv[..., 0] = (hsv[..., 0] + 150) % 256
    hsv[..., 1] = np.clip(hsv[..., 1] * 1.25 + 20, 0, 255)
    rgb = Image.fromarray(hsv.astype(np.uint8), "HSV").convert("RGB")
    result = Image.merge("RGBA", (*rgb.split(), rgba.split()[3]))
    result.save(out / "kaiju" / strip.name, optimize=True)

lines = [
    "# 示例皮肤：由 tools/make_sample_skin.py 根据内置画面生成。格式说明见 --export-skin 导出的 skin.txt。",
    "name 霓虹夜",
    "",
    "color gold      120 236 255 255",
    "color cyan      255 120 220 255",
    "color timer     255 120 220 255",
    "color muted     160 150 210 255",
    "color line      150  90 200 150",
    "color green     120 255 190 255",
    "",
]
for strip in sorted((out / "kaiju").glob("*.png")):
    lines.append(f"kaiju {strip.stem:<12} frames 12 fps 12 size 80 feet 0.94")
lines += ["", "map_paths on", "map_decor on", ""]
(out / "skin.txt").write_text("\n".join(lines), encoding="utf-8", newline="\n")

index = root / "assets" / "skins" / "skins.txt"
names = index.read_text(encoding="utf-8").splitlines() if index.exists() else [
    "# 可选皮肤（一行一个 assets/skins 下的目录名，按标题页切换顺序）。每个目录必须有 skin.txt。"]
if "neon" not in names:
    names.append("neon")
index.write_text("\n".join(names) + "\n", encoding="utf-8", newline="\n")
print(f"wrote {out}")
