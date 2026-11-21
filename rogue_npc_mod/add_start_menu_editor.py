#!/usr/bin/env python3
"""Add START -> EDIT MON to the upstream EX-v2.1 source AFTER the NPC overlay.

This is a fail-closed source modification. It reuses the existing NPC move
editor and ability-override format. It does not patch CIA/ROM/save bytes.
"""
from pathlib import Path
import argparse

PORY = r'''
// START-menu edition: no NPC has been spoken to, so do not use faceplayer.
script RogueHub_StartMenuEditor
{
    lockall
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
    releaseall
}
'''

CALLBACK = '''static bool8 StartMenuMonEditorCallback(void)
{
    // Use the same Start-menu-to-script path as RETIRE / QUICK SAVE.
    RemoveExtraStartMenuWindows();
    HideStartMenu();
    ScriptContext_SetupScript(RogueHub_StartMenuEditor);
    return TRUE;
}

'''

def once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one anchor, found {count}")
    return text.replace(old, new, 1)

def patch_menu(s: str) -> str:
    if "StartMenuMonEditorCallback" in s:
        raise RuntimeError("START menu already patched")
    s = once(s, '#include "rogue_settings.h"',
        '#include "rogue_settings.h"\n\nextern const u8 RogueHub_StartMenuEditor[];', "script linkage")
    s = once(s, "    MENU_ACTION_DEBUG,\n};",
        "    MENU_ACTION_DEBUG,\n    MENU_ACTION_MON_EDITOR,\n};", "menu enum")
    s = once(s, "static bool8 StartMenuDebugCallback(void);",
        "static bool8 StartMenuDebugCallback(void);\nstatic bool8 StartMenuMonEditorCallback(void);", "prototype")
    s = once(s, 'static const u8 sText_MenuDebug[] = _("DEBUG");',
        'static const u8 sText_MenuDebug[] = _("DEBUG");\nstatic const u8 sText_MenuMonEditor[] = _("EDIT MON");',
        "label")
    s = once(s,
        "    [MENU_ACTION_DEBUG]           = {sText_MenuDebug,       {.u8_void = StartMenuDebugCallback}},",
        "    [MENU_ACTION_DEBUG]           = {sText_MenuDebug,       {.u8_void = StartMenuDebugCallback}},\n"
        "    [MENU_ACTION_MON_EDITOR]      = {sText_MenuMonEditor,   {.u8_void = StartMenuMonEditorCallback}},",
        "menu action")
    # The original nine-action GBA menu cannot accommodate a tenth row.
    # In normal play (hub/run) it has room; guard any future unusual modes.
    add = (
        "    if (!inCatchingContest && FlagGet(FLAG_SYS_POKEMON_GET) == TRUE\n"
        "        && sNumStartMenuActions < ARRAY_COUNT(sCurrentStartMenuActions))\n"
        "        AddStartMenuAction(MENU_ACTION_MON_EDITOR);\n"
    )
    s = once(s,
        "    AddStartMenuAction(MENU_ACTION_OPTION);\n    //AddStartMenuAction(MENU_ACTION_EXIT);",
        "    AddStartMenuAction(MENU_ACTION_OPTION);\n" + add +
        "    //AddStartMenuAction(MENU_ACTION_EXIT);", "hub menu")
    s = once(s,
        "    AddStartMenuAction(MENU_ACTION_RETIRE_ADVENTURE);\n    //AddStartMenuAction(MENU_ACTION_EXIT);",
        "    AddStartMenuAction(MENU_ACTION_RETIRE_ADVENTURE);\n" + add +
        "    //AddStartMenuAction(MENU_ACTION_EXIT);", "run menu")
    s = once(s, "                && gMenuCallback != StartMenuDebugCallback",
        "                && gMenuCallback != StartMenuDebugCallback\n"
        "                && gMenuCallback != StartMenuMonEditorCallback", "no-fade script entry")
    s = once(s, "static bool8 StartMenuDebugCallback(void)\n{",
        CALLBACK + "static bool8 StartMenuDebugCallback(void)\n{", "callback")
    assert s.count("AddStartMenuAction(MENU_ACTION_MON_EDITOR);") == 2
    assert s.count("ScriptContext_SetupScript(RogueHub_StartMenuEditor);") == 1
    return s

def patch_scripts(s: str) -> str:
    if "script RogueHub_StartMenuEditor" in s:
        raise RuntimeError("Start menu script already patched")
    if ("script RogueHub_Studio_ChangeMove" not in s
            or "script RogueHub_Studio_ChangeAbility" not in s):
        raise RuntimeError("Existing NPC overlay missing; run apply_source_overlay.py first")
    return s.rstrip() + "\n\n" + PORY.strip() + "\n"

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    root = parser.parse_args().source.resolve()
    if not (root / "src/rogue_npc_editors.c").is_file():
        raise RuntimeError("NPC editor source absent; apply existing overlay first")
    menu = root / "src/start_menu.c"
    scripts = root / "data/maps/Rogue_Hub/scripts.pory"
    if not menu.is_file() or not scripts.is_file():
        raise RuntimeError("Missing EX-v2.1 source files")
    # Build and validate both proposed changes before writing either one.
    new_menu = patch_menu(menu.read_text(encoding="utf-8"))
    new_scripts = patch_scripts(scripts.read_text(encoding="utf-8"))
    menu.write_text(new_menu, encoding="utf-8", newline="\n")
    scripts.write_text(new_scripts, encoding="utf-8", newline="\n")
    print("PASS: START -> EDIT MON in hub/run, invoking existing move/ability NPC engine")
    print("SOURCE ONLY; not a verified playable ROM or installable CIA.")

if __name__ == "__main__":
    main()
