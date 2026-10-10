#!/usr/bin/env python3
"""Format the adopted C++ scope. Pass paths to format additional files gradually."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_PATHS = (
    "examples/forge_runner/src",
    "examples/forge_runner/tests",
    "examples/bouncing_balls/main.cpp",
    "examples/bullet_hell/main.cpp",
    "include/CanvasLayout.h",
    "include/TouchTracker.h",
    "tests/touch_layout.h",
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--clang-format", default="clang-format")
    parser.add_argument("paths", nargs="*", help="Paths relative to the repository root")
    args = parser.parse_args()
    formatter = shutil.which(args.clang_format)
    if not formatter:
        parser.error("clang-format 19 is required; install it or pass --clang-format PATH")
    files = set()
    for name in args.paths or DEFAULT_PATHS:
        path = ROOT / name
        if not path.exists():
            parser.error(f"Path does not exist: {name}")
        candidates = path.rglob("*") if path.is_dir() else (path,)
        files.update(file for file in candidates if file.suffix in (".h", ".cpp") and file.is_file())
    if not files:
        parser.error("No C++ files selected")
    failed = False
    for file in sorted(files):
        result = subprocess.run([formatter, "--style=file", str(file)], check=True, capture_output=True)
        formatted = result.stdout.replace(b"\r\n", b"\n")
        if formatted != file.read_bytes():
            if args.check:
                print(f"Needs formatting: {file.relative_to(ROOT)}")
                failed = True
            else:
                file.write_bytes(formatted)
    print(f"{'Checked' if args.check else 'Formatted'} {len(files)} C++ files")
    return int(failed)


if __name__ == "__main__":
    sys.exit(main())
