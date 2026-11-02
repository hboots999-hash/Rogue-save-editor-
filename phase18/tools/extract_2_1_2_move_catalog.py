#!/usr/bin/env python3
"""Read-only, SHA-pinned 2.1.2 CIA -> move-name/PP CSV.

Does not export or modify the ROM. No private binary data is committed.
Never uses move IDs as authorization for save writing.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import struct
from pathlib import Path

ROM_SHA = "111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3"
ROM_SIZE = 32 * 1024 * 1024
NAMES = 0x004C283C
BATTLE_TABLES = (0x00BD27D0, 0x00BD8E30)
COUNT = 936
NAME_WIDTH = 13
BATTLE_WIDTH = 28
PACKED_ROW_WIDTH = 15
CATALOG_SHA = "3d2d63e5fd77371a76540d07a6c5a4d20a9ddae7acea9f03035d86936ee34fa4"

def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def align(x: int) -> int:
    return (x+63) & ~63

def cia_rom(data: bytes) -> bytes:
    if len(data)<0x5000 or struct.unpack_from("<I", data, 0)[0]!=0x2020:
        raise ValueError("Unsupported or truncated CIA")
    cert,ticket,tmd = struct.unpack_from("<III",data,8)
    content=align(align(align(0x2020+cert)+ticket)+tmd)
    size=struct.unpack_from("<Q",data,24)[0]
    if size<0x2000000 or content+size>len(data) or data[content+256:content+260]!=b"NCCH":
        raise ValueError("Invalid CIA NCCH content")
    exefs=content+512*struct.unpack_from("<I",data,content+0x1a0)[0]
    if data[exefs:exefs+8].rstrip(bytes(1))!=b".code":
        raise ValueError("GBA VC ExeFS .code missing")
    off,length=struct.unpack_from("<II",data,exefs+8)
    start=exefs+512+off
    if length<ROM_SIZE or start+length>len(data):
        raise ValueError("Truncated ExeFS .code")
    rom=data[start:start+ROM_SIZE]
    if (rom[0xa0:0xac],rom[0xac:0xb0])!=(b"POKEMON EMER",b"BPEE"):
        raise ValueError("Not the expected GBA header")
    if ((-sum(rom[0xa0:0xbd])-0x19)&255)!=rom[0xbd]:
        raise ValueError("Corrupted GBA header checksum")
    if sha(rom)!=ROM_SHA:
        raise ValueError("ROM SHA does not match approved 2.1.2 research fingerprint")
    return rom

def decode_name(chunk: bytes) -> str:
    if len(chunk)!=NAME_WIDTH or 255 not in chunk:
        return ""
    chars=[]
    for c in chunk[:chunk.index(255)]:
        if c==0: chars.append(" ")
        elif 0xbb<=c<=0xd4: chars.append(chr(ord("A")+c-0xbb))
        elif 0xd5<=c<=0xee: chars.append(chr(ord("a")+c-0xd5))
        elif c==0xae: chars.append("-")
        elif c==0xb4: chars.append("'")
        elif c==0xa3: chars.append("2")
        else: chars.append("[0x%02x]"%c)
    return "".join(chars).strip()

def rows(rom: bytes):
    if len(rom)!=ROM_SIZE or sha(rom)!=ROM_SHA:
        raise ValueError("Not the pinned ROM")
    if decode_name(rom[NAMES+NAME_WIDTH:NAMES+2*NAME_WIDTH])!="Pound":
        raise ValueError("Name table alignment incorrect")
    if decode_name(rom[NAMES+NAME_WIDTH*33:NAMES+NAME_WIDTH*34])!="Tackle":
        raise ValueError("Move IDs not confirmed")
    for start in BATTLE_TABLES:
        if rom[start+28+2:start+28+6]!=bytes((40,0,100,35)):
            raise ValueError("Battle move PP table alignment incorrect")
    for mid in range(COUNT):
        name=decode_name(rom[NAMES+mid*NAME_WIDTH:NAMES+(mid+1)*NAME_WIDTH])
        pp=[rom[off+mid*BATTLE_WIDTH+5] for off in BATTLE_TABLES]
        kind=("none" if mid==0 else
              "ordinary" if mid<=848 else
              "reserved" if not name else "special")
        candidate=(kind=="ordinary" and mid!=165 and
                   bool(name) and "[0x" not in name and
                   pp[0]==pp[1] and 1<=pp[0]<=64)
        yield (mid,name,kind,pp[0],pp[1],int(pp[0]==pp[1]),int(candidate))

def exact_packed_catalog(rom: bytes) -> bytes:
    """Emit ONLY name[13]+PPa+PPb metadata, never the 32MiB ROM.

    The result may stay locally on the owner's SD card. It MUST NOT be
    committed to git or treated as an authorization to write a game save.
    """
    if len(rom)!=ROM_SIZE or sha(rom)!=ROM_SHA:
        raise ValueError("Pinned GBA ROM digest mismatch")
    packed=bytearray()
    for move_id in range(COUNT):
        packed.extend(rom[NAMES+move_id*NAME_WIDTH:
                          NAMES+(move_id+1)*NAME_WIDTH])
        packed.append(rom[BATTLE_TABLES[0]+move_id*BATTLE_WIDTH+5])
        packed.append(rom[BATTLE_TABLES[1]+move_id*BATTLE_WIDTH+5])
    if len(packed)!=COUNT*PACKED_ROW_WIDTH or sha(packed)!=CATALOG_SHA:
        raise ValueError("ROM move catalog authentication failed")
    return bytes(packed)

def main() -> None:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("cia",type=Path)
    ap.add_argument("--out",required=True,type=Path,
                    help="Local CSV path; never commit private files")
    ap.add_argument("--packed",type=Path,
                    help="Optional LOCAL 14040-byte metadata index; NEVER commit")
    ns=ap.parse_args()
    rom=cia_rom(ns.cia.read_bytes())
    entries=list(rows(rom))
    with ns.out.open("w",encoding="utf-8",newline="") as f:
        writer=csv.writer(f)
        writer.writerow(("move_id","name","kind","pp_variant_a","pp_variant_b",
                         "pp_agreement","research_ui_candidate_only"))
        writer.writerows(entries)
    if ns.packed:
        ns.packed.write_bytes(exact_packed_catalog(rom))
        print("Local catalog SHA:",CATALOG_SHA)
    print("ROM SHA:",ROM_SHA)
    print("Records:",len(entries))
    print("Ordinary PP conflicts:",sum(x[2]=="ordinary" and not x[5] for x in entries))
    print("Read-only UI candidates:",sum(x[6] for x in entries))
    print("Export/edit authorisation: DISABLED")

if __name__=="__main__":
    main()
