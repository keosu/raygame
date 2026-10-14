#!/usr/bin/env python3
"""Prepares kaiju sprites for the "real" skin from cut-outs.

A skin kaiju PNG is a single still frame (or a horizontal strip of frames). The
game scales it to `size` world pixels and aligns `feet` (0-1, measured from the
top of the image) with the ground line. These sources are full-body cut-outs, so
the content bounding box is all that goes in: no letterboxing, no background.

Usage:
    python make_kaiju.py [--out DIR]
"""

import argparse
import os

from PIL import Image, ImageEnhance

# key -> (source, channel shift applied after cropping)
SOURCES = {
    "baltan": (r"E:\works\raygame\build\kaiju_dl\baltan\10.png", None),
    "gomora": (r"E:\works\raygame\build\kaiju_dl\gomora\01.png", None),
    "eleking": (r"E:\works\raygame\build\kaiju_dl\eleking\06.png", {"r": -34, "g": 14, "b": 62}),
    "redking": (r"E:\works\raygame\build\kaiju_dl\redking\04.png", {"r": 40, "b": -46}),
    "zetton": (r"E:\works\raygame\build\kaiju_dl4\zetton\full.png", None),
    "belial": (r"E:\works\ultralman\public\img\cutout\belial.png", None),
}

# Sprites are drawn at ~48 world pixels, so a long edge of 768 is plenty.
MAX_EDGE = 768


def cut(source):
    """Tight bounding box around the opaque content, no borders."""
    image = Image.open(source).convert("RGBA")
    box = image.getchannel("A").getbbox()
    return image.crop(box)


def fit(image):
    long_edge = max(image.size)
    if long_edge <= MAX_EDGE:
        return image
    scale = MAX_EDGE / long_edge
    return image.resize((int(image.width * scale), int(image.height * scale)), Image.Resampling.LANCZOS)


def clone_of_tint(source, hue_shift, flip=False):
    """Recolours a cut-out so a clone reads as a different individual."""
    image = fit(cut(source))
    if flip:
        image = image.transpose(Image.FLIP_LEFT_RIGHT)
    r, g, b, a = image.split()
    r = r.point(lambda v: max(0, min(255, v + hue_shift)))
    b = b.point(lambda v: max(0, min(255, v - hue_shift)))
    return Image.merge("RGBA", (r, g, b, a))


def shift_channels(image, shift):
    if not shift:
        return image
    channels = image.split()
    moved = [
        channel.point(lambda v, amount=shift.get("rgb"[index], 0): max(0, min(255, v + amount)))
        for index, channel in enumerate(channels[:3])
    ]
    return Image.merge("RGBA", (*moved, channels[3]))


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", default=os.path.join(here, "..", "assets", "skins", "real", "kaiju"))
    args = parser.parse_args()

    out = os.path.normpath(args.out)
    os.makedirs(out, exist_ok=True)

    for key, (source, shift) in SOURCES.items():
        image = shift_channels(fit(cut(source)), shift)
        image.save(os.path.join(out, key + ".png"), "PNG", optimize=True)
        print(f"{key}: {image.size}")

    clone = clone_of_tint(SOURCES["baltan"][0], 70, flip=True)
    clone.save(os.path.join(out, "baltan_clone.png"), "PNG", optimize=True)
    print(f"baltan_clone: {clone.size} (blue tint, mirrored)")


if __name__ == "__main__":
    main()
