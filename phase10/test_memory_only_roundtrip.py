#!/usr/bin/env python3
"""CI regression on deterministic, non-playable Phase 7 fixture. No personal data."""
import hashlib
from pathlib import Path
import sys
from memory_only_roundtrip import (
    draft_copy,rollback_bytes,inspect,money,existing_item
)

def must_fail(action):
    try:action()
    except (ValueError,AssertionError):
        return
    raise AssertionError("Expected safe refusal")

def run(path):
    original=Path(path).read_bytes()
    original_sha=hashlib.sha256(original).hexdigest()
    banks,active=inspect(original)
    assert len(original)==131072
    assert len(banks)==1
    assert active.number==0 and active.counter==0
    assert money(original,active)==40000
    assert existing_item(original,active,113)[1]==10
    for op,value,item in (
        ("money",40001,None),
        ("existing_quantity",11,113),
        ("existing_quantity",3,5),
    ):
        new,proof=draft_copy(original,expected_sha256=original_sha,
                              operation=op,value=value,item_id=item)
        valid,chosen=inspect(new)
        assert len(valid)==1 and chosen.counter==active.counter
        assert 0<len(proof.changed_offsets)<=6
        assert new!=original
        assert rollback_bytes(new,proof,item_id=item)==original
        assert original_sha==hashlib.sha256(original).hexdigest()
    must_fail(lambda:draft_copy(original,expected_sha256="0"*64,
                               operation="money",value=40001))
    must_fail(lambda:draft_copy(original,expected_sha256=original_sha,
                               operation="money",value=1000000))
    must_fail(lambda:draft_copy(original,expected_sha256=original_sha,
                               operation="existing_quantity",value=1000,item_id=113))
    must_fail(lambda:draft_copy(original,expected_sha256=original_sha,
                               operation="existing_quantity",value=1,item_id=65535))
    must_fail(lambda:draft_copy(original,expected_sha256=original_sha,
                               operation="money",value=40000))
    must_fail(lambda:draft_copy(original[:-1],expected_sha256=original_sha,
                               operation="money",value=40001))
    corrupt=bytearray(original);corrupt[4096+0x4a8]^=1
    must_fail(lambda:draft_copy(bytes(corrupt),
        expected_sha256=hashlib.sha256(corrupt).hexdigest(),
        operation="money",value=40001))
    corrupt=bytearray(original);corrupt[4096+0x238+28]^=1
    # Sector checksum has changed, so a structurally damaged bank is rejected.
    must_fail(lambda:draft_copy(bytes(corrupt),
        expected_sha256=hashlib.sha256(corrupt).hexdigest(),
        operation="money",value=40001))
    print("PASS Phase 10: memory-only, SHA-pinned draft, delta mask, exact rollback, stale/bad input refused")

if __name__=="__main__":
    if len(sys.argv)!=2:raise SystemExit("Usage: test_memory_only_roundtrip.py synthetic.sav")
    run(sys.argv[1])
