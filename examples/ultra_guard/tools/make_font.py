"""Build the bundled Chinese UI fonts from a Noto Sans SC variable font.

Usage: python tools/make_font.py [NotoSansSC-VF.ttf]

Runs raygame's scripts/subset-font.py for a Medium and a Bold instance with every
character used in this example's sources, assets/levels/ and assets/skins/, and writes
assets/fonts/charset.txt for the runtime glyph list. Requires fonttools. Re-run after
editing any user-facing text, including level and skin files.

The variable font keeps both weights in one file. When it is not installed the
Microsoft YaHei pair is used instead: static fonts cannot be instanced, so --weight
is skipped for them.
"""
import subprocess
import sys
from pathlib import Path

from fontTools.ttLib import TTFont

root = Path(__file__).resolve().parents[1]
script = root.parents[1] / "scripts" / "subset-font.py"
fonts = root / "assets" / "fonts"
texts = [p for p in sorted(root.glob("*.[ch]*"))]
texts += [root / folder for folder in ("assets/levels", "assets/skins") if (root / folder).exists()]

VARIABLE = Path(r"C:\Windows\Fonts\NotoSansSC-VF.ttf")
# (source, wght) pairs: one variable font twice, or a regular/bold pair.
PAIRS = [(VARIABLE, 500, "Medium"), (VARIABLE, 700, "Bold")]
FALLBACK = [(Path(r"C:\Windows\Fonts\msyh.ttc"), 500, "Medium"),
            (Path(r"C:\Windows\Fonts\msyhbd.ttc"), 700, "Bold")]
temporary = []  # extracted .ttc faces, removed before exit


def is_variable(path):
    try:
        return "fvar" in TTFont(path, fontNumber=0, lazy=True).keys()
    except Exception:
        return False


def extract(path):
    """A .ttc holds several faces; subset-font.py wants one plain font file."""
    if path.suffix.lower() != ".ttc":
        return path
    plain = fonts / f"_source-{path.stem}.ttf"
    TTFont(path, fontNumber=0).save(plain)
    temporary.append(plain)
    return plain


def run(source, weight, style):
    source = extract(source)
    command = [sys.executable, str(script), str(source), str(fonts / f"UltraSC-{style}.ttf"),
               *sum((["--text", str(p)] for p in texts), []),
               "--family", "UltraGuard SC", "--charset", str(fonts / "charset.txt")]
    if is_variable(source):
        command += ["--weight", str(weight)]
    subprocess.run(command, check=True)


def main():
    try:
        source = Path(sys.argv[1]) if len(sys.argv) > 1 else VARIABLE
        if source.exists():
            for path, weight, style in PAIRS:
                run(source, weight, style)
            return
        if not all(path.exists() for path, _, _ in FALLBACK):
            raise SystemExit("No CJK source font found. Pass a Noto Sans SC variable font: "
                             "python tools/make_font.py <path>")
        print("NotoSansSC-VF.ttf not found; using the Microsoft YaHei pair.")
        for path, _, style in FALLBACK:
            run(path, 0, style)
    finally:
        for path in temporary:
            path.unlink(missing_ok=True)


if __name__ == "__main__":
    main()
