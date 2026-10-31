#!/usr/bin/env python3
"""Restrict PKSM to Rogue SD-copy inspection and non-destructive startup.

Run AFTER isolate_pksm.py and apply_to_pksm.py on a disposable PKSM checkout.
The fork must NEVER delete an extdata archive or offer a storage reset.
Source anchors are checked before any changes are committed to working files.
"""
from pathlib import Path
import argparse
import sys


def once(data: str, old: str, new: str, name: str) -> str:
    count = data.count(old)
    if count != 1:
        raise ValueError(f"{name}: expected one anchor, got {count}")
    return data.replace(old, new, 1)


def cut_section(data: str, start: str, end: str, new: str, name: str) -> str:
    if data.count(start) != 1 or data.count(end) != 1:
        raise ValueError(f"{name}: expected unique section boundaries")
    i = data.index(start)
    j = data.index(end)
    if j <= i:
        raise ValueError(f"{name}: wrong section order")
    return data[:i] + new + data[j:]


def restrict(root: Path) -> None:
    app = root / "3ds/source/app.cpp"
    archive = root / "3ds/source/io/Archive.cpp"
    inspector = root / "3ds/source/gui/screen/RogueInspectScreen.cpp"
    if not app.is_file() or not archive.is_file() or not inspector.is_file():
        raise ValueError("Missing upstream startup/archive or Rogue overlay; apply overlay first")

    original = app.read_text(encoding="utf-8")
    if "rogueInspectOnly" in original:
        raise ValueError("Startup already restricted")
    edited = once(original, '#include "app.hpp"',
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
        "missing-asset recovery")
    edited = once(edited,
        'if (Configuration::getInstance().autoUpdate() && update(execPath))',
        'if (!rogueInspectOnly && Configuration::getInstance().autoUpdate() && update(execPath))',
        "automatic updater")
    edited = once(edited,
        "Archive::init(execPath, confirmExtdataReset);",
        "Archive::init(execPath, nullptr);",
        "disable destructive reset callback")
    edited = once(edited,
        '    const bool contentUp =\n',
        '    const bool contentUp = rogueInspectOnly || (\n',
        "save-scanning startup")
    edited = once(edited,
        '            stopCartScan);\n',
        '            stopCartScan));\n',
        "cart-scanning block")
    edited = once(edited,
        'ScreenStack::push(std::make_unique<TitleLoadScreen>());',
        'ScreenStack::push(std::make_unique<RogueInspectScreen>());',
        "startup UI")

    # Remove the entire old "Press A to reset archive" function.
    edited = cut_section(
        edited,
        "    bool confirmExtdataReset()\n    {",
        "    Result HBLDR_SetTarget(",
        "    // Rogue-only build: no extdata reset UI is compiled.\n\n",
        "destructive reset UI")

    archive_old = archive.read_text(encoding="utf-8")
    archive_new = once(archive_old,
        """if (R_SUMMARY(openResult) == RS_INVALIDSTATE)
        {
            if (!confirmReset())
            {
                return openResult;
            }

            if (R_FAILED(res = resetPKSMExtdataArchive(execPath)))
            {
                return res;
            }
        }""",
        """if (R_SUMMARY(openResult) == RS_INVALIDSTATE)
        {
            // Attempt a CREATE only for this fork's separate extdata ID.
            // If an existing archive cannot mount, creation must fail closed.
            if (R_FAILED(res = mountNewPKSMExtdataArchive(execPath)))
            {
                return res;
            }
        }""",
        "create-only extdata handling")

    # Old PKSM uses a repair callback to DELETE data when unreadable.
    # Never call the callback, even when the missing archive was opened.
    archive_new = cut_section(
        archive_new,
        "    else if (!extdataReadable())\n    {",
        '    else\n    {\n        auto checkFile = data().file',
        """    else if (!extdataReadable())
    {
        Logging::startupLog("archive", "Rogue inspect: existing extdata root unreadable; refusing reset");
        return R_FAILED(data().result()) ? data().result() : (Result)-1;
    }
""",
        "unreadable extdata recovery")

    # PKSM's legacy no-sizeCheck migration includes an unconditional
    # FSUSER_DeleteExtSaveData call. A copy-first migration is still a delete.
    begin = "        if (!checkFile)\n        {"
    finish = "        else\n        {\n            checkFile->close();\n        }"
    if archive_new.count(begin) != 1 or archive_new.count(finish) != 1:
        raise ValueError("legacy migration boundaries no longer match")
    i = archive_new.index(begin)
    j = archive_new.index(finish)
    if j <= i or "FSUSER_DeleteExtSaveData" not in archive_new[i:j]:
        raise ValueError("unrecognised legacy migration code; refusing")
    archive_new = archive_new[:i] + """        if (!checkFile)
        {
            Logging::startupLog("archive", "Rogue inspect: extdata missing sizeCheck; refusing migration");
            return (Result)-1;
        }
""" + archive_new[j:]

    archive_new = once(archive_new,
        "    const auto confirmReset = [confirmExtdataReset]\n"
        "    { return confirmExtdataReset && confirmExtdataReset(); };\n",
        "", "remove unused reset callback")
    archive_new = once(archive_new,
        "Result Archive::init(const std::string& execPath, bool (*confirmExtdataReset)())\n"
        "{\n    Result res = 0;",
        "Result Archive::init(const std::string& execPath, bool (*confirmExtdataReset)())\n"
        "{\n    (void)confirmExtdataReset;\n    Result res = 0;",
        "suppress intentionally unused callback parameter")

    init_part = archive_new.split("void Archive::exit(void)", 1)[0]
    if "FSUSER_DeleteExtSaveData" in init_part or "resetPKSMExtdataArchive(" in init_part:
        raise ValueError("destructive extdata path remains reachable in Archive::init")
    if "confirmExtdataReset()" in edited or "Archive::init(execPath, nullptr);" not in edited:
        raise ValueError("unsafe reset callback remains in app startup")

    # All anchors and invariants passed: write only the disposable PKSM copy.
    app.write_text(edited, encoding="utf-8")
    archive.write_text(archive_new, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("pksm_checkout", type=Path)
    args = parser.parse_args()
    try:
        restrict(args.pksm_checkout.resolve())
    except (OSError, ValueError, UnicodeError) as exc:
        print("Rogue-only safety patch REFUSED: " + str(exc), file=sys.stderr)
        return 1
    print("Rogue-only startup patched: create-only extdata; destructive reset/migration disabled. UNTESTED on hardware.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
