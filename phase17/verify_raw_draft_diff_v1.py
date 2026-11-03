#!/usr/bin/env python3
"""Independent, READ-ONLY raw H168 byte-diff verifier for offline drafts.

A manifest must be prepared INDEPENDENTLY from an original protected source
and an independently reviewed expected edit. Never derive the expected bytes
or digest by copying the editor's draft. This checker validates a byte
footprint; it does NOT authenticate installed ROM, move identity/PP semantics,
game compatibility, CMAC, a GBA VC wrapper or game-save restoration.

Move replacement currently has RESEARCH-ONLY status, even on a successful
byte-level comparison. This tool never creates files or opens a game archive.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import sys

SIZE = 131072
MANIFEST_MAX = 65536
SCHEMA = 1
KINDS = {"scalar-batch": 4096, "move-replacement-research": 8}


class Reject(Exception):
    pass


def exact_pairs(pairs):
    out = {}
    for key, value in pairs:
        if key in out:
            raise Reject("Repeated JSON key in manifest")
        out[key] = value
    return out


def checked_file(path, size):
    path = Path(path)
    if path.is_symlink():
        raise Reject("Symlink input is refused")
    fd = None
    try:
        fd = os.open(path, os.O_RDONLY |
                     getattr(os, "O_NOFOLLOW", 0) |
                     getattr(os, "O_CLOEXEC", 0))
        first = os.fstat(fd)
        if not stat.S_ISREG(first.st_mode) or first.st_size != size:
            raise Reject("File is absent, nonregular, short or overlong")
        chunks = []
        total = 0
        while True:
            chunk = os.read(fd, 32768)
            if not chunk:
                break
            total += len(chunk)
            if total > size:
                raise Reject("File grew during read")
            chunks.append(chunk)
        last = os.fstat(fd)
        if total != size or (first.st_dev, first.st_ino, first.st_size,
                             first.st_mtime_ns, first.st_ctime_ns) != (
                             last.st_dev, last.st_ino, last.st_size,
                             last.st_mtime_ns, last.st_ctime_ns):
            raise Reject("File shortened or changed during read")
        return b"".join(chunks), (first.st_dev, first.st_ino)
    except OSError as exc:
        raise Reject(f"Input unavailable or SD read failed: {exc}") from exc
    finally:
        if fd is not None:
            os.close(fd)


def hex_digest(value):
    if not isinstance(value, str) or not re.fullmatch(r"[a-fA-F0-9]{64}", value):
        raise Reject("Invalid expected SHA-256")
    return value.lower()


def parse_manifest(path):
    manifest_bytes, _ = checked_file_manifest(path)
    try:
        m = json.loads(manifest_bytes.decode("utf-8"), object_pairs_hook=exact_pairs,
                       parse_constant=lambda x: (_ for _ in ()).throw(Reject("Invalid JSON constant")))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise Reject("Malformed manifest") from exc
    if not isinstance(m, dict) or set(m) != {
        "schema", "kind", "source_sha256", "draft_sha256", "changes"
    }:
        raise Reject("Manifest schema or unknown fields")
    if type(m["schema"]) is not int or m["schema"] != SCHEMA or m["kind"] not in KINDS:
        raise Reject("Unsupported schema or editing kind")
    source_sha = hex_digest(m["source_sha256"])
    draft_sha = hex_digest(m["draft_sha256"])
    if source_sha == draft_sha:
        raise Reject("No-op or same source/draft fingerprint")
    changes = m["changes"]
    limit = KINDS[m["kind"]]
    if not isinstance(changes, list) or not 1 <= len(changes) <= limit:
        raise Reject("Missing/too many expected byte differences")
    previous = -1
    for row in changes:
        if not isinstance(row, dict) or set(row) != {"offset", "before", "after"}:
            raise Reject("Unknown change fields")
        if any(type(row[key]) is not int for key in ("offset", "before", "after")):
            raise Reject("Invalid byte or offset type")
        off = row["offset"]
        if not previous < off < SIZE:
            raise Reject("Changes must be unique and strictly sorted within raw save")
        if not 0 <= row["before"] <= 255 or not 0 <= row["after"] <= 255:
            raise Reject("Byte outside 0..255")
        if row["before"] == row["after"]:
            raise Reject("Manifest includes a non-change")
        previous = off
    return m, source_sha, draft_sha


def checked_file_manifest(path):
    path = Path(path)
    if path.is_symlink():
        raise Reject("Manifest symlink refused")
    try:
        size = path.stat().st_size
    except OSError as exc:
        raise Reject("Missing manifest") from exc
    if not 1 <= size <= MANIFEST_MAX:
        raise Reject("Manifest too large/empty")
    return checked_file(path, size)


def verify(source_path, draft_path, manifest_path):
    m, source_sha, draft_sha = parse_manifest(manifest_path)
    original, original_id = checked_file(source_path, SIZE)
    draft, draft_id = checked_file(draft_path, SIZE)
    if original_id == draft_id:
        raise Reject("Source and draft alias the same file")
    if hashlib.sha256(original).hexdigest() != source_sha:
        raise Reject("Stale or corrupted protected source fingerprint")
    if hashlib.sha256(draft).hexdigest() != draft_sha:
        raise Reject("Corrupt/stale edited draft fingerprint")
    observed = [i for i, (a, b) in enumerate(zip(original, draft)) if a != b]
    expected = [row["offset"] for row in m["changes"]]
    if observed != expected:
        raise Reject("Changed-byte footprint differs from independently approved manifest")
    for row in m["changes"]:
        i = row["offset"]
        if original[i] != row["before"] or draft[i] != row["after"]:
            raise Reject("Expected before/after byte mismatch")
    # Re-read both files to block obvious mid-review substitutions.
    second_original, second_original_id = checked_file(source_path, SIZE)
    second_draft, second_draft_id = checked_file(draft_path, SIZE)
    if (second_original_id != original_id or second_draft_id != draft_id or
            second_original != original or second_draft != draft):
        raise Reject("Source or draft changed during verification")
    return {"kind": m["kind"], "changed_bytes": len(observed),
            "source_sha256": source_sha, "draft_sha256": draft_sha,
            "status": "RAW_BYTES_ONLY_MOVE_RESEARCH_BLOCKED"
            if m["kind"] == "move-replacement-research" else
            "RAW_BYTES_ONLY_SCALAR_OFFLINE"}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True)
    parser.add_argument("--draft", required=True)
    parser.add_argument("--independent-manifest", required=True)
    args = parser.parse_args(argv)
    try:
        result = verify(args.source, args.draft, args.independent_manifest)
    except (OSError, Reject, TypeError, ValueError) as exc:
        print(f"BLOCKED — NO GAME RESTORE: {exc}", file=sys.stderr)
        return 2
    print(f"PASS independent RAW byte footprint: {result['changed_bytes']} bytes")
    print(f"Original SHA-256: {result['source_sha256']}")
    print(f"Draft SHA-256:    {result['draft_sha256']}")
    print(result["status"])
    print("NO CMAC / NO ROM PROOF / NO GAMEPLAY OR RESTORE APPROVAL")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
