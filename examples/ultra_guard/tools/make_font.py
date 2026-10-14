"""Build the bundled Chinese UI fonts from Noto Sans SC (SIL Open Font License).

Usage: python tools/make_font.py [NotoSansSC-variable.ttf]

Runs raygame's scripts/subset-font.py for a Medium and a Bold instance with every
character used in this example's sources, and writes assets/fonts/charset.txt for the runtime glyph list.
Requires fonttools. Re-run after editing any user-facing text.
"""
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = sys.argv[1] if len(sys.argv) > 1 else r"C:\Windows\Fonts\NotoSansSC-VF.ttf"
script = root.parents[1] / "scripts" / "subset-font.py"
fonts = root / "assets" / "fonts"
for weight, style in ((500, "Medium"), (700, "Bold")):
    subprocess.run([sys.executable, str(script), source, str(fonts / f"UltraSC-{style}.ttf"), *sum((["--text", str(p)] for p in sorted(root.glob("*.[ch]*"))), []),
                    "--weight", str(weight), "--family", "UltraGuard SC", "--charset", str(fonts / "charset.txt")],
                   check=True)
