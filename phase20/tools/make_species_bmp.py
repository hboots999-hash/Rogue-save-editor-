#!/usr/bin/env python3
"""Convert user-owned Pokémon portrait PNG to the exact 3DS 48x48 BGR24 BMP.

No network, ROM, save or installed-title access. Requires Pillow on a desktop.
Each destination file is 6,966 bytes with magenta (#FF00FF) transparency.
Usage:
    python3 phase20/tools/make_species_bmp.py 1370 annihilape.png --out sprites
The species ID is the game's *internal* save ID, not necessarily National Dex.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import struct


def convert(source: Path, species: int, out_dir: Path) -> Path:
    if not 1 <= species <= 9999:
        raise ValueError("Internal species ID must be in 1..9999")
    if source.suffix.lower() != ".png":
        raise ValueError("Input must be a PNG you own / are permitted to use")
    from PIL import Image  # pip install Pillow; no runtime 3DS dependency

    # File remains a local-only user asset. Do not fetch copyrighted sprites
    # from GitHub during CI or include user files in source control.
    with Image.open(source) as loaded:
        if loaded.format != "PNG" or loaded.width < 1 or loaded.height < 1:
            raise ValueError("Invalid PNG")
        if loaded.width > 2048 or loaded.height > 2048:
            raise ValueError("Refuse excessively large input image")
        icon = loaded.convert("RGBA")
    scale = min(46 / icon.width, 46 / icon.height, 1.0)
    nw = max(1, round(icon.width * scale))
    nh = max(1, round(icon.height * scale))
    if (nw, nh) != icon.size:
        icon = icon.resize((nw, nh), Image.Resampling.NEAREST)
    canvas = Image.new("RGBA", (48, 48), (255, 0, 255, 0))
    canvas.alpha_composite(icon, ((48 - nw) // 2, (48 - nh) // 2))
    raw = bytearray()
    rgba = canvas.load()
    for y in range(47, -1, -1):
        for x in range(48):
            r, g, b, alpha = rgba[x, y]
            if alpha < 128:
                r, g, b = 255, 0, 255  # 3DS app's keyed transparency
            raw.extend((b, g, r))
    assert len(raw) == 48 * 48 * 3
    size = 54 + len(raw)
    header = (struct.pack("<2sIHHI", b"BM", size, 0, 0, 54)
              + struct.pack("<IiiHHIIiiII", 40, 48, 48, 1, 24, 0,
                            len(raw), 0, 0, 0, 0))
    assert len(header) == 54
    out_dir.mkdir(parents=True, exist_ok=True)
    target = out_dir / f"{species:04d}.bmp"
    # Don't clobber a previous owner's manually curated portrait.
    with target.open("xb") as output:
        output.write(header)
        output.write(raw)
    return target


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("species", type=int, help="Game internal species ID")
    p.add_argument("image", type=Path, help="Local user-provided source PNG")
    p.add_argument("--out", type=Path, default=Path("sprites"),
                   help="SD sprites folder")
    args = p.parse_args()
    output = convert(args.image, args.species, args.out)
    print(f"{output}: 6966 bytes, copy to SD:/3ds/PKSM-Rogue/sprites/")


if __name__ == "__main__":
    main()
