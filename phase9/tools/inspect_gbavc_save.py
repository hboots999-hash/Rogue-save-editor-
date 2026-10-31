#!/usr/bin/env python3
"""Read-only inspector for *copied* 128KiB GBA VC Emerald Rogue save candidates.
No writes or repair. Assumed offsets are observed for 2.1.2 GBA-P-H168,
but passing this probe is NEVER permission to edit or restore a real save.
No user ROMs or saves are stored in this project.
"""
import argparse
import json
import struct
from pathlib import Path

SECTOR = 4096
SIZE = 131072
PARTY_STRIDE = 104
ORDERS = (
    "GAEM", "GAME", "GEAM", "GEMA", "GMAE", "GMEA",
    "AGEM", "AGME", "AEGM", "AEMG", "AMGE", "AMEG",
    "EGAM", "EGMA", "EAGM", "EAMG", "EMGA", "EMAG",
    "MGAE", "MGEA", "MAGE", "MAEG", "MEGA", "MEAG",
)

def u16(b, offset):
    return struct.unpack_from("<H", b, offset)[0]

def u32(b, offset):
    return struct.unpack_from("<I", b, offset)[0]

def sector_checksum(data):
    if len(data) % 4:
        raise ValueError("Unaligned sector")
    total = sum(struct.unpack_from("<%dI" % (len(data)//4), data)) & 0xffffffff
    return ((total >> 16) + (total & 0xffff)) & 0xffff

def nickname(chars):
    result = ""
    for v in chars:
        if v in (0, 255):
            break
        if 0xbb <= v <= 0xd4:
            result += chr(65 + v - 0xbb)
        elif 0xd5 <= v <= 0xee:
            result += chr(97 + v - 0xd5)
        elif 0xa1 <= v <= 0xaa:
            result += chr(48 + v - 0xa1)
        else:
            result += "?"
    return result

def party_mon(raw, start):
    p = raw[start:start+PARTY_STRIDE]
    if len(p) != PARTY_STRIDE:
        raise ValueError("Truncated party member")
    pid, ot = struct.unpack_from("<II", p)
    key = pid ^ ot
    plain = bytearray(48)
    for pos in range(0, 48, 4):
        struct.pack_into("<I", plain, pos, u32(p, 32+pos)^key)
    words = struct.unpack_from("<24H", plain)
    order = ORDERS[pid % 24]
    sub = {order[j]:plain[j*12:(j+1)*12] for j in range(4)}
    growth, attacks, effort, misc = (sub[t] for t in "GAEM")
    ivpack = u32(misc, 4)
    return {
        "nickname": nickname(p[8:18]),
        "species_id": u16(growth,0),
        "held_item_id": u16(growth,2),
        "level": p[84],
        "move_ids": list(struct.unpack_from("<4H", attacks)),
        "move_pp": list(attacks[8:12]),
        "ivs": [(ivpack >> (5*i)) & 31 for i in range(6)],
        "evs": list(effort[:6]),
        "checksum_valid": (sum(words)&0xffff)==u16(p,28),
    }

def inspect_bytes(raw):
    if len(raw)!=SIZE:
        raise ValueError("Invalid size: expected 131072 bytes")
    banks=[]
    for bank in (0,1):
        sectors={}
        counters=[]
        checks=[]
        for physical in range(bank*14,(bank+1)*14):
            off=physical*SECTOR
            sid=u16(raw,off+0xff4)
            if sid in sectors:
                raise ValueError("Duplicate sector ID in bank")
            sectors[sid]=off
            counters.append(u32(raw,off+0xffc))
            checks.append(u32(raw,off+0xff8)==0x08012025 and
                sector_checksum(raw[off:off+0xff4])==u16(raw,off+0xff6))
        if set(sectors)!=set(range(14)) or len(set(counters))!=1:
            banks.append({"bank":bank,"valid":False,"reason":"Incomplete sector map",
                          "sectors_checksum_valid":sum(checks)})
            continue
        s0, s1 = sectors[0], sectors[1]
        key=u32(raw,s0+0x4c)
        count=u32(raw,s1+0x234)
        if count>6:
            raise ValueError("Party size exceeds six")
        party=[party_mon(raw,s1+0x238+i*PARTY_STRIDE) for i in range(count)]
        items=[]
        gap=False
        contiguous=True
        for i in range(450):
            ident=u16(raw,s1+0x578+i*4)
            if ident==0:
                gap=True
                continue
            if gap:
                contiguous=False
            items.append({"slot":i,"id":ident,"quantity":u16(raw,s1+0x578+i*4+2)^(key&0xffff)})
        banks.append({
            "bank":bank,
            "valid":all(checks) and all(mon["checksum_valid"] for mon in party),
            "sectors_checksum_valid":sum(checks),
            "counter":counters[0],
            "party":party,
            "party_count":count,
            "money":u32(raw,s1+0x4a8)^key,
            "occupied_bag_slots":len(items),
            "bag_contiguous":contiguous,
            "bag_quantity_range_valid":all(1<=v["quantity"]<=999 for v in items),
            "rare_candy_id_113":next((v["quantity"] for v in items if v["id"]==113),None),
            "item_id_1":next((v["quantity"] for v in items if v["id"]==1),None),
        })
    candidates=[b for b in banks if b["valid"]]
    current=max(candidates,key=lambda b:b["counter"]) if candidates else None
    return {"read_only":True,"bytes":len(raw),"banks":banks,
            "current_bank":current["bank"] if current else None,
            "current":current}

if __name__=="__main__":
    ap=argparse.ArgumentParser(description="Inspect a copied save; does not write any files")
    ap.add_argument("copy", help="A copied 128-KiB GBA VC .sav, never edited")
    args=ap.parse_args()
    print(json.dumps(inspect_bytes(Path(args.copy).read_bytes()),indent=2))
