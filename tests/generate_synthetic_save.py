#!/usr/bin/env python3
"""Generate a disposable, entirely synthetic Rogue-format candidate save.

NOT a real game save and NOT for use with Emerald Rogue. All 128 KiB are
constructed locally from literals. It contains no user-supplied data.
Fixture targets candidate marker 6, zero party members, numeric bag IDs 1/2,
and 123456 money; neither ROM identity nor editing is implied.
"""
from pathlib import Path
import argparse
import struct

RAW_SIZE = 0x20000
SECTOR_BYTES = 0x1000
SECTORS = 14
SECTOR_DATA_LENGTHS = (
    0x0EF4, 0x0FF4, 0x0FF4, 0x0FF4, 0x0480,
    0x0FF4, 0x0FF4, 0x0FF4, 0x0FF4, 0x0FF4,
    0x0FF4, 0x0FF4, 0x0FF4, 0x05D0
)
SIG = 0x08012025
KEY = 0x11223344
MONEY = 123456
BAG_ITEMS = ((1, 50), (2, 75))


def checksum(buffer: bytes | bytearray, length: int) -> int:
    total = 0
    for at in range(0, length, 4):
        total = (total + struct.unpack_from("<I", buffer, at)[0]) & 0xFFFFFFFF
    return ((total >> 16) + total) & 0xFFFF


def generate() -> bytes:
    flash = bytearray(RAW_SIZE)
    for logical_id in range(SECTORS):
        start = logical_id * SECTOR_BYTES
        sector = memoryview(flash)[start:start + SECTOR_BYTES]
        if logical_id == 0:
            struct.pack_into("<I", sector, 0x4C, KEY)
        if logical_id == 1:
            struct.pack_into("<I", sector, 0x234, 0)  # no party data
            struct.pack_into("<I", sector, 0x4A8, MONEY ^ KEY)
            for i, (item_id, quantity) in enumerate(BAG_ITEMS):
                struct.pack_into("<HH", sector, 0x578 + i * 4,
                                 item_id, quantity ^ (KEY & 0xFFFF))
        if logical_id == 10:
            sector[0xE00:0xE06] = bytes((6, 0, 2, 0xDF, 0x6B, 1))
        # GBA sector metadata; all other padding is zero.
        struct.pack_into("<HHII", sector, 0xFF4,
                         logical_id, checksum(sector, SECTOR_DATA_LENGTHS[logical_id]),
                         SIG, 0)
    return bytes(flash)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data = generate()
    assert len(data) == RAW_SIZE
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"Synthetic non-game fixture: {args.output} ({len(data)} bytes)")
    print("Expected: marker 6, money 123456, party 0, bag IDs 1x50 and 2x75")


if __name__ == "__main__":
    main()
