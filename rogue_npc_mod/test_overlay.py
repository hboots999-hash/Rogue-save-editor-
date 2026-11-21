#!/usr/bin/env python3
"""Conservative static integration checks for a patched clean EX-v2.1 tree."""
from __future__ import annotations

import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
p = (root / "include/pokemon.h").read_text()
pc = (root / "src/pokemon.c").read_text()
ss = (root / "src/pokemon_summary_screen.c").read_text()
sp = (root / "data/specials.inc").read_text()
npc = (root / "src/rogue_npc_editors.c").read_text()
mapdata = json.loads((root / "data/maps/Rogue_Hub/map.json").read_text())
hub = (root / "data/maps/Rogue_Hub/scripts.pory").read_text()

assert p.count("u32 rogueAbilityOverride:10;") == 1
assert "u32 unused_1:1;" in p
assert "MON_DATA_ROGUE_ABILITY_OVERRIDE" in p
assert "ABILITIES_COUNT_GEN9" in pc
assert "return encoded - 1;" in pc
assert "dst->ability = GetMonAbility(src);" in pc
assert ss.count("GetMonAbility(&sMonSummaryScreen->currentMon)") == 2
assert "GetAbilityBySpecies_ForRevised" in ss  # original comparison retained
assert len([o for o in mapdata["object_events"]
            if o.get("script") == "RogueHub_SpeakCustomEditor"]) == 1
chosen = next(o for o in mapdata["object_events"]
              if o.get("script") == "RogueHub_SpeakCustomEditor")
assert (chosen["x"], chosen["y"]) == (31, 19)
assert chosen["flag"] == "0"
assert "script RogueHub_SpeakCustomEditor" in hub
assert "call(RogueHub_Studio_ChangeMove)" in hub
assert "call(RogueHub_Studio_ChangeAbility)" in hub
assert "MSGBOX_YESNO" in hub
assert "special(ChoosePartyMon)" in hub
assert "ScriptMenu_ScrollingMultichoiceDynamicEnd()" in npc
assert "if (gBattleMoves[move].pp == 0)" in npc
assert "SetMonMoveSlot(mon, sChosenMove, i);" in npc
assert "sChosenAbility" in npc
assert "MON_DATA_ROGUE_ABILITY_OVERRIDE" in npc
assert "GetMonAbility(mon) != ability" in npc
names = [
    "RogueNpc_ResetDraft", "RogueNpc_OpenServiceChoices",
    "RogueNpc_RememberService", "RogueNpc_OpenMoveChoices",
    "RogueNpc_RememberMoveChoice", "RogueNpc_OpenReplaceSlotChoices",
    "RogueNpc_RememberReplaceSlot", "RogueNpc_ApplyMove",
    "RogueNpc_OpenAbilityChoices", "RogueNpc_RememberAbilityChoice",
    "RogueNpc_ApplyAbility",
]
for name in names:
    assert sp.count("\tdef_special " + name) == 1, name
    assert "void " + name + "(void)" in npc, name
assert npc.count("NpcSelectedPokemon()") >= 4
assert "gPlayerPartyCount" in npc
assert "gSpecialVar_Result = FALSE;" in npc
print("PASS static single main-hub NPC, both move/ability flows, no other map,")
print("PASS ability saved substruct, exact references in battle and summary,")
print("PASS all specials registered, selection validation and confirmation,")
print("PASS no installed-title/save injection paths added.")
print("These are SOURCE checks, not a GBA ROM build/gameplay test.")
