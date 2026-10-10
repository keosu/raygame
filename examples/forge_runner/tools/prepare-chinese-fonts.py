"""Create redistributable FORGE SC font subsets from Noto Sans SC (SIL OFL).

Usage: python prepare-chinese-fonts.py <NotoSansSC-variable.ttf> [--output <dir>]
Requires fonttools. Normal builds use committed subsets and need no Python.
See assets/fonts/README.md for the pinned upstream URL and SHA-256.
"""
import argparse
from pathlib import Path
from fontTools import subset
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument("source", type=Path)
parser.add_argument("--output", type=Path, default=root / "assets/fonts")
args = parser.parse_args()
rows = (root / "assets/locales/zh-CN.tsv").read_text(encoding="utf-8").splitlines()
characters = "".join(row.split("\t")[1] for row in rows if row and not row.startswith("#"))
points = set(range(32, 127)) | set(map(ord, characters))
args.output.mkdir(parents=True, exist_ok=True)
for weight, style in ((500, "Medium"), (600, "SemiBold")):
    font = TTFont(args.source, recalcTimestamp=False)
    missing = points - font.getBestCmap().keys()
    if missing:
        raise ValueError(f"Missing upstream glyphs: {sorted(missing)}")
    font = instantiateVariableFont(font, {"wght": weight}, inplace=True)
    options = subset.Options()
    options.name_IDs = [0, 1, 2, 3, 4, 5, 6, 13, 14]
    options.name_languages = [0x409]
    worker = subset.Subsetter(options=options)
    worker.populate(unicodes=points)
    worker.subset(font)
    # Rename the modified font; preserve upstream copyright and license records.
    names = {1: "FORGE SC", 2: style, 3: f"FORGE SC {style} 1.0", 4: f"FORGE SC {style}", 6: f"FORGESC-{style}"}
    for name_id, value in names.items():
        font['name'].setName(value, name_id, 3, 1, 0x409)
    font.recalcTimestamp = False
    font.save(args.output / f"ForgeSC-{style}.ttf")
    print(f"FORGE SC {style}: {len(points)} Unicode scalars")
