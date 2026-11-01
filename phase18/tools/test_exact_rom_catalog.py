#!/usr/bin/env python3
"""Synthetic-only verifier of exact-ROM catalogue extraction safety."""
import importlib.util
from pathlib import Path
root=Path(__file__).resolve().parent
modpath=root/"extract_2_1_2_move_catalog.py"
spec=importlib.util.spec_from_file_location("exact_rom_catalog", modpath)
mod=importlib.util.module_from_spec(spec)
assert spec and spec.loader
spec.loader.exec_module(mod)
pound=bytes.fromhex("cae3e9e2d8ff00000000000000")
tackle=bytes.fromhex("ced5d7dfe0d9ff000000000000")
assert mod.decode_name(pound)=="Pound"
assert mod.decode_name(tackle)=="Tackle"
assert mod.decode_name(bytes(13))==""
assert mod.decode_name(bytes.fromhex("c7aecde8e6dddfd9ff00000000"))=="M-Strike"
assert len(mod.ROM_SHA)==64
assert mod.COUNT==936 and mod.BATTLE_WIDTH==28 and mod.NAME_WIDTH==13
assert len(mod.BATTLE_TABLES)==2
for invalid in (b"",bytes(0x2020),bytes(0x4fff),bytes(0x5000)):
    try:mod.cia_rom(invalid)
    except ValueError:pass
    else:raise AssertionError("Malformed CIA must fail closed")
try:
    list(mod.rows(bytes(mod.ROM_SIZE)))
except ValueError:
    pass
else:
    raise AssertionError("Unpinned synthetic ROM must be rejected")
print("PASS SHA-pinned local extractor: text decode, 936-record geometry, bad CIA and ROM refusal")
