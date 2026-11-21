#!/usr/bin/env python3
"""Apply the experimental single-hub-NPC mod to a clean EX-v2.1 source tree.

Never edits original ROMs, CIAs or game saves. See README.md for limitations.
Requires the upstream source at EX-v2.1, not a newer/different decomp release.
"""
from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path


def replace_once(src: str, old: str, new: str, label: str) -> str:
    count = src.count(old)
    if count != 1:
        raise ValueError(f"{label}: expected exactly one source anchor, found {count}")
    return src.replace(old, new, 1)


def edit(path: Path, callback):
    original = path.read_text(encoding="utf-8")
    updated = callback(original)
    if updated == original:
        raise ValueError(f"No changes made: {path}")
    path.write_text(updated, encoding="utf-8", newline="\n")


def patch_pokemon_h(source: str) -> str:
    source = replace_once(source,
        "    MON_DATA_TUTOR_MOVE_LVL,\n",
        "    MON_DATA_TUTOR_MOVE_LVL,\n"
        "    MON_DATA_ROGUE_ABILITY_OVERRIDE, // optional encoded 0 or ability+1\n",
        "pokemon.h new MON_DATA enum")
    source = replace_once(source,
        "    u32 unused_1:11;",
        "    u32 rogueAbilityOverride:10; // 0=species default, n=ability id n-1\n"
        "    u32 unused_1:1;",
        "pokemon.h existing unused Growth bits")
    return source


def patch_pokemon_c(source: str) -> str:
    # The GetBoxMonData3 case is unique in the original source.
    source = replace_once(source,
        "        case MON_DATA_ABILITY_NUM:\n"
        "            retVal = substruct3->abilityNum;\n"
        "            break;",
        "        case MON_DATA_ABILITY_NUM:\n"
        "            retVal = substruct3->abilityNum;\n"
        "            break;\n"
        "        case MON_DATA_ROGUE_ABILITY_OVERRIDE:\n"
        "            retVal = substruct0->rogueAbilityOverride;\n"
        "            break;",
        "GetBoxMonData3 override storage")
    source = replace_once(source,
        "        case MON_DATA_ABILITY_NUM:\n"
        "            SET8(substruct3->abilityNum);\n"
        "            break;",
        "        case MON_DATA_ABILITY_NUM:\n"
        "            SET8(substruct3->abilityNum);\n"
        "            break;\n"
        "        case MON_DATA_ROGUE_ABILITY_OVERRIDE:\n"
        "        {\n"
        "            u16 override = data[0] | ((u16)data[1] << 8);\n"
        "            if (override <= ABILITIES_COUNT_GEN9)\n"
        "                substruct0->rogueAbilityOverride = override;\n"
        "            break;\n"
        "        }",
        "SetBoxMonData validated override")
    source = replace_once(source,
        "u16 GetMonAbility(struct Pokemon *mon)\n"
        "{\n"
        "    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);\n"
        "    u32 otId = GetMonData(mon, MON_DATA_OT_ID, NULL);\n"
        "    u8 abilityNum = GetMonData(mon, MON_DATA_ABILITY_NUM, NULL);\n"
        "    return GetAbilityBySpecies(species, abilityNum, otId);\n"
        "}",
        "u16 GetMonAbility(struct Pokemon *mon)\n"
        "{\n"
        "    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);\n"
        "    u32 otId = GetMonData(mon, MON_DATA_OT_ID, NULL);\n"
        "    u8 abilityNum = GetMonData(mon, MON_DATA_ABILITY_NUM, NULL);\n"
        "    u16 encoded = GetMonData(mon, MON_DATA_ROGUE_ABILITY_OVERRIDE, NULL);\n"
        "    if (encoded != 0 && encoded <= ABILITIES_COUNT_GEN9)\n"
        "        return encoded - 1;\n"
        "    return GetAbilityBySpecies(species, abilityNum, otId);\n"
        "}",
        "GetMonAbility must respect per-mon stored ability")
    source = replace_once(source,
        "    dst->ability = GetAbilityBySpecies(dst->species, dst->abilityNum, dst->otId);",
        "    dst->ability = GetMonAbility(src);",
        "PokemonToBattleMon ability must match summary and save")
    return source


