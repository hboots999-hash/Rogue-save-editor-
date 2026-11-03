#!/usr/bin/env python3
"""Disposable synthetic fixtures: never real game data, never filesystem writes from verifier."""
import hashlib
import json
import os
from pathlib import Path
import tempfile
import unittest

from verify_raw_draft_diff_v1 import SIZE, Reject, verify, parse_manifest


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


class DraftDiffTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="synthetic_phase17_draft_")
        self.addCleanup(self.tmp.cleanup)
        root = Path(self.tmp.name)
        self.source = root / "copy.sav"
        self.draft = root / "draft.sav"
        self.manifest = root / "independently_prepared_expected.json"
        self.old = bytes([0x41]) * SIZE
        data = bytearray(self.old)
        data[37] = 0x42
        data[65535] = 0x44
        self.new = bytes(data)
        self.source.write_bytes(self.old)
        self.draft.write_bytes(self.new)
        self.m = {"schema":1,"kind":"move-replacement-research",
                  "source_sha256":sha(self.old),"draft_sha256":sha(self.new),
                  "changes":[{"offset":37,"before":65,"after":66},
                             {"offset":65535,"before":65,"after":68}]}
        self.save()

    def save(self):
        self.manifest.write_text(json.dumps(self.m),encoding="utf-8")

    def good(self):
        result = verify(self.source,self.draft,self.manifest)
        self.assertEqual(result["status"],"RAW_BYTES_ONLY_MOVE_RESEARCH_BLOCKED")
        self.assertEqual(result["changed_bytes"],2)
        return result

    def blocked(self):
        with self.assertRaises((Reject, ValueError, TypeError)):
            verify(self.source,self.draft,self.manifest)

    def test_positive_is_never_gameplay_approval_and_restart(self):
        before = [p.read_bytes() for p in [self.source,self.draft,self.manifest]]
        self.good()
        self.good()  # independent repeat/restart has no stateful approval
        self.assertEqual(before,[p.read_bytes() for p in [self.source,self.draft,self.manifest]])
        self.assertNotIn("GAMEPLAY",self.good()["status"])

    def test_scalar_batch_byte_only(self):
        self.m["kind"]="scalar-batch"
        self.save()
        out=verify(self.source,self.draft,self.manifest)
        self.assertEqual(out["status"],"RAW_BYTES_ONLY_SCALAR_OFFLINE")

    def test_missing_sd_file_and_power_failure(self):
        for item in (self.source,self.draft,self.manifest):
            data=item.read_bytes()
            item.unlink()
            self.blocked()
            item.write_bytes(data)
            self.good()

    def test_truncated_or_overlong_source_and_draft(self):
        for item, data in ((self.source,self.old),(self.draft,self.new)):
            for n in (0,1,4096,SIZE-1,SIZE+1):
                item.write_bytes((data+b"\x11")[:n])
                self.blocked()
            item.write_bytes(data)
            self.good()

    def test_stale_and_corrupt_digests(self):
        for key in ("source_sha256","draft_sha256"):
            old=self.m[key]
            self.m[key]=sha(b"wrong")
            self.save()
            self.blocked()
            self.m[key]=old
        self.save()
        self.good()

    def test_changed_unchecksummed_trailer_or_other_byte(self):
        drift=bytearray(self.new)
        drift[-1]^=1
        self.draft.write_bytes(drift)
        self.blocked()
        self.draft.write_bytes(self.new)
        self.good()

    def test_whole_hash_cannot_replace_expected_byte_footprint(self):
        other=bytearray(self.new)
        other[88]=0x46
        self.draft.write_bytes(other)
        self.m["draft_sha256"]=sha(other)
        self.save()
        self.blocked()  # whole hash matches but unapproved offset differs

    def test_wrong_expected_bytes_and_duplicate_offsets(self):
        self.m["changes"][0]["after"]=0x47
        self.save()
        self.blocked()
        self.m["changes"][0]["after"]=0x42
        self.m["changes"].append(dict(self.m["changes"][0]))
        self.save()
        self.blocked()

    def test_move_research_eight_byte_limit(self):
        # Nine distinct synthetic bytes still fail even when hashes match.
        raw=bytearray(self.old)
        self.m["changes"]=[]
        for offset in range(100,109):
            raw[offset]=0x23
            self.m["changes"].append({"offset":offset,"before":65,"after":35})
        self.draft.write_bytes(raw)
        self.m["draft_sha256"]=sha(raw)
        self.save()
        self.blocked()

    def test_noop_wrong_schema_extra_fields_and_bad_types(self):
        self.m["schema"]=2
        self.save()
        self.blocked()
        self.m["schema"]=1
        self.m["unexpected"]="ignore?"
        self.save()
        self.blocked()
        self.m.pop("unexpected")
        self.m["changes"][0]["offset"]=True
        self.save()
        self.blocked()

    def test_malformed_json_duplicate_keys_and_manifest_symlink(self):
        self.manifest.write_text('{"schema":1,"schema":1}')
        self.blocked()
        self.save()
        self.manifest.unlink()
        self.manifest.symlink_to(self.source)
        self.blocked()

    def test_hardlink_aliases_do_not_certify_draft(self):
        self.draft.unlink()
        os.link(self.source,self.draft)
        self.blocked()

    def test_both_raw_files_from_same_original_do_not_pass(self):
        self.draft.write_bytes(self.old)
        self.blocked()

    def test_byte_only_cannot_validate_move_id_or_pp_semantics(self):
        # Deliberately fabricated bytes/data: the verifier checks only
        # protected exact-byte expectations, not Pokémon learnability,
        # PP-Up reset, decrypted substruct semantics, or ROM identity.
        self.assertEqual(self.good()["status"],"RAW_BYTES_ONLY_MOVE_RESEARCH_BLOCKED")


if __name__=="__main__":
    unittest.main()
