#!/usr/bin/env python3
"""Execute pinned upstream functions with synthetic dependencies, never an editor.

This is a source-semantics probe, NOT a save mutator or exact-ROM certification.
Upstream function bodies are read from a separately SHA-checked public cache.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile
from fetch_public_source import read_sources


def balanced(text, start):
    # Preserve positions while hiding braces in C comments and literals.
    masked = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                    lambda m: ' ' * len(m[0]), text, flags=re.S)
    at = masked.index('{', start)
    depth = 1
    end = at + 1
    while depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    return text[start:end]


def function(text, name):
    pattern = r'^(?:static\s+)?\w+\s+' + re.escape(name) + r'\([^;]*?\)\s*(?://[^\n]*)?\s*\{'
    matches = list(re.finditer(pattern, text, re.M))
    if len(matches) != 1:
        raise ValueError(f'expected one definition of {name}, found {len(matches)}')
    return balanced(text, matches[0].start())


def struct(text, name):
    start = re.search(r'^struct ' + name + r'\s*\{', text, re.M).start()
    return balanced(text, start) + ';'


def case(text, label):
    start = text.index('case ' + label + ':')
    return balanced(text, text.index('{', start))


def preprocess(text):
    # Evaluate upstream conditionals, keep move names symbolic; no full table is saved.
    text = re.sub(r'^\s*#include[^\n]*', '', text, flags=re.M)
    return subprocess.run(['cc', '-E', '-P', '-x', 'c', '-DROGUE_EXPANSION=1', '-'],
                          input=text, text=True, capture_output=True, check=True).stdout


def move_pp(text):
    entries = re.findall(r'\[(MOVE_\w+)\]\s*=\s*\{(.*?)\}', preprocess(text), re.S)
    return {name: int(re.search(r'\.pp\s*=\s*(\d+)', body)[1]) for name, body in entries}


PREFIX = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef uint8_t bool8; typedef uint32_t bool32;
#define TRUE 1
#define FALSE 0
#define ROGUE_EXPANSION 1
#define NUM_ABILITY_SLOTS 3
#define NUM_NORMAL_ABILITY_SLOTS 2
#define ABILITY_NONE 0
#define TYPE_NONE 255
#define NUM_NATURES 25
#define POKEMON_NAME_LENGTH 10
#define PLAYER_NAME_LENGTH 7
#define NUM_SUBSTRUCT_BYTES 12
#define PARTY_SIZE 6
enum { MON_DATA_HIDDEN_NATURE };
static int config, run;
static struct { u8 revisedModeEnabled; } gRogueRun;
static int Rogue_GetConfigRange(int unused) { (void)unused; return config; }
static int Rogue_IsRunActive(void) { return run; }
union PokemonSubstruct { u8 bytes[12]; };
struct RoguePartyMon { u8 bytes[4]; };
'''

STUBS = r'''
static unsigned recalcCalls;
static u8 GetNatureFromPersonality(u32 personality);
static u8 GetNature(struct Pokemon *mon);
static void SetNatureBoxMon(struct BoxPokemon *mon, u8 nature);
// Observe the invocation only. This stub does NOT implement game stat arithmetic.
static void CalculateMonStats(struct Pokemon *mon) { (void)mon; ++recalcCalls; }
static struct { u8 playerPartyCount; struct Pokemon playerParty[6]; } saved;
static struct { u8 playerTrainerId[4]; } saved2;
static void *unusedPointer;
#define gSaveBlock1Ptr (&saved)
#define gSaveBlock2Ptr (&saved2)
static u8 gPlayerPartyCount;
static struct Pokemon gPlayerParty[6];
static struct RoguePokemonBaseStats gRogueSpeciesInfo[4];
static struct { struct RoguePokemonBaseStats baseStats; } gRoguePokemonProfiles_Revised[4];
static u16 gLastUsedAbility;
static u32 customId;
static u16 customAbilities[3];
static unsigned customCount, giftLookups;
static u32 RogueGift_GetCustomMonIdBySpecies(u16 species, u32 otId) {
    (void)species; (void)otId; ++giftLookups; return customId;
}
static u16 RogueGift_GetCustomMonAbilityCount(u32 id) { (void)id; return customCount; }
static u16 RogueGift_GetCustomMonAbility(u32 id, u8 i) { (void)id; return customAbilities[i]; }
static struct { u8 pp; } gBattleMoves_Mainline[3]={{0},{10},{3}},
                         gBattleMoves_Revised[3]={{0},{15},{3}};
static const u8 gPPUpGetMask[4]={3,12,48,192};
'''

CHECKS = r'''
int main(void) {
    (void)unusedPointer;
    (void)Rogue_GetConfigRange; (void)Rogue_IsRunActive; (void)GetRevisionModeActive_Slow;
    // Exhaustive selector truth table, including disagreement with cached run state.
    for (int c=0;c<4;++c) for (int r=0;r<2;++r) for (int cache=0;cache<2;++cache) {
        config=c; run=r; gRogueRun.revisedModeEnabled=cache;
#ifdef ROGUE_FEATURE_REVISED_MODE
        int expected = r ? cache : (c==REVISION_MODE_ALWAYS_ON);
        assert(GetRevisionModeActive_Slow(r)==(c==REVISION_MODE_ALWAYS_ON || (c==REVISION_MODE_IN_RUN && r)));
#else
        int expected=0;
        assert(GetRevisionModeActive_Slow(r)==0);
#endif
        assert(Rogue_GetRevisionModeActive()==expected);
        const u8 ten[4]={10,12,14,16}, fifteen[4]={15,18,21,24}, three[4]={3,3,4,4};
        for (u8 lane=0;lane<4;++lane) for (int bonus=0;bonus<256;++bonus) {
            int ups=(bonus>>(2*lane))&3;
            assert(CalculatePPWithBonus(1,bonus,lane)==(expected?fifteen[ups]:ten[ups]));
            assert(CalculatePPWithBonus(2,bonus,lane)==three[ups]);
        }
    }
    assert(sizeof(struct BoxPokemon)==80 && sizeof(struct Pokemon)==104);
    assert(sizeof(struct PokemonSubstruct3)==12);
    assert(offsetof(struct Pokemon, attack)==90);
    // Execute upstream setter/getter branches across all PID residues/natures/languages.
    unsigned natureCases=0;
    for (u32 p=0;p<25;++p) for (u8 n=0;n<25;++n) for (u8 l=0;l<8;++l) {
        struct Pokemon mon, before;
        memset(&mon,0xA5,sizeof mon);
        mon.box.personality=p; mon.box.language=l; before=mon;
        SetNatureBoxMon(&mon.box,n);
        assert(GetNature(&mon)==n);
        assert(mon.box.language==l);
        const u8 *a=(const u8 *)&mon, *b=(const u8 *)&before;
        for (size_t i=0;i<sizeof mon;++i) if (i!=18) assert(a[i]==b[i]);
        assert(a[18]==((b[18]&7)|(((p%25)^n)<<3)));
        ++natureCases;
    }
    // The actual mint wrapper requests recalc; direct header edits do not call it.
    struct Pokemon mon={0}; recalcCalls=0;
    SetNature(&mon,3); assert(recalcCalls==1 && GetNature(&mon)==3);
    saved.playerPartyCount=6;
    for (int i=0;i<6;++i) { saved.playerParty[i]=mon; saved.playerParty[i].attack=777+i; }
    recalcCalls=0; LoadPlayerParty();
    assert(gPlayerPartyCount==6 && recalcCalls==0);
    for (int i=0;i<6;++i) assert(gPlayerParty[i].attack==777+i);
    // Measure actual source bitfields with host ABI; does not certify the target ABI.
    for (u32 pattern=0;pattern<2;++pattern) for (u32 slot=0;slot<4;++slot) {
        struct PokemonSubstruct3 misc; memset(&misc,pattern?255:0,sizeof misc);
        u32 before,after; memcpy(&before,((u8 *)&misc)+8,4);
        misc.abilityNum=slot; memcpy(&after,((u8 *)&misc)+8,4);
        assert(after==((before&~0x60000000u)|(slot<<29)));
    }
    // Exhaustive fallback truth table using fabricated ability IDs (never real mappings).
    unsigned abilityCases=0;
    for (u16 a=0;a<3;++a) for (u16 b=0;b<3;++b) for (u16 c=0;c<3;++c) {
        u16 choices[3]={a,b,c};
        for (u8 mode=0;mode<2;++mode) for (u8 slot=0;slot<4;++slot) {
            memcpy(gRogueSpeciesInfo[1].abilities,choices,sizeof choices);
            memset(&gRoguePokemonProfiles_Revised[1],0,sizeof gRoguePokemonProfiles_Revised[1]);
            gRoguePokemonProfiles_Revised[1].baseStats.types[0]=TYPE_NONE;
            u16 expected=slot<3?choices[slot]:0;
            // Even invalid slot 3 falls through the game's hidden-slot search.
            // An editor must reject it rather than infer validity from a display result.
            if (!expected && slot>=2) expected=choices[2];
            if (!expected) for (int i=0;i<3;++i) if (choices[i]) {expected=choices[i]; break;}
            customId=0;
            assert(GetAbilityBySpecies_ForRevised(1,slot,0,mode)==expected);
            ++abilityCases;
        }
    }
    // Revised fallback replaces the entire abilities array if its first entry is NONE.
    gRogueSpeciesInfo[1]=(struct RoguePokemonBaseStats){10,20,30,40,50,60,{1,2},{11,12,13}};
    struct RoguePokemonBaseStats *rev=&gRoguePokemonProfiles_Revised[1].baseStats;
    *rev=(struct RoguePokemonBaseStats){99,98,97,96,95,94,{5,6},{0,22,23}};
    struct RoguePokemonBaseStats out;
    Rogue_GetPokemonBaseStatsFor(1,&out,1);
    assert(out.baseHP==99 && out.types[0]==5 && out.abilities[2]==13);
    *rev=(struct RoguePokemonBaseStats){0,98,97,96,95,94,{TYPE_NONE,6},{21,0,23}};
    Rogue_GetPokemonBaseStatsFor(1,&out,1);
    assert(out.baseHP==10 && out.baseAttack==20 && out.types[0]==1 && out.types[1]==2);
    assert(out.abilities[0]==21 && out.abilities[1]==0 && out.abilities[2]==23);
    assert(GetAbilityBySpecies_ForRevised(1,1,0,1)==21);
    // Player identity is part of resolution: same OT suppresses gift lookup.
    memset(&saved2,0,sizeof saved2); customId=1; customCount=3;
    customAbilities[0]=31; customAbilities[1]=0; customAbilities[2]=0;
    giftLookups=0;
    assert(GetAbilityBySpecies_ForRevised(1,2,0,0)==13 && giftLookups==0);
    assert(GetAbilityBySpecies_ForRevised(1,2,1,0)==31 && giftLookups==1);
    customCount=0; assert(GetAbilityBySpecies_ForRevised(1,2,1,0)==13);
    customId=0; customCount=3; assert(GetAbilityBySpecies_ForRevised(1,2,1,0)==13);
    printf("source probe passed: %u nature cases, %u ability cases, 16 mode cases; recalc invocation + loader + bitfields\n",natureCases,abilityCases);
    return 0;
}
'''


def harness(src):
    p = src['src/pokemon.c']
    h = src['include/pokemon.h']
    mode_defines = '\n'.join(re.findall(
        r'^#define (?:CONFIG_RANGE_REVISION_MODE|REVISION_MODE_ALWAYS_ON|REVISION_MODE_IN_RUN)\s+\d+.*$',
        src['include/constants/rogue.h'], re.M))
    assert len(mode_defines.splitlines()) == 3
    pieces = [PREFIX, mode_defines, struct(h, 'PokemonSubstruct3'), struct(h, 'BoxPokemon'),
              struct(h, 'Pokemon'), struct(src['include/rogue.h'], 'RoguePokemonBaseStats'), STUBS]
    # Compile upstream case bodies, not a second implementation of the mint encoding.
    setter = case(p[p.index('void SetBoxMonData('):], 'MON_DATA_HIDDEN_NATURE')
    getter = case(p[p.index('u32 GetBoxMonData3('):], 'MON_DATA_HIDDEN_NATURE')
    pieces += [r'''static void SetBoxMonData(struct BoxPokemon *boxMon, int field, const u8 *data) {
        (void)field;
        #define SET8(x) ((x)=*data)
        do ''' + setter + r''' while (0);
        #undef SET8
    }''', r'''static u32 GetMonData(struct Pokemon *mon, int field, void *ignored) {
        (void)field; (void)ignored; struct BoxPokemon *boxMon=&mon->box; u32 retVal=0;
        do ''' + getter + r''' while (0); return retVal;
    }''']
    for name in ['GetNatureFromPersonality', 'GetNature', 'SetNatureBoxMon', 'SetNature', 'IsOtherTrainer']:
        pieces.append(function(p, name))
    pieces.append(function(src['src/rogue_baked.c'], 'Rogue_GetPokemonBaseStatsFor'))
    pieces.append(function(p, 'GetAbilityBySpecies_ForRevised'))
    pieces.append(function(src['src/load_save.c'], 'LoadPlayerParty'))
    for name in ['GetRevisionModeActive_Slow', 'Rogue_GetRevisionModeActive']:
        pieces.append(function(src['src/rogue_controller.c'], name))
    pieces.append(re.search(r'^#define gBattleMoves .*$', h, re.M)[0])
    pieces.append(function(p, 'CalculatePPWithBonus'))
    pieces.append(CHECKS)
    return '\n'.join(pieces)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    src = read_sources(args.source)
    # Regression of the upstream default: Revised Mode is commented out, not enabled.
    config = preprocess(src['include/constants/rogue.h'] + '\n#ifdef ROGUE_FEATURE_REVISED_MODE\n#error unexpected default\n#endif\n')
    assert config is not None
    mainline = move_pp(src['src/data/battle_moves.h'])
    revised = move_pp(src['src/data/battle_moves_revised.h'])
    assert mainline.keys() == revised.keys()
    assert mainline['MOVE_ICE_BEAM'] == 10 and revised['MOVE_ICE_BEAM'] == 15
    differences = sum(mainline[m] != revised[m] for m in mainline)
    assert len(mainline) == 936 and differences == 40
    print(f'PUBLIC source tables: {len(mainline)} named initializers; {differences} PP disagreements (not exact-ROM identity)', flush=True)
    with tempfile.TemporaryDirectory(prefix='rogue-public-probe-') as tmp:
        c = Path(tmp) / 'probe.c'
        c.write_text(harness(src))
        for enabled in [False, True]:
            flags = ['-DROGUE_FEATURE_REVISED_MODE=1'] if enabled else []
            if args.sanitize:
                flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
            out = Path(tmp) / 'probe'
            # Preserve upstream definitions; these two warnings occur in upstream code.
            subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-Wno-unused-parameter', '-Wno-sign-compare', '-O1', *flags,
                            str(c), '-o', str(out)], check=True)
            print(f'Revised feature compiled {"on" if enabled else "off"}:', flush=True)
            subprocess.run([str(out)], check=True)
    print('PASS: pinned public source only; no ROM/runtime/gameplay certification')


if __name__ == '__main__':
    main()
