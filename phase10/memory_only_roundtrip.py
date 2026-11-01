#!/usr/bin/env python3
"""Phase 10 memory-only H168 save roundtrip probe. Never touches installed saves.

No file I/O, no real save fingerprint hardcoded, no ROM/save bytes committed.
Caller MUST pin the exact original SHA256; output is BYTES only, not a release.
No authority to restore the draft to actual game or console.
"""
from dataclasses import dataclass
import hashlib
import struct

SIZE=131072
SECTOR=4096
SIGNATURE=0x08012025

def r16(b,i): return struct.unpack_from("<H",b,i)[0]
def r32(b,i): return struct.unpack_from("<I",b,i)[0]
def w16(b,i,v): struct.pack_into("<H",b,i,v)
def w32(b,i,v): struct.pack_into("<I",b,i,v)

def checksum(b):
    if len(b)!=0xff4: raise ValueError("Bad sector checksum input length")
    acc=0
    for i in range(0,len(b),4):
        acc=(acc+r32(b,i))&0xffffffff
    return ((acc>>16)+(acc&0xffff))&0xffff

@dataclass(frozen=True)
class Bank:
    number:int
    sectors:dict
    counter:int
    key:int

def inspect(raw):
    if len(raw)!=SIZE: raise ValueError("Not an unwrapped 128KiB copied save")
    valid=[]
    for number in range(2):
        sectors={}
        counters=[]
        ok=True
        for physical in range(number*14,(number+1)*14):
            off=physical*SECTOR
            block=raw[off:off+SECTOR]
            sid=r16(block,0xff4)
            if sid in sectors:ok=False
            sectors[sid]=off
            counters.append(r32(block,0xffc))
            if r32(block,0xff8)!=SIGNATURE or r16(block,0xff6)!=checksum(block[:0xff4]):
                ok=False
        if ok and set(sectors)==set(range(14)) and len(set(counters))==1:
            sec1=sectors[1]
            count=r32(raw,sec1+0x234)
            if count>6:continue
            for i in range(count):
                mon=raw[sec1+0x238+104*i:sec1+0x238+104*(i+1)]
                if len(mon)!=104:ok=False;break
                key=r32(mon,0)^r32(mon,4)
                plain=bytearray(48)
                for j in range(0,48,4):w32(plain,j,r32(mon,32+j)^key)
                total=sum(r16(plain,k) for k in range(0,48,2))&0xffff
                if total!=r16(mon,28):ok=False;break
            if ok:valid.append(Bank(number,sectors,counters[0],r32(raw,sectors[0]+0x4c)))
    if not valid:raise ValueError("No valid bank")
    if len(valid)==2 and valid[0].counter==valid[1].counter:
        raise ValueError("Ambiguous save counters")
    return valid,max(valid,key=lambda x:x.counter)

def money(raw,bank):
    return r32(raw,bank.sectors[1]+0x4a8)^bank.key

def existing_item(raw,bank,item_id):
    base=bank.sectors[1]+0x578
    matching=[]
    for i in range(450):
        off=base+4*i
        if r16(raw,off)==item_id:
            matching.append((off,r16(raw,off+2)^(bank.key&0xffff)))
    if len(matching)!=1:raise ValueError("Item absent or duplicate: refuse")
    return matching[0]

@dataclass(frozen=True)
class Proof:
    operation:str
    old:int
    new:int
    sha_before:str
    sha_after:str
    active_bank:int
    counter:int
    changed_offsets:tuple

def draft_copy(raw,*,expected_sha256,operation,value,item_id=None):
    if hashlib.sha256(raw).hexdigest()!=expected_sha256:
        raise ValueError("Unexpected source hash: stale or wrong copy")
    valid,bank=inspect(raw)
    section=bank.sectors[1]
    if operation=="money":
        if not 0<=value<=999999:raise ValueError("Invalid money")
        off=section+0x4a8
        old=money(raw,bank)
        encoded=value^bank.key
        width=4
    elif operation=="existing_quantity":
        if not 1<=value<=999 or item_id is None or not 1<=item_id<=65535:
            raise ValueError("Invalid item request")
        itemoff,old=existing_item(raw,bank,item_id)
        if not 1<=old<=999:raise ValueError("Unsupported source quantity")
        off=itemoff+2
        encoded=value^(bank.key&0xffff)
        width=2
    else:raise ValueError("Unknown operation")
    if value==old:raise ValueError("Refuse no-change request")
    if off//SECTOR!=section//SECTOR:raise ValueError("Section boundary error")
    result=bytearray(raw)
    if width==4:w32(result,off,encoded)
    else:w16(result,off,encoded)
    ck=section+0xff6
    w16(result,ck,checksum(result[section:section+0xff4]))
    output=bytes(result)
    permitted=set(range(off,off+width))|{ck,ck+1}
    changed=tuple(i for i,(a,b) in enumerate(zip(raw,output)) if a!=b)
    if not changed or not set(changed).issubset(permitted) or \
       not any(off<=n<off+width for n in changed):
        raise AssertionError("Unexpected write footprint")
    after_banks,after=inspect(output)
    if len(after_banks)!=len(valid) or after.number!=bank.number or \
       after.counter!=bank.counter or after.key!=bank.key:
        raise AssertionError("Save bank invariants changed")
    if operation=="money":
        if money(output,after)!=value:raise AssertionError("Money mismatch")
    elif existing_item(output,after,item_id)[1]!=value:
        raise AssertionError("Quantity mismatch")
    for before_bank in valid:
        if before_bank.number!=bank.number:
            lo=before_bank.number*14*SECTOR
            hi=lo+14*SECTOR
            if output[lo:hi]!=raw[lo:hi]:
                raise AssertionError("Historical bank changed")
    return output,Proof(operation,old,value,expected_sha256,
        hashlib.sha256(output).hexdigest(),bank.number,bank.counter,changed)

def rollback_bytes(edited,proof,*,item_id=None):
    restored,_=draft_copy(edited,expected_sha256=proof.sha_after,
        operation=proof.operation,value=proof.old,item_id=item_id)
    if hashlib.sha256(restored).hexdigest()!=proof.sha_before:
        raise AssertionError("Rollback not exact original")
    return restored
