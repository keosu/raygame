# Example game icons

The five `examples/<game>/artwork/icon.svg` files are original geometric artwork (including Engine Lab's connected circle, square and triangle): a cyan ship,
mint/blue/gold bouncing balls, neon bricks and a paddle, and a pink radial bullet core.
They share a dark rounded tile while keeping distinct silhouettes at small sizes.

Generated assets are committed, so normal xmake and CI builds need no image tools:

- `examples/<game>/resources/windows/<game>.ico` and `.rc`: 16, 20, 24, 32, 48, 64,
  128 and 256 px, embedded in the EXE with RC.
- `examples/<game>/assets/icon.png`: 256 px native window icon (Windows/Linux),
  copied to `bin/assets/<game>/icon.png` for desktop runs.
- `examples/<game>/android/res`: launcher PNGs at five densities, Android 8+ adaptive layers,
  and Android 13+ monochrome layers for themed icons. The system controls the mask.

After editing the SVGs, regenerate from the repository root:

```text
python -m pip install Pillow==12.3.0 resvg-py==0.2.6
python scripts/generate-icons.py
```

The generator also writes `build/icons-preview.png`. Windows resource validation is
`pwsh -NoProfile -File scripts/verify-windows-icons.ps1`; it compares all eight
embedded icon images against each game's ICO without executing the EXEs. The Android
build script checks launcher/adaptive/themed icon resources inside each APK.

On Windows, xmake compiles `<game>.rc` with the resource compiler and links its
output into the EXE's PE resource section. Windows Explorer reads those embedded
ICO images without needing an external icon file. The PNG is loaded separately by
`SetWindowIcon` for the running window. FORGE keeps its own generator, ICO/RC and
runtime assets in [its independent project](../forge_runner/README.md).
