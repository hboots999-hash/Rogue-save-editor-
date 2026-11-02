#!/usr/bin/env python3
"""Synthetic-only checks of SHA-pinned Thumb BL/PP selector audit."""
import importlib.util
from pathlib import Path
path=Path(__file__).with_name("audit_h168_pp_selectors.py")
spec=importlib.util.spec_from_file_location("h168_pp_audit",path)
assert spec and spec.loader
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

# Known exact-ROM Thumb-1 BL instruction bytes, embedded here as four
# machine-instruction bytes only (not a ROM slice or code image).
code=bytearray(0x159ae)
code[0x159aa:0x159ae]=bytes((0xbb,0xf1,0x05,0xfc))
assert mod.thumb_bl_target(code,0x159aa)==0x1d11b8
assert mod.thumb_bl_target(code,0x159ab) is None
assert mod.thumb_bl_target(code,0x159ac) is None
assert mod.thumb_bl_target(code,len(code)-2) is None
assert mod.thumb_bl_target(bytes((0,0,0,0)),0) is None
assert mod.TABLE_A==0x08bd27d0 and mod.TABLE_B==0x08bd8e30
assert mod.SELECTOR==0x081d11b8
assert mod.ROM_SHA.startswith("111b0008") and len(mod.ROM_SHA)==64
assert mod.SCAN_END==0x00bd27d0

for bogus in (b"",bytes(128),bytes(mod.ROM_SIZE)):
    try:
        mod.structural_report(bogus)
    except ValueError:
        pass
    else:
        raise AssertionError("Unpinned ROM must never be admitted")
print("PASS SHA-gated Thumb BL audit: exact branch bytes, invalid opcodes, stale ROM reject")
