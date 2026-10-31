#!/usr/bin/env python3
"""Generate a tiny fully synthetic 128KiB Emerald Rogue candidate for CI.

This has no real user's information and is not playable. Only the sector checksum
format, toy BoxPokemon fields and ball-pocket anchors needed by tests are present.
"""
import argparse
from pathlib import Path
import struct

SECTOR = 4096
SIZE = 131072
def w16(b, off, x): struct.pack_into('<H', b, off, x)
def w32(b, off, x): struct.pack_into('<I', b, off, x)
def checksum(b):
    total = sum(struct.unpack_from('<I',b,i)[0] for i in range(0,0xff4,4)) & 0xffffffff
    return ((total>>16)+total)&0xffff

def make():
    flash=bytearray(SIZE)
    s0=memoryview(flash)[:SECTOR]
    s1=memoryview(flash)[SECTOR:2*SECTOR]
    s10=memoryview(flash)[10*SECTOR:11*SECTOR]
    key=0x1234abcd
    w32(s0,0x4c,key)
    w32(s1,0x4a8,40000^key)
    w32(s1,0x234,1)
    mon=bytearray(104)
    pid,ot=2400,0x44556677
    w32(mon,0,pid)
    w32(mon,4,ot)
    mon[0x13]=2
    plain=bytearray(48)
    w16(plain,0,25)
    w16(plain,12,33)
    plain[20]=20
    plain[24:30]=bytes((4,252,0,252,0,0))
    w32(plain,40,sum(10<<(5*i) for i in range(6)))
    w16(mon,28,sum(struct.unpack_from('<H',plain,i)[0] for i in range(0,48,2))&0xffff)
    for i in range(0,48,4):
        w32(mon,32+i,struct.unpack_from('<I',plain,i)[0]^(pid^ot))
    s1[0x238:0x238+104]=mon
    for i,(item,amount) in enumerate(((113,10),(1,5),(5,5))):
        w16(s1,0x578+4*i,item)
        w16(s1,0x578+4*i+2,amount^(key&0xffff))
    s10[0xe00:0xe06]=bytes((6,0,2,0xdf,0x6b,1))
    for i in range(14):
        sector=memoryview(flash)[i*SECTOR:(i+1)*SECTOR]
        w16(sector,0xff4,i)
        w16(sector,0xff6,checksum(sector))
        w32(sector,0xff8,0x08012025)
        w32(sector,0xffc,0)
    return flash

def main():
    p=argparse.ArgumentParser()
    p.add_argument('output',type=Path)
    a=p.parse_args()
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_bytes(make())
    print('Generated synthetic candidate (not a playable game save)')

if __name__=='__main__':
    main()
