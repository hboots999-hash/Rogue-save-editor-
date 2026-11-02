#!/usr/bin/env python3
"""Read-only, fail-closed SHA-256 review of Phase20 raw SD exports.

Reads independent PC copies; NEVER writes, repairs, moves, restores or injects.
Hash agreement is necessary evidence, NOT proof of Pokémon field semantics,
GBA VC authentication, installed-ROM identity or durable SD writes.
"""
import argparse
import hashlib
import os
from pathlib import Path
import re
import stat
import sys

RAW = 131072
CONTAINER = 263168
JOURNAL = 526400


class ReviewError(Exception):
    pass


def read_hash(path: Path, expected_size: int) -> str:
    """Use read-only/no-follow FD, reject partial, altered or nonregular files."""
    if path.is_symlink():
        raise ReviewError(f"Symlink not accepted: {path}")
    flags = os.O_RDONLY | getattr(os, "O_CLOEXEC", 0) | getattr(os, "O_NOFOLLOW", 0)
    try:
        fd = os.open(path, flags)
        try:
            first = os.fstat(fd)
            if not stat.S_ISREG(first.st_mode) or first.st_size != expected_size:
                raise ReviewError(f"Wrong size/nonregular file: {path}")
            digest = hashlib.sha256()
            read_total = 0
            while True:
                part = os.read(fd, 32768)
                if not part:
                    break
                read_total += len(part)
                if read_total > expected_size:
                    raise ReviewError(f"Overlong/changing file: {path}")
                digest.update(part)
            last = os.fstat(fd)
            if (read_total != expected_size or last.st_size != expected_size or
                    (first.st_dev, first.st_ino, first.st_mtime_ns, first.st_ctime_ns) !=
                    (last.st_dev, last.st_ino, last.st_mtime_ns, last.st_ctime_ns)):
                raise ReviewError(f"Short/changing SD read: {path}")
            return digest.hexdigest()
        finally:
            os.close(fd)
    except OSError as exc:
        raise ReviewError(f"Unavailable/removed/unreadable SD file: {path}: {exc}") from exc


def require_digest(value: str) -> str:
    if not re.fullmatch(r"[0-9a-fA-F]{64}", value):
        raise ReviewError("Expected SHA-256 must contain exactly 64 hexadecimal digits")
    return value.lower()


def verify(args) -> tuple[str, str, str]:
    source = Path(args.import_file)
    backup = Path(args.backup_file)
    draft = Path(args.draft_file)
    if source.name != "00000001.sav" or source.parent.name != "imports":
        raise ReviewError("Source must be the fixed imports/00000001.sav copy")
    b = re.fullmatch(r"phase18_original_(\d{3,})\.sav", backup.name)
    d = re.fullmatch(r"phase18_draft_(\d{3,})\.sav", draft.name)
    if not b or not d or b.group(1) != d.group(1):
        raise ReviewError("Backup and draft names must share the same new NNN")
    if backup.parent.name != "backups" or draft.parent.name != "drafts":
        raise ReviewError("Backup/draft must be separate app-owned SD directories")
    if source.parent.parent != backup.parent.parent or source.parent.parent != draft.parent.parent:
        raise ReviewError("Source/backup/draft must be from one copied SD directory root")
    expected_source = require_digest(args.source_sha256)
    expected_draft = require_digest(args.expected_draft_sha256)
    if expected_source == expected_draft:
        raise ReviewError("No-op export is not accepted as an edited draft")
    inputs = (source, backup, draft)
    identities = []
    for path in inputs:
        if path.is_symlink():
            raise ReviewError("Symlink is not independent SD export evidence")
        try:
            info = path.stat()
        except OSError as exc:
            raise ReviewError(f"Missing/unreadable SD evidence: {path}: {exc}") from exc
        identities.append((info.st_dev, info.st_ino))
    if len(set(identities)) != 3:
        raise ReviewError("Aliased/hardlinked source, backup or draft is not independent output")
    source_hash = read_hash(source, RAW)
    backup_hash = read_hash(backup, RAW)
    draft_hash = read_hash(draft, RAW)
    if source_hash != expected_source:
        raise ReviewError("Source no longer matches protected pre-test SHA-256")
    if backup_hash != source_hash:
        raise ReviewError("Raw offline backup is not byte-identical to original import")
    if draft_hash != expected_draft:
        raise ReviewError("Draft differs from independently expected full-byte candidate")
    # Defend against a source being changed during the independent review.
    if read_hash(source, RAW) != source_hash:
        raise ReviewError("Original source changed during read-only revalidation")
    optional = (args.phase16_container, args.phase16_container_sha256,
                args.phase16_journal, args.phase16_journal_sha256)
    if any(x is not None for x in optional):
        if any(x is None for x in optional):
            raise ReviewError("Both Phase16 immutable files and protected hashes required")
        container = Path(args.phase16_container)
        journal = Path(args.phase16_journal)
        if container.name != "original_container.bin" or journal.name != "rehearsal_only.rec":
            raise ReviewError("Unexpected Phase16 file names")
        if container in inputs or journal in inputs or container == journal:
            raise ReviewError("Phase16 records must remain separate from raw inputs")
        if read_hash(container, CONTAINER) != require_digest(args.phase16_container_sha256):
            raise ReviewError("Protected authenticated Phase16 full-container hash changed")
        if read_hash(journal, JOURNAL) != require_digest(args.phase16_journal_sha256):
            raise ReviewError("Protected Phase16 RAM journal hash changed")
    return source_hash, backup_hash, draft_hash


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("import-file", "backup-file", "draft-file",
                 "source-sha256", "expected-draft-sha256"):
        parser.add_argument("--" + name, required=True)
    for name in ("phase16-container", "phase16-container-sha256",
                 "phase16-journal", "phase16-journal-sha256"):
        parser.add_argument("--" + name)
    args = parser.parse_args(argv)
    try:
        a, b, c = verify(args)
    except (OSError, ReviewError) as exc:
        print(f"BLOCKED — no game-save authority: {exc}", file=sys.stderr)
        return 2
    print(f"PASS copied RAW import SHA-256:  {a}")
    print(f"PASS separate RAW backup SHA-256: {b}")
    print(f"PASS expected RAW draft SHA-256: {c}")
    if args.phase16_container:
        print("PASS Phase16 full container + journal match protected off-SD hashes")
    print("RAW SD BYTES VERIFIED ONLY — NOT A PLAYABLE SAVE OR RESTORE APPROVAL")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
