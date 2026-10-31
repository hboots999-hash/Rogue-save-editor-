"""Unit tests for private local GBA identity report. Synthetic fake ROM only."""
from hashlib import sha256
from pathlib import Path
from tempfile import TemporaryDirectory
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from rom_identity import fingerprint

def main():
    with TemporaryDirectory() as tmp:
        p=Path(tmp)/"fake-test.gba"
        raw=bytearray(4*1024*1024)
        raw[0xA0:0xAC]=b"TEST ONLY   "
        raw[0xAC:0xB0]=b"ABCD"
        raw[0xB0:0xB2]=b"01"
        raw[0xB2]=0x96
        raw[0xBD]=(0x100-((sum(raw[0xA0:0xBD])+0x19)&0xff))&0xff
        p.write_bytes(raw)
        result=fingerprint(p)
        assert result["sha256"]==sha256(raw).hexdigest()
        assert result["game_code"]=="ABCD"
        assert result["compatibility_verified"] is False
        raw[0xBD]^=0x01
        p.write_bytes(raw)
        try:
            fingerprint(p)
        except ValueError as e:
            assert "checksum" in str(e)
        else:
            raise AssertionError("Broken header accepted")
        p.write_bytes(b"THIS IS NOT A GBA ROM")
        try:
            fingerprint(p)
        except ValueError as e:
            assert "standard" in str(e)
        else:
            raise AssertionError("Non-ROM file accepted")
    print("PASS: local-only SHA-256 report, checksum, refusal; no ROM content uploaded")

if __name__=="__main__":
    main()
