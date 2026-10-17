#!/usr/bin/env python3
"""Builds square hero avatars from full-body cutouts.

The game replaces a drawn portrait with ``assets/avatars/<key>.png``: a square,
transparent PNG whose head sits slightly above the middle (the drawn bust has its
head centred near 0.42 of the height). Full-body photos therefore get cropped to a
bust window that is centred on the head.

Usage:
    python make_avatars.py [--source DIR] [--out DIR] [--size 512]

Sources are matched by key in ``SOURCES``; missing files are reported, not fatal.
"""

import argparse
import os
import sys

from PIL import Image

# hero key -> file name inside the cutout directory
SOURCES = {
    "z": "z.png",
    "orb": "orb.png",
    "tiga": "tiga.png",
    "geed": "geed.png",
    "arc": "arc.png",
    "zero": "zero.png",
    "trigger": "trigger.png",
    "decker": "decker.png",
    "blazar": "blazar.png",
}

# Hand-measured (centre x, top y, head height) in source pixels for poses that fool
# head_window, e.g. Trigger's raised arm reaching above the head.
WINDOWS = {
    "trigger": (528, 20, 456),
}

# Head share of the final square, and where the head starts vertically.
HEAD_SHARE = 0.46
HEAD_TOP = 0.12
# A row counts as "shoulders" once its opaque width passes this share of the widest row.
SHOULDER_RATIO = 0.60


def opaque_rows(image):
    """Opaque pixel count and x extremes for every row of the alpha channel."""
    alpha = image.getchannel("A")
    width, height = image.size
    pixels = alpha.load()
    rows = []
    for y in range(height):
        left, right, count = width, -1, 0
        for x in range(width):
            if pixels[x, y] > 16:
                count += 1
                if x < left:
                    left = x
                if x > right:
                    right = x
        rows.append((count, left, right))
    return rows


def head_window(image):
    """(centre x, top y, head height) of the head, measured from the opaque mask."""
    rows = opaque_rows(image)
    counts = [count for count, _, _ in rows]
    top = next(y for y, count in enumerate(counts) if count)
    bottom = len(rows) - 1 - next(i for i, count in enumerate(reversed(counts)) if count)
    peak = max(counts[top : bottom + 1])
    limit = top + (bottom - top) * 0.12
    shoulder = next(
        (y for y in range(int(limit), bottom + 1) if counts[y] >= peak * SHOULDER_RATIO),
        bottom,
    )
    # The photo adds a neck below the chin; the drawn eye line sits at roughly 0.75 of it.
    head_height = (shoulder - top) * 0.8
    weights = [rows[y][0] for y in range(top, int(shoulder) + 1)]
    xs = [
        (rows[y][1] + rows[y][2]) / 2.0
        for y in range(top, int(shoulder) + 1)
        if rows[y][0]
    ]
    centre = sum(x * w for x, w in zip(xs, weights)) / sum(weights)
    return centre, top, head_height


def build(source, target, size, pad_x, alpha_floor, window=None):
    image = Image.open(source).convert("RGBA")
    centre, top, head_height = window or head_window(image)
    side = head_height / HEAD_SHARE
    left = centre - side / 2
    upper = top - side * HEAD_TOP

    # crop() keeps the window inside the source, filling anything outside with
    # transparent pixels — exactly what a head near the frame edge needs.
    box = (int(round(left)), int(round(upper)), int(round(left + side)), int(round(upper + side)))
    crop = image.crop(box)
    canvas = crop.resize((size, size), Image.Resampling.LANCZOS)

    if pad_x:
        # Re-centre horizontally using the opaque mask so off-centre poses stay put.
        bbox = canvas.getchannel("A").getbbox()
        if bbox:
            shift = int(round((size / 2 - (bbox[0] + bbox[2]) / 2) * pad_x))
            if shift:
                shifted = Image.new("RGBA", (size, size), (0, 0, 0, 0))
                shifted.alpha_composite(canvas, (shift, 0))
                canvas = shifted

    if alpha_floor > 1:
        # Kill near-invisible matte fringes left over from the cutout.
        alpha = canvas.getchannel("A").point(lambda v: 0 if v < alpha_floor else v)
        canvas.putalpha(alpha)

    canvas.save(target, "PNG", optimize=True)
    return side, head_height


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", default=r"E:\works\ultralman\public\img\cutout")
    parser.add_argument("--out", default=os.path.join(here, "..", "assets", "avatars"))
    parser.add_argument("--size", type=int, default=384)
    parser.add_argument("--pad-x", type=float, default=0.0, help="1 = centre on the opaque mask")
    parser.add_argument("--alpha-floor", type=int, default=24)
    args = parser.parse_args()

    out = os.path.normpath(args.out)
    os.makedirs(out, exist_ok=True)
    missing = 0
    for key, name in SOURCES.items():
        source = os.path.join(args.source, name)
        if not os.path.isfile(source):
            print(f"skip {key}: {source} not found")
            missing += 1
            continue
        target = os.path.join(out, key + ".png")
        side, head = build(source, target, args.size, args.pad_x, args.alpha_floor, WINDOWS.get(key))
        print(f"{key}: window {side:.0f}px, head {head:.0f}px -> {target}")
    return 1 if missing == len(SOURCES) else 0


if __name__ == "__main__":
    sys.exit(main())
