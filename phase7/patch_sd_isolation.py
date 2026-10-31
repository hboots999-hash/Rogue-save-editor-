#!/usr/bin/env python3
"""Isolate the experimental PKSM Rogue fork's inherited SD paths.

Run only AFTER applying the Phase 7 overlay to a disposable upstream checkout.
This is deliberately a conservative, all-or-nothing patch:
- only upstream source files, no private .sav and no built binaries
- no changes to PKSM's original upstream repository
- all replacement candidate files are validated before any are written
- refuse second application or unmatched layout
"""
from __future__ import annotations
import argparse
from pathlib import Path
import sys

OLD = "/3ds/PKSM"
NEW = "/3ds/PKSM-Rogue"
# Enumerate upstream source files containing literal SD paths. Restrict changes to
# this known set instead of running an indiscriminate find/replace over the repo.
TARGETS = (
 "3ds/source/Bank.cpp",
 "3ds/source/banks.cpp",
 "3ds/source/utils/PkmUtils.cpp",
 "common/include/sound/sound.hpp",
 "3ds/source/io/Archive.cpp",
 "3ds/source/AssetStore.cpp",
 "3ds/source/app.cpp",
 "3ds/source/gui/screen/ScriptScreen.cpp",
 "3ds/source/sound/sound.cpp",
 "3ds/source/utils/logging.cpp",
 "3ds/source/gui/gui.cpp",
 "3ds/source/gui/screen/GpssScreen.cpp",
 "3ds/source/utils/WirelessTransfer.cpp",
 "3ds/source/titles/loader.cpp",
 "3ds/source/gui/screen/StorageScreen.cpp",
 "3ds/source/gui/screen/InjectSelectorScreen.cpp",
)

def apply(root: Path) -> dict[str, int]:
    if not (root/"3ds/source/gui/screen/RogueInspectScreen.cpp").is_file():
        raise RuntimeError("Expected Rogue overlay missing; apply it before SD isolation")
    old_bytes = OLD.encode()
    new_bytes = NEW.encode()
    originals: dict[Path,bytes] = {}
    revisions: dict[Path,bytes] = {}
    counts: dict[str,int] = {}
    for rel in TARGETS:
        path = root / rel
        if not path.is_file() or path.is_symlink():
            raise RuntimeError(f"Expected regular source file missing: {rel}")
        data = path.read_bytes()
        # If an upstream file no longer references these paths, stop instead of
        # silently assuming isolation succeeded on an unknown source revision.
        n = data.count(old_bytes)
        if n == 0 or new_bytes in data:
            raise RuntimeError(f"Unexpected or already patched SD paths in {rel}: {n}")
        originals[path] = data
        revisions[path] = data.replace(old_bytes,new_bytes)
        counts[rel] = n
    # The Rogue overlay itself already uses /3ds/PKSM-Rogue, and remains intact.
    overlay = root/"3ds/source/gui/screen/RogueInspectScreen.cpp"
    if NEW not in overlay.read_text():
        raise RuntimeError("Rogue overlay import/draft path unexpectedly changed")
    written = []
    try:
        for path,data in revisions.items():
            path.write_bytes(data)
            written.append(path)
    except OSError:
        for path in written:
            path.write_bytes(originals[path])
        raise
    return counts

def main() -> int:
    p=argparse.ArgumentParser()
    p.add_argument("checkout", type=Path)
    args=p.parse_args()
    try:
        counts=apply(args.checkout.resolve())
        print(f"Isolated {sum(counts.values())} inherited SD path occurrences across {len(counts)} files")
        for filename, n in counts.items():
            print(f"  {filename}: {n}")
        return 0
    except (OSError,RuntimeError) as e:
        print(f"Isolation BLOCKED: {e}", file=sys.stderr)
        return 2

if __name__ == "__main__":
    raise SystemExit(main())
