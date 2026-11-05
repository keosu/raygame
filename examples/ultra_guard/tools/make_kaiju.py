#!/usr/bin/env python3
"""Prepares the built-in kaiju sprites (assets/kaiju/) from cut-outs.

A kaiju PNG is a single still frame (or a horizontal strip of frames). The game
scales it to `size` world pixels and aligns `feet` (0-1, measured from the top of
the image) with the ground line; both are set in assets/kaiju/kaiju.txt. These
sources are full-body cut-outs, so the content bounding box is all that goes in:
no letterboxing, no background.

Usage:
    python make_kaiju.py [--out DIR] [--source DIR]
"""

import argparse
import os

from PIL import Image, ImageEnhance

# key -> (source, channel shift applied after cropping[, share of the height trimmed off the bottom])
SOURCES = {
    "baltan": (r"E:\works\raygame\build\kaiju_dl\baltan\10.png", None),
    "gomora": (r"E:\works\raygame\build\kaiju_dl\gomora\01.png", None),
    "eleking": (r"E:\works\raygame\build\kaiju_dl\eleking\06.png", {"r": -34, "g": 14, "b": 62}),
    "redking": (r"E:\works\raygame\build\kaiju_dl\redking\04.png", {"r": 40, "b": -46}),
    "zetton": (r"E:\works\raygame\build\kaiju_dl4\zetton\full.png", None),
    # v2.0: the old Belial cut-out had lost its black suit to keying; this is the wiki's "Ultraman Belial".
    "belial": ("Ultraman_Belial.png", None),
    # Ultra series wiki renders (ultra.fandom.com, fetched with a Referer header): Ultraseven
    # King Joe render, Alien MetronI, TregearRender, Gatanothor render.
    "kingjoe": ("kingjoe.png", None),
    "metron": ("metron.png", None),
    "tregear": ("tregear.png", None),
    "gatanothor": ("gatanothor.png", None),
    # v2.0, same wiki: Bemstar(X) Render, Gabora Render, Ultraman Tiga Golza Render (its display
    # base trimmed), Ultraman The Ultimate Hero Dada render, King-Pandon(Ginga) Render, Dark Zagi render.
    "bemstar": ("Bemstar_X__Render.png", None),
    "gabora": ("Gabora_Render.png", None),
    "golza": ("Ultraman_Tiga_Golza_Render.png", None, 0.045),
    "dada": ("Ultraman_The_Ultimate_Hero_Dada_render.png", None),
    "kingpandon": ("King-Pandon_Ginga__Render.png", None),
    "zagi": ("Dark_Zagi_render.png", None),
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
    parser.add_argument("--out", default=os.path.join(here, "..", "assets", "kaiju"))
    parser.add_argument("--source", default=".", help="directory for sources given as bare file names")
    args = parser.parse_args()

    out = os.path.normpath(args.out)
    os.makedirs(out, exist_ok=True)

    for key, (source, shift, *rest) in SOURCES.items():
        path = source if os.path.isabs(source) else os.path.join(args.source, source)
        if not os.path.isfile(path):
            print(f"skip {key}: {path} not found")
            continue
        image = cut(path)
        if rest and rest[0] > 0:  # a display base under the feet
            image = image.crop((0, 0, image.width, int(image.height * (1 - rest[0]))))
            image = image.crop(image.getchannel("A").getbbox())
        image = shift_channels(fit(image), shift)
        image.save(os.path.join(out, key + ".png"), "PNG", optimize=True)
        print(f"{key}: {image.size}")

    if os.path.isfile(SOURCES["baltan"][0]):
        clone = clone_of_tint(SOURCES["baltan"][0], 70, flip=True)
        clone.save(os.path.join(out, "baltan_clone.png"), "PNG", optimize=True)
        print(f"baltan_clone: {clone.size} (blue tint, mirrored)")


if __name__ == "__main__":
    main()
