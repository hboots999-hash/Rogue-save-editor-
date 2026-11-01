#!/usr/bin/env python3
"""Convert Phase 7 deterministic nonplayable one-bank fixture to two valid toy banks."""
from pathlib import Path
import sys,struct,hashlib
if len(sys.argv)!=3:
    raise SystemExit("make_toy_two_bank.py original.sav two_bank.sav")
p=Path(sys.argv[1]).read_bytes()
assert len(p)==131072
buf=bytearray(p)
bank_size=14*4096
buf[bank_size:2*bank_size]=buf[:bank_size]
for j in range(14):
    struct.pack_into("<I",buf,bank_size+j*4096+0xffc,1)
Path(sys.argv[2]).write_bytes(buf)
print("Toy two-bank fixture, nonplayable",hashlib.sha256(buf).hexdigest())
