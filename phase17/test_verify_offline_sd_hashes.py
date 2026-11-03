#!/usr/bin/env python3
"""Negative-only deterministic synthetic fixtures for read-only SHA checker."""
import hashlib
import os
from pathlib import Path
import tempfile
import unittest

from verify_offline_sd_hashes import ReviewError, verify, RAW, CONTAINER, JOURNAL


def digest(data):
    return hashlib.sha256(data).hexdigest()


class Input:
    pass


class RawSdReviewTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="phase17-SYNTHETIC-")
        self.addCleanup(self.tmp.cleanup)
        root = Path(self.tmp.name) / "3ds" / "PKSM-Rogue"
        for folder in ("imports", "backups", "drafts", "phase16"):
            (root / folder).mkdir(parents=True)
        self.paths = [root / "imports/00000001.sav",
                      root / "backups/phase18_original_007.sav",
                      root / "drafts/phase18_draft_007.sav",
                      root / "phase16/original_container.bin",
                      root / "phase16/rehearsal_only.rec"]
        self.source = bytes([0x71])*RAW
        self.draft = self.source[:512] + b"\x31" + self.source[513:]
        self.container = b"\x12" * CONTAINER  # not authentic, synthetic size proof only
        self.journal = b"\x61" * JOURNAL     # not a durable signed journal
        for path, data in zip(self.paths, (self.source, self.source, self.draft,
                                           self.container, self.journal)):
            path.write_bytes(data)
        self.a = Input()
        self.a.import_file = str(self.paths[0])
        self.a.backup_file = str(self.paths[1])
        self.a.draft_file = str(self.paths[2])
        self.a.source_sha256 = digest(self.source)
        self.a.expected_draft_sha256 = digest(self.draft)
        self.a.phase16_container = str(self.paths[3])
        self.a.phase16_journal = str(self.paths[4])
        self.a.phase16_container_sha256 = digest(self.container)
        self.a.phase16_journal_sha256 = digest(self.journal)

    def passes(self):
        before = [p.read_bytes() for p in self.paths]
        out = verify(self.a)
        self.assertEqual(out,(digest(self.source),digest(self.source),digest(self.draft)))
        self.assertEqual(before,[p.read_bytes() for p in self.paths])
        return out

    def blocked(self):
        with self.assertRaises(ReviewError):
            verify(self.a)

    def test_protected_two_archives_and_restart(self):
        self.passes()
        self.passes()  # independent no-state restart equivalent
        self.paths[4].write_bytes(self.journal[:-1])  # torn journal
        self.blocked()
        self.paths[4].write_bytes(self.journal)
        self.paths[3].write_bytes(self.container[:-1])  # truncated full VC
        self.blocked()

    def test_missing_source_or_backup_or_draft(self):
        for path in self.paths[:3]:
            path.unlink()  # synthetic removal / interrupted export / unplug
            self.blocked()
            source_data = (self.source, self.source, self.draft)[self.paths.index(path)]
            path.write_bytes(source_data)
            self.passes()

    def test_partial_write_power_failure_and_oversize(self):
        for path, original in zip(self.paths[:3],
                                  (self.source, self.source, self.draft)):
            for length in (0, 1, 32768, RAW-1, RAW+1):
                path.write_bytes((original + b"x")[:length])
                self.blocked()
            path.write_bytes(original)
        self.passes()

    def test_content_corruption_even_unchecksummed_tail(self):
        for path, original in zip(self.paths[:3],
                                  (self.source, self.source, self.draft)):
            corrupted = bytearray(original)
            corrupted[-1] ^= 1
            path.write_bytes(corrupted)
            self.blocked()
            path.write_bytes(original)
        self.a.source_sha256 = digest(b"stale")
        self.blocked()
        self.a.source_sha256 = digest(self.source)
        self.a.expected_draft_sha256 = digest(b"stale")
        self.blocked()

    def test_phase16_checks_cannot_be_skipped_partially(self):
        self.a.phase16_journal_sha256 = None
        self.blocked()
        self.a.phase16_journal_sha256 = digest(self.journal)
        self.a.phase16_container_sha256 = digest(b"other")
        self.blocked()
        self.a.phase16_container_sha256 = digest(self.container)
        self.passes()

    def test_aliasing_filename_reuse_symlink_and_unverified_draft(self):
        self.a.draft_file = self.a.backup_file
        self.blocked()
        self.a.draft_file = str(self.paths[2])
        self.paths[2].unlink()
        os.link(self.paths[1],self.paths[2])
        self.blocked()
        self.paths[2].unlink()
        self.paths[2].symlink_to(self.paths[0])
        self.blocked()
        self.paths[2].unlink()
        self.paths[2].write_bytes(self.draft)
        self.a.backup_file = str(self.paths[1]).replace("_007","_008")
        self.blocked()
        self.a.backup_file = str(self.paths[1])
        self.a.expected_draft_sha256 = digest(self.source)
        self.blocked()

    def test_no_permissions_or_file_rewrite(self):
        # The checker has no file-output option: on success, the untouched
        # original and both protected Phase16 files remain byte-identical.
        self.passes()
        self.assertEqual(self.paths[3].read_bytes(),self.container)
        self.assertEqual(self.paths[4].read_bytes(),self.journal)


if __name__=="__main__":
    unittest.main()
