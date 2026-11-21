// SPDX-License-Identifier: GPL-3.0-or-later
// Experimental Emerald Rogue EX-v2.1 source overlay.
// Two OPTIONAL hub NPCs. No direct 3DS/AGBSAVE/CMAC access.
// The native game handles its own Save; backup first and never overwrite
// the owner's original installed CIA without a separate Title ID.
// NOT YET native-compiled or validated against owner H168 ROM v2.1.2.
#include "global.h"
#include "battle.h"
#include "event_data.h"
#include "pokemon.h"
#include "script_menu.h"
#include "string_util.h"
#include "constants/abilities.h"
#include "constants/moves.h"
#include "constants/species.h"

// ScriptMenu uses 0x7F as cancel; offset real IDs to avoid move/ability 127
// accidentally becoming Cancel.
#define NPC_CHOICE_BASE 0x1000
#define NPC_MOVE_SLOT_BASE 0x2000
#define NPC_CANCEL 0x7f

static u16 sChosenMove;
static u16 sChosenSlot;
static u16 sChosenAbility;

// More than 255 entries are supported by the existing dynamic SCROLLING menu
// (u16 count); this avoids overflowing the normal move-relearner's 255 cap.
static void NpcBeginList(u16 capacity)
{
    gSpecialVar_0x8005 = 1; // left
    gSpecialVar_0x8006 = 1; // top
    gSpecialVar_0x8007 = 8; // visible rows
    gSpecialVar_0x8008 = TRUE; // B can cancel
    ScriptMenu_ScrollingMultichoiceDynamicBegin(capacity);
}

static struct Pokemon *NpcSelectedPokemon(void)
{
    const u16 slot = gSpecialVar_0x8004;
    if (slot >= PARTY_SIZE || slot >= gPlayerPartyCount)
        return NULL;
    if (GetMonData(&gPlayerParty[slot], MON_DATA_SPECIES_OR_EGG, NULL)
            == SPECIES_EGG ||
        GetMonData(&gPlayerParty[slot], MON_DATA_SPECIES, NULL)
            == SPECIES_NONE)
        return NULL;
    return &gPlayerParty[slot];
}

void RogueNpc_OpenMoveChoices(void)
{
    u16 move;
    if (NpcSelectedPokemon() == NULL)
    {
        gSpecialVar_Result = NPC_CANCEL;
        return;
    }

    NpcBeginList(MOVES_COUNT);
    for (move = 1; move < MOVES_COUNT; move++)
    {
        // Refuse dummy/zero-PP moves. Move PP may vary across Rogue's
        // rulesets: game-internal SetMonMoveSlot resolves standard PP.
        if (gBattleMoves[move].pp == 0)
            continue;
        ScriptMenu_ScrollingMultichoiceDynamicAppendOption(
            gMoveNames[move], NPC_CHOICE_BASE + move);
    }
    ScriptMenu_ScrollingMultichoiceDynamicEnd();
}

void RogueNpc_RememberMoveChoice(void)
{
    struct Pokemon *mon = NpcSelectedPokemon();
    u16 move;
    u8 i;
    sChosenMove = MOVE_NONE;
    if (!mon || gSpecialVar_Result <= NPC_CHOICE_BASE ||
        gSpecialVar_Result >= NPC_CHOICE_BASE + MOVES_COUNT)
    {
        gSpecialVar_Result = FALSE;
        return;
    }
    move = gSpecialVar_Result - NPC_CHOICE_BASE;
    if (gBattleMoves[move].pp == 0)
    {
        gSpecialVar_Result = FALSE;
        return;
    }
    for (i = 0; i < MAX_MON_MOVES; i++)
        if (GetMonData(mon, MON_DATA_MOVE1 + i, NULL) == move)
        {
            gSpecialVar_Result = FALSE;
            return; // refuse duplicates
        }
    sChosenMove = move;
    gSpecialVar_Result = TRUE;
}

void RogueNpc_OpenReplaceSlotChoices(void)
{
    struct Pokemon *mon = NpcSelectedPokemon();
    u8 i;
    if (!mon || sChosenMove == MOVE_NONE)
    {
        gSpecialVar_Result = NPC_CANCEL;
        return;
    }
    NpcBeginList(MAX_MON_MOVES);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        const u16 oldMove = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);
        ScriptMenu_ScrollingMultichoiceDynamicAppendOption(
            gMoveNames[oldMove], NPC_MOVE_SLOT_BASE + i);
    }
    ScriptMenu_ScrollingMultichoiceDynamicEnd();
}

void RogueNpc_RememberReplaceSlot(void)
{
    sChosenSlot = gSpecialVar_Result;
}

void RogueNpc_ApplyMove(void)
{
    struct Pokemon *mon = NpcSelectedPokemon();
    const u16 i = sChosenSlot - NPC_MOVE_SLOT_BASE;
    gSpecialVar_Result = FALSE;
    if (!mon || sChosenSlot < NPC_MOVE_SLOT_BASE ||
        i >= MAX_MON_MOVES || !sChosenMove ||
        sChosenMove >= MOVES_COUNT || gBattleMoves[sChosenMove].pp == 0)
        return;

    // A newly selected move begins without inherited PP Ups.
    {
        u8 ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
        ppBonuses &= (u8)~(3u << (2u * i));
        SetMonData(mon, MON_DATA_PP_BONUSES, &ppBonuses);
    }
    SetMonMoveSlot(mon, sChosenMove, i);
    gSpecialVar_Result =
        GetMonData(mon, MON_DATA_MOVE1 + i, NULL) == sChosenMove;
}

// Save format: the Source EX-v2.1 Growth substruct contains unused high
// 11 bits after the 21-bit experience field. 0 = normal species ability;
// n>=1 = explicit ability ID (n-1). This function requires the companion
// pokemon.h/pokemon.c/battle-summary patches from apply_source_overlay.py.
void RogueNpc_OpenAbilityChoices(void)
{
    u16 i;
    static const u8 sSpeciesDefault[] = _("SPECIES DEFAULT");
    if (NpcSelectedPokemon() == NULL)
    {
        gSpecialVar_Result = NPC_CANCEL;
        return;
    }
    NpcBeginList(ABILITIES_COUNT + 1u);
    ScriptMenu_ScrollingMultichoiceDynamicAppendOption(
        sSpeciesDefault, NPC_CHOICE_BASE);
    for (i = 1; i < ABILITIES_COUNT; i++)
        ScriptMenu_ScrollingMultichoiceDynamicAppendOption(
            gAbilityNames[i], NPC_CHOICE_BASE + i);
    ScriptMenu_ScrollingMultichoiceDynamicEnd();
}

void RogueNpc_RememberAbilityChoice(void)
{
    sChosenAbility = gSpecialVar_Result;
}

void RogueNpc_ApplyAbility(void)
{
    struct Pokemon *mon = NpcSelectedPokemon();
    u16 ability;
    u16 encoded;
    gSpecialVar_Result = FALSE;
    if (!mon || sChosenAbility < NPC_CHOICE_BASE ||
        sChosenAbility >= NPC_CHOICE_BASE + ABILITIES_COUNT)
        return;
    ability = sChosenAbility - NPC_CHOICE_BASE;
    encoded = ability == ABILITY_NONE ? 0u : ability + 1u;
    SetMonData(mon, MON_DATA_ROGUE_ABILITY_OVERRIDE, &encoded);
    if (GetMonData(mon, MON_DATA_ROGUE_ABILITY_OVERRIDE, NULL) != encoded)
        return;
    if (encoded != 0u && GetMonAbility(mon) != ability)
        return;
    gSpecialVar_Result = TRUE;
}
