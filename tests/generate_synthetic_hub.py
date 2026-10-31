#!/usr/bin/env python3
"""Write a deterministic Rogue hub layout into a generated fake GBA save.

Never uses real ROMs or user data. 4-byte coordinate stride is an OBSERVED
candidate profile; the pinned upstream Coords8 C struct has 2 bytes.
"""
import argparse
from pathlib import Path
import struct
from generate_synthetic_save import generate, checksum, SECTOR_DATA_LENGTHS

def make_hub_save():
    save=bytearray(generate())
    header=struct.pack("<HBHBH",6,2,27615,1,1574)
    body=(struct.pack("<H",222)+bytes(222*8)
          +struct.pack("<H",7)+bytes(7*4)
          +struct.pack("<H",2)+bytes((0x05,0))
          +struct.pack("<H",11)
          +b"".join(bytes((i,0,0,0)) for i in range(11))
          +struct.pack("<H",6)+bytes((0x05,0,0,0,0,0))
          +struct.pack("<H",192)+bytes(192*4))
    blob=bytearray().join(save[i*4096:i*4096+0xff4] for i in range(5,14))
    at=5*0xff4+0xe00
    assert header==bytes((6,0,2,0xdf,0x6b,1,0x26,0x06))
    blob[at:at+len(header+body)]=header+body
    for i in range(5,14):
        start=i*0xff4
        save[i*4096:i*4096+0xff4]=blob[start:start+0xff4]
        struct.pack_into("<H",save,i*4096+0xff6,
                         checksum(save[i*4096:(i+1)*4096],SECTOR_DATA_LENGTHS[i]))
    return bytes(save)

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("output",type=Path)
    a=p.parse_args()
    a.output.parent.mkdir(parents=True,exist_ok=True)
    data=make_hub_save()
    a.output.write_bytes(data)
    print("Synthetic hub: 11 areas with 0 & 2 built; 48 upgrades with 0 & 2 owned")