def patch_summary(source: str) -> str:
    original = (
       "GetAbilityBySpecies(sMonSummaryScreen->summary.species, "
       "sMonSummaryScreen->summary.abilityNum, sMonSummaryScreen->summary.OTID)"
    )
    if source.count(original) != 2:
        raise ValueError("summary must have exactly two actual-ability lookups")
    return source.replace(original, "GetMonAbility(&sMonSummaryScreen->currentMon)")


SPECIALS = [
    "RogueNpc_ResetDraft",
    "RogueNpc_OpenServiceChoices", "RogueNpc_RememberService",
    "RogueNpc_OpenMoveChoices", "RogueNpc_RememberMoveChoice",
    "RogueNpc_OpenReplaceSlotChoices", "RogueNpc_RememberReplaceSlot",
    "RogueNpc_ApplyMove", "RogueNpc_OpenAbilityChoices",
    "RogueNpc_RememberAbilityChoice", "RogueNpc_ApplyAbility",
]


def patch_specials(source: str) -> str:
    hook = "    def_special ChoosePartyMon"
    # Source uses a single tab, not spaces.
    anchor = "\tdef_special ChoosePartyMon"
    inject = anchor + "\n" + "\n".join("\tdef_special " + name for name in SPECIALS)
    return replace_once(source, anchor, inject, "specials hook near ChoosePartyMon")


def patch_map(source: str) -> str:
    data = json.loads(source)
    if data.get("id") != "MAP_ROGUE_HUB":
        raise ValueError("must patch the main outdoor hub, not a side room")
    npc = {
      "graphics_id": "OBJ_EVENT_GFX_SCIENTIST_1",
      "x": 31, "y": 19, "elevation": 3,
      "movement_type": "MOVEMENT_TYPE_FACE_DOWN",
      "movement_range_x": 0, "movement_range_y": 0,
      "trainer_type": "TRAINER_TYPE_NONE",
      "trainer_sight_or_berry_tree_id": "0",
      "script": "RogueHub_SpeakCustomEditor",
      "flag": "0",
    }
    occupied = {(obj["x"], obj["y"]) for obj in data["object_events"]}
    occupied |= {(obj["x"], obj["y"]) for obj in data.get("warp_events", [])}
    if (npc["x"],npc["y"]) in occupied:
        raise ValueError("NPC coordinates collide with event/warp")
    data["object_events"].append(npc)
    return json.dumps(data, indent=2, ensure_ascii=False) + "\n"


