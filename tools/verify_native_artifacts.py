#!/usr/bin/env python3
"""Fail-closed structural check of compiled PKSM Rogue 3DS artifacts.

Does NOT establish emulator or console launch safety or save compatibility.
"""
from pathlib import Path
import hashlib
import json
import struct
import sys

EXPECTED_TITLE = 0x000400000F73A200

def align64(x):
    return (x + 63) & ~63

def check(root):
    root = Path(root)
    checks = {}
    errors = []
    for name in ("PKSM_Rogue_Inspect.3dsx", "PKSM_Rogue_Inspect.cia", "PKSM_Rogue_Inspect.elf"):
        path = root / name
        if not path.is_file():
            errors.append("Missing " + name)
            continue
        raw = path.read_bytes()
        checks[name] = {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest()}
        if len(raw) < 4096:
            errors.append("Suspiciously small " + name)
        if name.endswith(".3dsx") and raw[:4] != b"3DSX":
            errors.append("Bad 3DSX header")
        if name.endswith(".elf") and not (
            raw[:4] == b"\x7fELF" and raw[4:6] == b"\x01\x01"
            and struct.unpack_from("<H", raw, 18)[0] == 40
        ):
            errors.append("Not a 32-bit little-endian ARM ELF")
        if name.endswith(".cia") and len(raw) >= 0x2020:
            header_size, typ, ver, cert_len, ticket_len, tmd_len, meta_len, content_len = \
                struct.unpack_from("<IHHIIIIQ", raw)
            cert = align64(header_size)
            ticket = align64(cert + cert_len)
            tmd = align64(ticket + ticket_len)
            content = align64(tmd + tmd_len)
            meta = align64(content + content_len)
            if (header_size, typ, ver) != (0x2020, 0, 0):
                errors.append("Unexpected CIA header")
            if min(cert_len, ticket_len, tmd_len, meta_len, content_len) <= 0:
                errors.append("CIA has empty section")
            if meta + meta_len != len(raw):
                errors.append("CIA section offsets or total size invalid")
            if tmd_len < 0x194 or content_len < 0x200 or content + 0x200 > len(raw):
                errors.append("CIA TMD/NCCH truncated")
            else:
                tmd_title = int.from_bytes(raw[tmd + 0x18C:tmd + 0x194], "big")
                magic = raw[content + 0x100:content + 0x104]
                program = struct.unpack_from("<Q", raw, content + 0x118)[0]
                partition = struct.unpack_from("<Q", raw, content + 0x108)[0]
                checks[name]["title_ids"] = [f"{x:016X}" for x in (tmd_title, program, partition)]
                if magic != b"NCCH":
                    errors.append("Missing NCCH at CIA content offset")
                if any(x != EXPECTED_TITLE for x in (tmd_title, program, partition)):
                    errors.append("CIA title-ID mismatch, potential collision with another installed title")
    return {"status": "PASS_STATIC_ONLY" if not errors else "FAIL",
            "expected_title": f"{EXPECTED_TITLE:016X}",
            "artifacts": checks, "errors": errors,
            "warning": "Does not validate emulator boot, real hardware, the Rogue ROM, or saves."}

if __name__ == "__main__":
    result = check(sys.argv[1] if len(sys.argv) > 1 else "build-outputs")
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if not result["errors"] else 1)
