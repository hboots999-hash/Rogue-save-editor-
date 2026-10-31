#!/usr/bin/env python3
"""Apply Phase 3's read-only Rogue inspection screen to a clean PKSM checkout.

This script modifies the supplied PKSM working tree. It does NOT run an on-device
build, and it neither reads nor writes real Pokémon save files.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import sys

ROOT = Path(__file__).resolve().parent.parent


def replace_once(data: str, old: str, new: str, where: Path) -> str:
    count = data.count(old)
    if count != 1:
        raise ValueError(f"{where}: expected one matching anchor, found {count}: {old[:80]!r}")
    return data.replace(old, new, 1)


def patch_tree(target: Path) -> None:
    title = target / "3ds/source/gui/screen/TitleLoadScreen.cpp"
    rootmake = target / "Makefile"
    ds_make = target / "3ds/Makefile"
    for p in (title, rootmake, ds_make):
        if not p.is_file():
            raise ValueError(f"Not a PKSM checkout: missing {p}")
    title_text = title.read_text()
    root_text = rootmake.read_text()
    ds_text = ds_make.read_text()
    if 'RogueInspectScreen' in title_text:
        raise ValueError("Rogue inspection already installed; refusing a second patch")
    title_text = replace_once(title_text, '#include "TitleLoadScreen.hpp"',
        '#include "TitleLoadScreen.hpp"\n#include "RogueInspectScreen.hpp"', title)
    title_text = replace_once(title_text,
        '    u32 buttonsDown = hidKeysDown();',
        '    u32 buttonsDown = hidKeysDown();\n'
        '    // Separate SD-copy inspector; do not invoke PKSM retail-save loading.\n'
        '    if (buttonsDown & KEY_SELECT)\n'
        '    {\n'
        '        ScreenStack::push(std::make_unique<RogueInspectScreen>());\n'
        '        return;\n'
        '    }', title)
    root_text = replace_once(root_text, 'export PKSM_TITLE\t\t:= \tPKSM',
        'export PKSM_TITLE\t\t:= \tPKSM Rogue Inspect', rootmake)
    ds_text = replace_once(ds_text, 'UNIQUE_ID\t\t:=\t0xEC100',
        'UNIQUE_ID\t\t:=\t0xF73A2', ds_make)
    ds_text = replace_once(ds_text, 'PRODUCT_CODE\t:=\tCTR-HB-PKSM',
        'PRODUCT_CODE\t:=\tCTR-HB-PRGI', ds_make)
    ds_text = replace_once(ds_text, 'source/gui/screen \\\n',
        'source/gui/screen \\\n\t\t\t\t\tsource/rogue \\\n', ds_make)
    ds_text = replace_once(ds_text, 'include/gui/screen \\\n',
        'include/gui/screen \\\n\t\t\t\t\tinclude/rogue \\\n', ds_make)

    copies = {
        'pksm_overlay/RogueInspectScreen.hpp': '3ds/include/gui/screen/RogueInspectScreen.hpp',
        'pksm_overlay/RogueInspectScreen.cpp': '3ds/source/gui/screen/RogueInspectScreen.cpp',
        'bridge/rogue_bridge.h': '3ds/include/rogue/rogue_bridge.h',
        'bridge/rogue_bridge.cpp': '3ds/source/rogue/rogue_bridge.cpp',
        'bridge/rogue_edit.h': '3ds/include/rogue/rogue_edit.h',
        'bridge/rogue_edit.cpp': '3ds/source/rogue/rogue_edit.cpp',
        'bridge/rogue_pokemon_edit.h': '3ds/include/rogue/rogue_pokemon_edit.h',
        'bridge/rogue_pokemon_edit.cpp': '3ds/source/rogue/rogue_pokemon_edit.cpp',
        'bridge/rogue_ball_insert.h': '3ds/include/rogue/rogue_ball_insert.h',
        'bridge/rogue_ball_insert.cpp': '3ds/source/rogue/rogue_ball_insert.cpp',
        'bridge/rogue_hub_probe.h': '3ds/include/rogue/rogue_hub_probe.h',
        'bridge/rogue_hub_probe.cpp': '3ds/source/rogue/rogue_hub_probe.cpp',
        'roguesm_core/include/edit_draft.hpp': '3ds/include/rogue/edit_draft.hpp',
        'roguesm_core/source/edit_draft.cpp': '3ds/source/rogue/edit_draft.cpp',
        'roguesm_core/include/candidate_profile.hpp': '3ds/include/rogue/candidate_profile.hpp',
        'roguesm_core/include/observed_profile.hpp': '3ds/include/rogue/observed_profile.hpp',
        'roguesm_core/include/sector.hpp': '3ds/include/rogue/sector.hpp',
        'roguesm_core/source/candidate_profile.cpp': '3ds/source/rogue/candidate_profile.cpp',
        'roguesm_core/source/observed_profile.cpp': '3ds/source/rogue/observed_profile.cpp',
        'roguesm_core/source/sector.cpp': '3ds/source/rogue/sector.cpp',
    }
    for source, dest in copies.items():
        a = ROOT / source
        b = target / dest
        if not a.is_file() or b.exists():
            raise ValueError(f"Missing source or destination already exists: {a}, {b}")
    # All compatibility checks passed: start applying; the caller should run this
    # on a disposable git checkout or clean git branch.
    title.write_text(title_text)
    rootmake.write_text(root_text)
    ds_make.write_text(ds_text)
    for source, dest in copies.items():
        dst = target / dest
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / source, dst)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('pksm_checkout', type=Path, help='root directory of a clean PKSM checkout')
    args = parser.parse_args()
    try:
        patch_tree(args.pksm_checkout.resolve())
    except (OSError, ValueError) as ex:
        print(f"Not applied: {ex}", file=sys.stderr)
        return 1
    print('Patched PKSM with read-only Rogue Inspect screen. Not built or hardware-tested.')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
