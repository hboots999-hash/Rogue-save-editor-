#!/usr/bin/env python3
"""Lock an experimentally patched PKSM build to Rogue SD-copy inspection only.

Run AFTER tools/apply_to_pksm.py. The regular PKSM title/bank/save
scanning and update installation routes are retained in source but
excluded from this build's startup execution. Only the separate
RogueInspectScreen is pushed. This does not make all inherited
initialization/storage operations read-only.
"""
from pathlib import Path
import argparse
import sys

def once(data: str, old: str, new: str, name: str) -> str:
    if data.count(old) != 1:
        raise ValueError(f"{name}: expected one anchor, got {data.count(old)}")
    return data.replace(old, new, 1)

def restrict(root: Path) -> None:
    app = root / "3ds/source/app.cpp"
    archive = root / "3ds/source/io/Archive.cpp"
    inspector = root / "3ds/source/gui/screen/RogueInspectScreen.cpp"
    if not app.is_file() or not inspector.is_file() or not archive.is_file():
        raise ValueError("Missing upstream app.cpp or Rogue inspector overlay; apply overlay first")
    old = app.read_text(encoding="utf-8")
    if 'rogueInspectOnly' in old:
        raise ValueError("Startup already restricted")
    edited = once(old, '#include "app.hpp"',
        '#include "app.hpp"\n#include "RogueInspectScreen.hpp"', "screen include")
    edited = once(edited,
        'Result App::init(const std::string& execPath)\n{',
        'Result App::init(const std::string& execPath)\n'
        '{\n    constexpr bool rogueInspectOnly = true;\n',
        "startup flag")
    edited = once(edited,
        'if (assetStore.verdict() != AssetStore::Verdict::Ok)\n    {',
        'if (assetStore.verdict() != AssetStore::Verdict::Ok)\n'
        '    {\n'
        '        if (rogueInspectOnly)\n'
        '        {\n'
        '            return consoleDisplayError("Rogue Inspect assets unavailable; automatic update disabled.", -1);\n'
        '        }',
        "missing asset recovery")
    edited = once(edited,
        'if (Configuration::getInstance().autoUpdate() && update(execPath))',
        'if (!rogueInspectOnly && Configuration::getInstance().autoUpdate() && update(execPath))',
        "automatic update")
    edited = once(edited,
        "Archive::init(execPath, confirmExtdataReset);",
        "Archive::init(execPath, nullptr);",
        "disable destructive extdata reset prompt")
    edited = once(edited,
        '    const bool contentUp =\n',
        '    const bool contentUp = rogueInspectOnly || (\n',
        "save-scanning startup block")
    edited = once(edited,
        '            stopCartScan);\n',
        '            stopCartScan));\n',
        "save-scanning block termination")
    edited = once(edited,
        'ScreenStack::push(std::make_unique<TitleLoadScreen>());',
        'ScreenStack::push(std::make_unique<RogueInspectScreen>());',
        "initial UI route")
    # Explicit source-level safety properties, checked before any change.
    if 'if (!rogueInspectOnly && Configuration::getInstance().autoUpdate()' not in edited:
        raise ValueError("Auto-update gating missing")
    if 'const bool contentUp = rogueInspectOnly || (' not in edited:
        raise ValueError("Title/bank/save startup gating missing")
    if 'ScreenStack::push(std::make_unique<RogueInspectScreen>());' not in edited:
        raise ValueError("Direct Rogue UI startup missing")
    # Never offer reset/delete of an extdata archive in the Rogue inspector.
    # The branch below may CREATE the fork's distinct extdata ID, but it
    # never calls FSUSER_DeleteExtSaveData. If creation fails, abort.
    archive_old = archive.read_text(encoding="utf-8")
    old_block = """if (R_SUMMARY(openResult) == RS_INVALIDSTATE)
        {
            if (!confirmReset())
            {
                return openResult;
            }

            if (R_FAILED(res = resetPKSMExtdataArchive(execPath)))
            {
                return res;
            }
        }"""
    new_block = """if (R_SUMMARY(openResult) == RS_INVALIDSTATE)
        {
            // Rogue-only fork: CREATE the separate extdata if absent; never reset.
            // A present but unmountable archive must fail rather than be erased.
            if (R_FAILED(res = mountNewPKSMExtdataArchive(execPath)))
            {
                return res;
            }
        }"""
    archive_new = once(archive_old, old_block, new_block, "non-destructive extdata handling")
    if "Archive::init(execPath, nullptr);" not in edited:
        raise ValueError("Unsafe extdata reset callback still present")
    if "resetPKSMExtdataArchive(execPath)" in new_block:
        raise ValueError("Destructive reset inadvertently enabled")
    # Apply only after all markers have been checked in both source files.
    app.write_text(edited, encoding="utf-8")
    archive.write_text(archive_new, encoding="utf-8")

def main() -> int:
    parser=argparse.ArgumentParser()
    parser.add_argument("pksm_checkout", type=Path)
    args=parser.parse_args()
    try:
        restrict(args.pksm_checkout.resolve())
    except (OSError, ValueError, UnicodeError) as exc:
        print("Readonly startup restriction REFUSED: " + str(exc), file=sys.stderr)
        return 1
    print("Restricted experimental startup: direct Rogue screen, no standard title scans or auto-update. Not yet hardware tested.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
