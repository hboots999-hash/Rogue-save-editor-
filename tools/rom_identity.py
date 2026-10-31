#!/usr/bin/env python3
"""Local GBA ROM fingerprint for PKSM Rogue compatibility evidence.

The whole ROM is NEVER uploaded or embedded in the report. Only a SHA-256,
size and GBA header fields are printed. This deliberately refuses CIA, SAV,
empty/truncated files or files with a bad GBA header checksum.

A matching hash alone still does not prove an edited SAV can be loaded.
"""
from __future__ import annotations
import argparse
from hashlib import sha256
from pathlib import Path
import sys

ALLOWED_SIZE = {4, 8, 16, 32}  # standard GBA cartridge sizes, MiB

def fingerprint(path: Path) -> dict:
    length=path.stat().st_size
    if length not in {n*1024*1024 for n in ALLOWED_SIZE}:
        raise ValueError("Not a standard 4/8/16/32 MiB GBA ROM image")
    with path.open("rb") as f:
        header=f.read(0xC0)
        if len(header)<0xC0:
            raise ValueError("GBA header missing")
        title=header[0xA0:0xAC]
        game_code=header[0xAC:0xB0]
        maker=header[0xB0:0xB2]
        if any(x<32 or x>126 for x in game_code):
            raise ValueError("Game code is not ASCII")
        if header[0xB2]!=0x96:
            raise ValueError("Missing GBA fixed header byte")
        check=(0x100-((sum(header[0xA0:0xBD])+0x19)&0xFF))&0xFF
        if check!=header[0xBD]:
            raise ValueError("Invalid GBA header checksum")
        h=sha256()
        f.seek(0)
        for block in iter(lambda:f.read(1024*1024),b""):
            h.update(block)
    return {
        "sha256": h.hexdigest(),
        "bytes": length,
        "title": title.rstrip(b"\0").decode("ascii",errors="replace"),
        "game_code": game_code.decode("ascii"),
        "maker_code": maker.decode("ascii",errors="replace"),
        "revision": header[0xBC],
        "header_checksum": f"{check:02x}",
        "compatibility_verified": False,
    }

def main() -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom",type=Path,help="locally owned .gba file ONLY, never upload to GitHub")
    args=parser.parse_args()
    try:
        result=fingerprint(args.rom)
    except (OSError,ValueError) as exc:
        print("ROM FINGERPRINT REFUSED:",exc,file=sys.stderr)
        return 1
    for key,value in result.items():
        print(f"{key}: {value}")
    print("WARNING: ROM hash is identification only, NOT a safe-save-write authorization.")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