SCRIPT = r'''
// Optional fan-made editor NPC, always visible in the outdoor main hub.
// No save-file injection or installed title manipulation.
// All modifications are mediated by the game-owned Pokemon SetMonData and Save.

script RogueHub_SpeakCustomEditor
{
    lock
    faceplayer

    msgbox(format("Welcome to the MOVE and ABILITY STUDIO!\pI can teach a new move or give your Pokémon a new Ability."), MSGBOX_DEFAULT)

    special(RogueNpc_ResetDraft)
    special(RogueNpc_OpenServiceChoices)
    waitstate
    special(RogueNpc_RememberService)

    if(var(VAR_RESULT) == 1)
    {
        call(RogueHub_Studio_ChangeMove)
    }
    elif(var(VAR_RESULT) == 2)
    {
        call(RogueHub_Studio_ChangeAbility)
    }

    release
}

script RogueHub_Studio_ChangeMove
{
    msgbox(format("Choose the Pokémon you want to teach."), MSGBOX_DEFAULT)
    special(ChoosePartyMon)
    waitstate

    if(var(VAR_0x8004) < 6)
    {
        special(RogueNpc_OpenMoveChoices)
        waitstate
        special(RogueNpc_RememberMoveChoice)

        if(var(VAR_RESULT) == TRUE)
        {
            special(RogueNpc_OpenReplaceSlotChoices)
            waitstate
            special(RogueNpc_RememberReplaceSlot)

            if(var(VAR_RESULT) == TRUE)
            {
                msgbox(format("Replace this move?\pThe new move will start with normal PP."), MSGBOX_YESNO)
                if(var(VAR_RESULT) == YES)
                {
                    special(RogueNpc_ApplyMove)
                    if(var(VAR_RESULT) == TRUE)
                    {
                        msgbox(format("Done! Your Pokémon has learned the selected move.\pSave your game when you're ready."), MSGBOX_DEFAULT)
                    }
                    else
                    {
                        msgbox(format("The move change was blocked to protect your Pokémon."), MSGBOX_DEFAULT)
                    }
                }
            }
        }
    }
}

script RogueHub_Studio_ChangeAbility
{
    msgbox(format("Choose the Pokémon whose Ability you want to change."), MSGBOX_DEFAULT)
    special(ChoosePartyMon)
    waitstate

    if(var(VAR_0x8004) < 6)
    {
        special(RogueNpc_OpenAbilityChoices)
        waitstate
        special(RogueNpc_RememberAbilityChoice)

        if(var(VAR_RESULT) == TRUE)
        {
            msgbox(format("Give this Pokémon the selected Ability?\pSpecies Default removes any custom Ability."), MSGBOX_YESNO)
            if(var(VAR_RESULT) == YES)
            {
                special(RogueNpc_ApplyAbility)
                if(var(VAR_RESULT) == TRUE)
                {
                    msgbox(format("Done! Your Pokémon now has that Ability.\pSave your game when you're ready."), MSGBOX_DEFAULT)
                }
                else
                {
                    msgbox(format("The Ability change was blocked to protect your Pokémon."), MSGBOX_DEFAULT)
                }
            }
        }
    }
}
'''


def patch_scripts(source: str) -> str:
    if "script RogueHub_SpeakCustomEditor" in source:
        raise ValueError("already patched")
    return source.rstrip() + "\n\n" + SCRIPT.lstrip()


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("source", type=Path, help="upstream EX-v2.1 clean directory")
    args = p.parse_args()
    root = args.source.resolve()
    required = [
        "include/pokemon.h", "src/pokemon.c",
        "src/pokemon_summary_screen.c", "data/specials.inc",
        "data/maps/Rogue_Hub/map.json",
        "data/maps/Rogue_Hub/scripts.pory",
    ]
    for path in required:
        if not (root / path).is_file():
            raise FileNotFoundError(root / path)
    # Read and validate ALL patches in memory before writing anything.
    transforms = {
        "include/pokemon.h": patch_pokemon_h,
        "src/pokemon.c": patch_pokemon_c,
        "src/pokemon_summary_screen.c": patch_summary,
        "data/specials.inc": patch_specials,
        "data/maps/Rogue_Hub/map.json": patch_map,
        "data/maps/Rogue_Hub/scripts.pory": patch_scripts,
    }
    prepared = {}
    for path, transform in transforms.items():
        original = (root / path).read_text(encoding="utf-8")
        patched = transform(original)
        if patched == original:
            raise ValueError(f"Patch did not modify {path}")
        prepared[path] = patched

    overlay_c = Path(__file__).parent / "overlay" / "src" / "rogue_npc_editors.c"
    if not overlay_c.is_file():
        raise FileNotFoundError(overlay_c)
    dest = root / "src" / "rogue_npc_editors.c"
    if dest.exists():
        raise FileExistsError("Refusing overwrite of existing NPC source: " + str(dest))

    for path, data in prepared.items():
        (root / path).write_text(data, encoding="utf-8", newline="\n")
    shutil.copyfile(overlay_c, dest)
    print("SOURCE PATCH COMPLETE: one outdoor hub NPC offering Move and Ability.")
    print("Not a validated ROM build. Never treat this as an installation CIA.")


if __name__ == "__main__":
    main()
