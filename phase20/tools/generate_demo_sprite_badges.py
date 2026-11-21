#!/usr/bin/env python3
"""Deterministic synthetic BMP test portraits, NOT actual Pokémon sprites.

Produces three clearly abstract badges for fictional Phase19 demo species
637, 260 and 282. For CI/emulator testing of graphical loading only.
No internet, ROM, GBA game, save, or image dependencies.
"""
from __future__ import annotations

from pathlib import Path
import argparse
import struct


def badge(species: int) -> bytes:
    if species not in (637, 260, 282):
        raise ValueError("Only 637, 260, 282 synthetic test ID badges supported")
    colors = {
        637: ((241, 142, 62), (254, 217, 97)),   # bright amber sunburst
        260: ((55, 142, 228), (91, 209, 246)),   # blue crystal
        282: ((91, 194, 123), (187, 247, 193)),  # green constellation
    }
    a, b = colors[species]
    out = bytearray()
    for y in range(47, -1, -1):
        for x in range(48):
            dx, dy = x - 23, y - 23
            r2 = dx * dx + dy * dy
            r, g, bl = 255, 0, 255
            if species == 637:
                if 13*13 <= r2 <= 19*19 or (abs(dx) <= 4 and abs(dy) <= 4):
                    r, g, bl = a if (x+y)%6 < 3 else b
                if abs(dx) <= 7 and abs(dy) <= 7 and r2 < 64:
                    r, g, bl = b
            elif species == 260:
                if abs(dx) + abs(dy) <= 23:
                    r, g, bl = a if abs(dx)+abs(dy)>12 else b
                if abs(dx) < 3 and abs(dy) < 3:
                    r, g, bl = (220, 244, 255)
            else:
                if 8*8 <= r2 <= 17*17 or (dx * dy == 0 and r2 <= 20*20):
                    r, g, bl = a if (x + 2*y)%5 < 3 else b
                if r2 < 28:
                    r, g, bl = b
            out.extend((bl, g, r))
    assert len(out) == 6912
    return out


def write_bmp(path: Path, rgb_bottom_up: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        raise FileExistsError(f"Refusing overwrite: {path}")
    header = (struct.pack("<2sIHHI", b"BM", 54 + len(rgb_bottom_up), 0, 0, 54)
              + struct.pack("<IiiHHIIiiII", 40, 48, 48, 1, 24, 0,
                            len(rgb_bottom_up), 0, 0, 0, 0))
    path.write_bytes(header + rgb_bottom_up)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    for species in (637, 260, 282):
        filename = args.out / f"{species:04d}.bmp"
        write_bmp(filename, badge(species))
        print(filename, "6966 bytes")


if __name__ == "__main__":
    main()
