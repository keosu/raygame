"""Deterministic, supersampled lit glass/ceramic marbles. Requires Pillow.

python examples/marble_temple/artwork/generate-marbles.py [--output <directory>]
"""
import argparse
import math
from pathlib import Path
from PIL import Image

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--output", type=Path, default=Path(__file__).resolve().parents[1] / "assets")
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
colors = [(226, 69, 67), (247, 178, 45), (57, 183, 114), (47, 143, 222), (173, 86, 209)]
for index, color in enumerate(colors):
    size = 512
    pixels = bytearray(size * size * 4)
    for y in range(size):
        for x in range(size):
            nx, ny = (x + 0.5 - size / 2) / (size / 2 / 1.12), (y + 0.5 - size / 2) / (size / 2 / 1.12)
            radius = math.hypot(nx, ny)
            alpha = max(0, min(1, (1 - radius) * size / 2 / 1.12 + 0.5))
            if alpha == 0:
                continue
            z = math.sqrt(max(0, 1 - radius * radius))
            diffuse = max(0, nx * -0.38 + ny * -0.48 + z * 0.78)
            highlight = max(0, nx * -0.35 + ny * -0.45 + z * 0.82) ** 48
            rim = (1 - z) ** 4 * (0.14 + max(0, nx + ny) * 0.14)
            glow = math.exp(-((nx + 0.25) ** 2 / 0.12 + (ny + 0.37) ** 2 / 0.055)) * 0.27
            reflection = math.exp(-((ny - 0.63) ** 2 / 0.006 + nx ** 2 / 0.38)) * 0.16
            rgb = [min(255, round(c * (0.32 + diffuse * 0.82) + 255 * (highlight * 0.8 + glow + rim + reflection))) for c in color]
            pos = (y * size + x) * 4
            pixels[pos:pos + 4] = bytes(rgb + [round(alpha * 255)])
    image = Image.frombytes("RGBA", (size, size), bytes(pixels))
    image.resize((256, 256), Image.Resampling.LANCZOS).save(args.output / f"ball-{index}.png")
size = 128
image = Image.new("RGBA", (size, size))
image.putdata([(0, 5, 12, round(90 * math.exp(-(((x - 64) / 37) ** 2 + ((y - 64) / 31) ** 2) * 2))) for y in range(size) for x in range(size)])
image.save(args.output / "shadow.png")
print("Generated five marbles and one soft shadow")
