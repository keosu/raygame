#!/usr/bin/env python3
"""Cuts the fighter action poses in assets/poses/ out of official figure photos.

Usage:
    python tools/cut_poses.py [--source DIR]

Sources are Bandai product and blog photos of posable figures, renamed in the source folder:
  shf_z_*   S.H.Figuarts Ultraman Z Original (tamashiiweb.com/item/13596, 14009)
  frs_z_*   Figure-rise Standard Ultraman Z Original (bandaihobby.hatenablog.com/entry/20230731)
  shf_rk_*  S.H.Figuarts Red King (tamashiiweb.com/item/14686)
  ua_rk_*   ULTRA-ACT Red King (tamashiiweb.com/item/1761)

The background is removed with rembg (isnet-general-use; pip install rembg), keeping the
largest shape plus anything big attached to it (a held rock), so captions and floor
reflections drop out. Every pose of one fighter is drawn at one scale in game: a pose is
`height * rel` pixels tall, where `rel` is its height relative to standing upright. Colours
from the other product lines are matched to the fighter's first pose. Writes poses.txt with
each pose's body centre (share of the width, measured on the upper body) and facing.
"""

import argparse
import os
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

# key: (file, crop (l, t, r, b) as shares of the photo, fighter height px, rel, facing, options)
# facing is the way the figure looks in the photo: -1 left, 0 front, 1 right.
POSES = {
    "z_idle": ("shf_z_stance.jpg", None, 800, 0.93, 0, {}),
    "z_ready": ("shf_z_open.jpg", None, 800, 0.97, 0, {}),
    "z_point": ("shf_z_point.jpg", None, 800, 0.98, -1, {}),
    "z_punch": ("frs_z_kick_punch.jpg", (0.51, 0, 1, 1), 800, 0.88, 1, {"match": "z_idle"}),
    "z_kick": ("frs_z_kick_punch.jpg", (0, 0, 0.49, 1), 800, 0.86, -1, {"match": "z_idle"}),
    "z_kneel": ("frs_z_kneel.jpg", None, 800, 0.62, 0, {"match": "z_idle"}),
    "z_beam": ("shf_z_zestium.jpg", None, 800, 0.92, -1, {}),
    "z_win": ("frs_z_spread.jpg", None, 800, 1.0, 0, {"match": "z_idle"}),
    "rk_idle": ("shf_rk_stand.jpg", None, 860, 1.0, 0, {}),
    "rk_roar": ("shf_rk_roar.jpg", None, 860, 1.0, 0, {}),
    "rk_charge": ("shf_rk_lunge.jpg", None, 860, 0.8, 1, {}),
    "rk_down": ("shf_rk_down.jpg", None, 860, 0.72, 1, {}),
    "rk_throw": ("shf_rk_throw.jpg", None, 860, 0.74, -1, {}),
    "rk_swing": ("ua_rk_swing.jpg", (0, 0, 1, 0.86), 860, 0.84, -1, {"match": "rk_idle"}),
    "rk_walk": ("ua_rk_walk.jpg", (0, 0, 1, 0.86), 860, 0.96, -1, {"match": "rk_idle"}),
    "rk_lift": ("ua_rk_lift.jpg", (0, 0, 1, 0.9), 860, 0.97, -1, {"match": "rk_idle"}),
}

ALPHA_FLOOR = 24


def keep_main(alpha):
    """Drops loose specks and captions: keep the largest blob and blobs at least 8 % of it."""
    mask = alpha > 128
    labels, count = ndimage.label(mask)
    if count <= 1:
        return alpha
    sizes = ndimage.sum(mask, labels, range(1, count + 1))
    biggest = sizes.max()
    keep = np.isin(labels, [i + 1 for i, s in enumerate(sizes) if s >= biggest * 0.08])
    keep = ndimage.binary_dilation(keep, iterations=3)
    return np.where(keep, alpha, 0).astype(np.uint8)


def color_match(image, reference):
    """Moves the opaque pixels' per-channel mean and spread onto the reference's."""
    a = np.asarray(image).astype(np.float32)
    r = np.asarray(reference).astype(np.float32)
    mask = a[..., 3] > 128
    ref_mask = r[..., 3] > 128
    for c in range(3):
        src, ref = a[..., c][mask], r[..., c][ref_mask]
        scale = ref.std() / max(1.0, src.std())
        a[..., c][mask] = (src - src.mean()) * scale + ref.mean()
    return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8), "RGBA")


def body_centre(image):
    """Alpha-weighted centre of the top 60 % of the figure, as a share of the width."""
    alpha = np.asarray(image.getchannel("A")).astype(np.float32)
    upper = alpha[: int(alpha.shape[0] * 0.6)]
    xs = np.arange(alpha.shape[1], dtype=np.float32)
    weight = upper.sum()
    return float((upper.sum(axis=0) * xs).sum() / weight / alpha.shape[1]) if weight else 0.5


def cut(session, source, file, crop):
    from rembg import remove

    image = Image.open(os.path.join(source, file)).convert("RGB")
    if crop:
        w, h = image.size
        image = image.crop((int(crop[0] * w), int(crop[1] * h), int(crop[2] * w), int(crop[3] * h)))
    out = remove(image, session=session).convert("RGBA")
    alpha = keep_main(np.asarray(out.getchannel("A")))
    out.putalpha(Image.fromarray(alpha))
    return out.crop(out.getchannel("A").point(lambda v: 255 if v > ALPHA_FLOOR else 0).getbbox())


def main():
    from rembg import new_session

    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", default=os.path.join(here, "..", "..", "..", "build", "uq-src2"))
    parser.add_argument("--out", default=os.path.join(here, "..", "assets", "poses"))
    args = parser.parse_args()
    out = os.path.normpath(args.out)
    os.makedirs(out, exist_ok=True)
    session = new_session("isnet-general-use")

    references = {}
    lines = ["# key anchor_x facing  (generated by tools/cut_poses.py)"]
    for key, (file, crop, height, rel, facing, options) in POSES.items():
        if not os.path.isfile(os.path.join(args.source, file)):
            print(f"missing {file}")
            return 1
        image = cut(session, args.source, file, crop)
        if options.get("match") in references:
            image = color_match(image, references[options["match"]])
        target_h = int(round(height * rel))
        target_w = max(1, int(round(image.width * target_h / image.height)))
        image = image.resize((target_w, target_h), Image.Resampling.LANCZOS)
        image.putalpha(image.getchannel("A").point(lambda v: 0 if v < ALPHA_FLOOR else v))
        references[key] = image
        image.save(os.path.join(out, key + ".png"), "PNG", optimize=True)
        anchor = body_centre(image)
        lines.append(f"{key} {anchor:.3f} {facing}")
        print(f"{key}: {image.width}x{image.height} anchor {anchor:.3f}")
    with open(os.path.join(out, "poses.txt"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
