#!/usr/bin/env python3
"""Subset a TrueType/OpenType font to the characters a game actually uses.

Large CJK fonts (10+ MB) shrink to a few hundred KB. Text is collected from source
and asset files, so re-run after editing user-facing strings.

Example:
  python scripts/subset-font.py NotoSansSC-VF.ttf assets/fonts/GameSC-Bold.ttf \
      --text src --text assets/locales --weight 700 --family "Game SC" --charset assets/fonts/charset.txt

At runtime, load the same characters with ResourceManager::CodepointsFromText (pass the
charset file's contents) and LoadFontResource(name, path, size, codepoints).
Requires fonttools (pip install fonttools). Keep the upstream font license with the output.
"""
import argparse
from pathlib import Path

from fontTools import subset
from fontTools.ttLib import TTFont

TEXT_SUFFIXES = {".h", ".hpp", ".c", ".cc", ".cpp", ".txt", ".tsv", ".csv", ".json", ".md", ".level", ".ini", ".lua"}


def collect(paths):
    chars = set()
    for name in paths:
        path = Path(name)
        files = [p for p in path.rglob("*") if p.is_file() and p.suffix.lower() in TEXT_SUFFIXES] if path.is_dir() else [path]
        for file in files:
            chars.update(file.read_text(encoding="utf-8", errors="ignore"))
    return {c for c in chars if ord(c) >= 160}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source", type=Path, help="Font to subset (static or variable)")
    parser.add_argument("output", type=Path, help="Destination .ttf")
    parser.add_argument("--text", action="append", default=[], help="File or directory to scan (repeatable)")
    parser.add_argument("--chars", default="", help="Extra characters to always include")
    parser.add_argument("--weight", type=float, help="Instance a variable font at this wght value")
    parser.add_argument("--family", help="Rename the output family (recommended for modified fonts)")
    parser.add_argument("--charset", type=Path, help="Also write the non-ASCII characters to this UTF-8 file")
    args = parser.parse_args()

    chars = collect(args.text) | {c for c in args.chars if ord(c) >= 32}
    points = set(range(32, 127)) | {ord(c) for c in chars}
    font = TTFont(args.source, recalcTimestamp=False)
    missing = sorted(points - font.getBestCmap().keys())
    if missing:
        raise SystemExit("Missing glyphs: " + " ".join(f"U+{p:04X}" for p in missing))
    if args.weight is not None:
        if "fvar" not in font:
            raise SystemExit("--weight requires a variable font")
        from fontTools.varLib.instancer import instantiateVariableFont

        instantiateVariableFont(font, {"wght": args.weight}, inplace=True)
    options = subset.Options()
    options.name_IDs = [0, 1, 2, 3, 4, 5, 6, 13, 14]
    options.name_languages = [0x409]
    worker = subset.Subsetter(options=options)
    worker.populate(unicodes=points)
    worker.subset(font)
    if args.family:
        style = font["name"].getDebugName(2) or "Regular"
        for name_id, value in {1: args.family, 3: f"{args.family} {style}", 4: f"{args.family} {style}",
                               6: f"{args.family.replace(' ', '')}-{style}"}.items():
            font["name"].setName(value, name_id, 3, 1, 0x409)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    font.save(args.output)
    if args.charset:
        args.charset.write_text("".join(sorted(c for c in chars if ord(c) >= 127)), encoding="utf-8")
    print(f"{args.output}: {len(points)} code points ({len(chars)} beyond ASCII)")


if __name__ == "__main__":
    main()
