#!/usr/bin/env python3
"""Isolate common PKSM SD paths in a disposable upstream checkout.

This is a mechanical, narrowly-scoped transformation. It does not prove
full application isolation or authorize console installation.

Run BEFORE apply_to_pksm.py. All upstream source paths containing the
literal /3ds/PKSM must be reviewed; only exact directory-prefix matches
in core PKSM source/header files are rewritten. Third-party, binary, ROM,
asset and script trees are not rewritten.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import re
import sys

OLD = "/3ds/PKSM"
NEW = "/3ds/PKSM-Rogue"
ALLOWED_ROOTS = (
    "3ds/source", "3ds/include",
    "common/source", "common/include",
    "core/source", "core/include",
)
SOURCE_SUFFIXES = frozenset((".cpp", ".c", ".cc", ".hpp", ".h", ".tcc"))
# Must be a directory or path stem, never a prefix of a different app.
OLD_PATH_RE = re.compile(r"/3ds/PKSM(?=[/\"'\\]|\s|$)")
MIN_EXPECTED_OCCURRENCES = 20

def isolate_tree(root: Path) -> dict[str, int]:
    root = root.resolve()
    required = [root / "3ds/Makefile", root / "3ds/source/io/Archive.cpp"]
    for path in required:
        if not path.is_file():
            raise ValueError("Missing PKSM source tree entry: " + str(path))
    if (root / "3ds/source/gui/screen/RogueInspectScreen.cpp").exists():
        raise ValueError("Run isolation BEFORE the overlay; refusing to rewrite the Rogue import path")
    edits = {}
    old_count = 0
    for sub in ALLOWED_ROOTS:
        base = root / sub
        if not base.is_dir():
            raise ValueError("Missing expected source directory: " + str(base))
        for path in sorted(base.rglob("*")):
            if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
                continue
            original = path.read_text(encoding="utf-8")
            updated, n = OLD_PATH_RE.subn(NEW, original)
            if n:
                if OLD in updated.replace(NEW, ""):
                    raise ValueError("Unclassified PKSM path remains: " + str(path))
                edits[path] = updated
                old_count += n
    if old_count < MIN_EXPECTED_OCCURRENCES:
        raise ValueError(
            f"Found only {old_count} known PKSM paths, expected at least "
            f"{MIN_EXPECTED_OCCURRENCES}; refusing a potentially mismatched upstream version"
        )
    if old_count > 300:
        raise ValueError("Unexpectedly large PKSM path rewrite; review upstream")
    # Validate all candidate text before changing a single source file.
    for path, updated in edits.items():
        if NEW + "-Rogue" in updated:
            raise ValueError("Double-isolated path detected: " + str(path))
    for path, updated in edits.items():
        path.write_text(updated, encoding="utf-8")
    return {"files": len(edits), "paths": old_count}

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("pksm_checkout", type=Path)
    args = parser.parse_args()
    try:
        stats = isolate_tree(args.pksm_checkout)
    except (OSError, ValueError, UnicodeError) as exc:
        print(f"Isolation REFUSED: {exc}", file=sys.stderr)
        return 1
    print(f"Rewrote {stats['paths']} absolute SD path occurrences in "
          f"{stats['files']} PKSM source files. Isolation remains UNVERIFIED.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
