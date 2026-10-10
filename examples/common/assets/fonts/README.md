# Example fonts

- **Rajdhani SemiBold**: display titles and score counters. Source:
  https://github.com/google/fonts/tree/main/ofl/rajdhani
- **Barlow Medium**: interface labels and controls. Source:
  https://github.com/google/fonts/tree/main/ofl/barlow

Both fonts are distributed under the SIL Open Font License 1.1. Their unmodified
license notices are included alongside the TTF files. The four small examples share
this directory. The `example_assets` target copies it to `bin/assets/fonts`, and
Android builds merge it into each game's staged assets directory. No system font
installation or runtime download is required. FORGE carries its own font files and
licenses so its independent SDK project can be copied as a whole.
