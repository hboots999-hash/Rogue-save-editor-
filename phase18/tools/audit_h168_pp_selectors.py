#!/usr/bin/env python3
"""Read-only Thumb BL and PP-table literal inventory for exact H168 GBA ROM.

Produces *metadata only*, never a ROM/save copy. A static match is not proof
of runtime reachability or which gameplay mode executes a caller. Deliberately
does not authorize replacement, PP-Ups, offline draft Apply or save writing.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

ROM_SHA = "111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3"
ROM_SIZE = 32 * 1024 * 1024
TABLE_A = 0x08BD27D0
TABLE_B = 0x08BD8E30
SELECTOR = 0x081D11B8
# Table A begins at ROM offset 0x00BD27D0; scan only earlier code/data
# space to avoid treating table data's arbitrary words as code references.
SCAN_END = 0x00BD27D0
# Verified exact-build PP-Up lane tables and compiled helper entrypoints.
PP_MASKS_AT = 0x010A9984
PP_MASKS = bytes((3,12,48,192,252,243,207,63,1,4,16,64))
CALC_MAX_PP = 0x00191634
CLEAR_MON_PP = 0x00191684
CLEAR_BATTLE_PP = 0x001916B8


def u16(data: bytes, at: int) -> int:
    return struct.unpack_from("<H", data, at)[0]

def thumb_bl_target(data: bytes, offset: int) -> int | None:
    """Decode a 32-bit Thumb-1 BL pair. Returned address is GBA ROM offset."""
    if offset < 0 or offset + 4 > len(data) or offset % 2:
        return None
    first, second = struct.unpack_from("<HH", data, offset)
    if first & 0xF800 != 0xF000 or second & 0xF800 != 0xF800:
        return None
    displacement = ((first & 0x7FF) << 12) | ((second & 0x7FF) << 1)
    if displacement & (1 << 22):
        displacement -= 1 << 23
    return offset + 4 + displacement

def direct_bl_sites(rom: bytes, target: int) -> list[int]:
    return [off for off in range(0, SCAN_END - 3, 2)
            if thumb_bl_target(rom, off) == target]

def pointer_sites(rom: bytes, pointer: int) -> list[int]:
    marker = struct.pack("<I", pointer)
    return [off for off in range(0, SCAN_END - 3, 4)
            if rom[off:off+4] == marker]

def structural_report(rom: bytes) -> dict:
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError("Unmatched ROM; refusing guessed PP-table mode")
    if rom[SELECTOR - 0x08000000: SELECTOR - 0x08000000+4] != bytes((0x00,0x20,0x70,0x47)):
        raise ValueError("Selector is not the verified movs r0,#0; bx lr")
    if rom[PP_MASKS_AT:PP_MASKS_AT+12]!=PP_MASKS or rom.rfind(PP_MASKS)!=PP_MASKS_AT:
        raise ValueError("Exact 2-bit PP-Up mask geometry is not uniquely located")
    if struct.unpack_from("<I",rom,0x0019167C)[0]!=0x090A9984:
        raise ValueError("Compiled CalculatePPWithBonus mask literal changed")
    if struct.unpack_from("<I",rom,0x001916B4)[0]!=0x090A9988:
        raise ValueError("Compiled RemoveMonPPBonus clear-mask literal changed")
    aa = pointer_sites(rom, TABLE_A)
    bb = pointer_sites(rom, TABLE_B)
    selectors = direct_bl_sites(rom, SELECTOR - 0x08000000)
    paired = [(a, min((b for b in bb if abs(b-a) <= 16),
                      key=lambda b: abs(a-b)))
              for a in aa if any(abs(b-a) <= 16 for b in bb)]
    zero_tests = [off for off in selectors
                  if u16(rom,off+4)==0x2800 and
                  u16(rom,off+6)&0xF000==0xD000 and
                  u16(rom,off+6)&0x0F00!=0x0F00]
    bonus_calls = {
        "calculate_max_pp":len(direct_bl_sites(rom,CALC_MAX_PP)),
        "clear_party_mon_pp_bonus":len(direct_bl_sites(rom,CLEAR_MON_PP)),
        "clear_battle_mon_pp_bonus":len(direct_bl_sites(rom,CLEAR_BATTLE_PP))
    }
    # A nearby BL is *not* sufficient to connect a literal to that BL.
    return {
      "rom_sha256": ROM_SHA,
      "static_only": True,
      "production_apply_approved": False,
      "direct_literals": {"table_a":len(aa),"table_b":len(bb),
                          "neighboring_pairs_at_most_16_bytes":len(paired)},
      "pp_up_compiled_evidence":{
          "mask_rom_offset":"0x010a9984",
          "get_slot_masks":[3,12,48,192],
          "clear_slot_masks":[252,243,207,63],
          "add_slot_masks":[1,4,16,64],
          "max_pp_helper_entry":"0x08191634",
          "clear_party_mon_helper_entry":"0x08191684",
          "clear_battle_mon_helper_entry":"0x081916b8",
          "raw_bl_patterns_targeting_helpers":bonus_calls,
          "max_pp_path_selects_table_a":True,
          "clearing_helper_exists_but_replacement_call_unproven":True
      },
      "selector_thumb_entry":f"0x{SELECTOR:08x}",
      "selector_instructions":"movs r0,#0; bx lr",
      "raw_thumb_bl_patterns_targeting_selector":len(selectors),
      "selector_calls_followed_by_cmp_r0_zero_conditional":len(zero_tests),
      "example_evidence": {
          "callsite_offsets":[f"0x{x:06x}" for x in selectors[:16]],
          "literal_pairs":[[f"0x{x:06x}",f"0x{y:06x}"] for x,y in paired[:16]]
      },
      "limitations":[
          "BL byte patterns may include unreachable data; no control-flow proof",
          "Pointers and selector calls are not individually connected by scanning",
          "Branch direction and table chosen must be proved per reachable mode",
          "Existence of clear-mon-PP helper does not prove learn/forget invokes it",
          "No observed replay/re-save of edited save in target emulator",
          "Unknown cached state and special-move/learnset constraints",
          "All 40 PP-conflicting IDs disabled; no automatic teach allowlist"
      ]
    }

def main() -> None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("rom",type=Path,help="Local 32MiB exact ROM, never commit it")
    p.add_argument("--json",type=Path,help="Optional metadata-only output")
    a=p.parse_args()
    report=structural_report(a.rom.read_bytes())
    rendered=json.dumps(report,indent=2)+"\n"
    if a.json:
        a.json.write_text(rendered,encoding="utf-8")
    else:
        print(rendered,end="")
if __name__=="__main__":
    main()
