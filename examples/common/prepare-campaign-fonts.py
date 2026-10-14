"""Build campaign catalogs and original-named Noto Sans SC subsets (SIL OFL).

python examples/common/prepare-campaign-fonts.py <NotoSansSC-variable.ttf>
Requires fonttools 4.65.0. Normal builds use the committed assets.
"""
import argparse
from pathlib import Path
from fontTools import subset
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("source", type=Path)
parser.add_argument("--output", type=Path, help="Isolated output root for reproducibility checks")
parser.add_argument("--game", choices=("marble_temple", "garden_guard", "goo_journey"))
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
shared = (root / "common/locales/zh-CN.tsv").read_text(encoding="utf-8")
for game in ([args.game] if args.game else ("marble_temple", "garden_guard", "goo_journey")):
    owned = (root / game / "artwork/zh-CN.tsv").read_text(encoding="utf-8")
    catalog = shared + owned
    output = args.output / game if args.output else root / game / "assets"
    (output / "locales").mkdir(parents=True, exist_ok=True)
    (output / "locales/zh-CN.tsv").write_text(catalog, encoding="utf-8", newline="\n")
    points = set(range(32, 127))
    for line in catalog.splitlines():
        if line and not line.startswith("#"):
            points.update(map(ord, line.replace("\t", "")))
    fonts = output / "fonts"
    fonts.mkdir(exist_ok=True)
    for weight, style in ((500, "Medium"), (600, "SemiBold")):
        font = TTFont(args.source, recalcTimestamp=False)
        if points - font.getBestCmap().keys():
            raise ValueError("Source font does not cover all campaign text")
        font = instantiateVariableFont(font, {"wght": weight}, inplace=True)
        options = subset.Options()
        options.name_IDs = [0, 1, 2, 3, 4, 5, 6, 13, 14]
        options.name_languages = [0x409]
        worker = subset.Subsetter(options=options)
        worker.populate(unicodes=points)
        worker.subset(font)
        names = {1: "Campaign SC", 2: style, 3: f"Campaign SC {style} 1.0",
                 4: f"Campaign SC {style}", 6: f"CampaignSC-{style}"}
        for name_id, value in names.items():
            font["name"].setName(value, name_id, 3, 1, 0x409)
        font.recalcTimestamp = False
        font.save(fonts / f"CampaignSC-{style}.ttf")
    (fonts / "NotoSansSC-OFL.txt").write_bytes((root / "forge_runner/assets/fonts/NotoSansSC-OFL.txt").read_bytes())
    (fonts / "CampaignSC-README.md").write_text(
        "# Campaign SC fonts\n\n"
        "Noto Sans SC 500/600 subsets, renamed Campaign SC Medium/SemiBold.\n"
        "Copyright and SIL OFL 1.1 notices are retained in NotoSansSC-OFL.txt.\n"
        "Source: https://github.com/google/fonts/tree/main/ofl/notosanssc\n"
        "Source SHA-256: a3041811a78c361b1de50f953c805e0244951c21c5bd412f7232ef0d899af0da\n"
        "Regenerate with examples/common/prepare-campaign-fonts.py and fonttools 4.65.0\n"
        "after editing common/locales or the game's artwork/zh-CN.tsv.\n",
        encoding="utf-8", newline="\n")
    print(f"{game}: {len(points)} Unicode scalars")
