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


def generate(with_party: bool = False, max_bag_capacity: bool = False) -> bytes:
    flash = bytearray(RAW_SIZE)
    for logical_id in range(SECTORS):
        start = logical_id * SECTOR_BYTES
        sector = memoryview(flash)[start:start + SECTOR_BYTES]
        if logical_id == 0:
            struct.pack_into("<I", sector, 0x4C, KEY)
        if logical_id == 1:
            struct.pack_into("<I", sector, 0x234, 0)  # no party data
            struct.pack_into("<I", sector, 0x4A8, MONEY ^ KEY)
            if max_bag_capacity:
                sector[0x4AC] = 0  # default type sort mode
                sector[0x4AD] = 37  # pinned EX 2.1.2 maximum bag upgrade
            if with_party:
                # A disposable valid BoxPokemon, PID 24 -> order G,A,E,M.
                struct.pack_into("<I", sector, 0x234, 1)
                mon = 0x238
                pid, ot = 24, 0x11225566
                struct.pack_into("<II", sector, mon, pid, ot)
                sector[mon + 0x13] = 2  # populated, not bad egg
                plain = bytearray(48)
                struct.pack_into("<H", plain, 0, 25)  # candidate species ID
                plain[24:30] = bytes((1, 2, 3, 4, 5, 6))
                struct.pack_into("<I", plain, 40, 1 | 2 << 5)
                internal_sum = sum(struct.unpack_from("<H", plain, i)[0] for i in range(0, 48, 2))
                struct.pack_into("<H", sector, mon + 28, internal_sum & 0xFFFF)
                for i in range(0, 48, 4):
                    val = struct.unpack_from("<I", plain, i)[0] ^ (pid ^ ot)
                    struct.pack_into("<I", sector, mon + 32 + i, val)

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
    parser.add_argument("--party", action="store_true", help="include one synthetic party Pokemon")
    parser.add_argument("--max-bag-capacity", action="store_true", help="set candidate bag capacity upgrades to 37")
    args = parser.parse_args()
    data = generate(with_party=args.party, max_bag_capacity=args.max_bag_capacity)
    assert len(data) == RAW_SIZE
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"Synthetic non-game fixture: {args.output} ({len(data)} bytes)")
    print(f"Expected: marker 6, money 123456, party {1 if args.party else 0}, bag IDs 1x50 and 2x75")


if __name__ == "__main__":
    main()
