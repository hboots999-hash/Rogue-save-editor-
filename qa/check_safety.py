#!/usr/bin/env python3
"""Fail-closed source invariants for pinned and isolated QA checkouts."""
from pathlib import Path
import re
import subprocess

root=Path("dependencies")
p17=(root/"phase17/phase14/authenticated_restore.hpp").read_text()
gate=(root/"phase17/phase17/recovery_evidence_gate.hpp").read_text()
p18=(root/"phase18/phase18/editor_model.hpp").read_text()
engine=(root/"phase18/phase18/offline_engine.hpp").read_text()
p19=(root/"phase19/phase19/ui.hpp").read_text()
importer=(root/"phase19/phase19/read_only_import.hpp").read_text()
native=(root/"phase19/phase19/main.cpp").read_text()
workflow=(root/"phase19/.github/workflows/phase19-editor-ui.yml").read_text()

def require(ok,reason):
    if not ok:
        raise SystemExit("BLOCK: "+reason)
    print("PASS:",reason)

require(bool(re.search(r"constexpr\s+bool\s+LiveWritesEnabled\s*=\s*false\s*;",p17)),
        "Phase14 live-write gate is literally false")
require("bool mayWriteInstalledGame()const{return false;}" in gate and
        "bool mayOverwriteJournal()const{return false;}" in gate,
        "Phase17 recovery gate cannot return write authorization")
require(all(re.search(r"\b"+k+r"\s*=\s*false\b",p18) for k in
            ("arbitraryMoveId","abilityId","nature","insertItem","pcEdit","liveWrite","restore")),
        "Phase18 unsupported editor capabilities default disabled")
require(all(x in engine for x in
            ("verifiedBagInsertion","verifiedH168BagUpgradeOffset",
             "verifiedNatureModifier","verifiedAbilitySlot","verifiedMoveIds")),
        "Phase18 strict evidence fields exist")
require("if(!s.demo.synthetic) return false" in p19 and
        "if(!s.demo.synthetic)" in p19,
        "Phase19 imported data never previews editing")
require("std::fopen(OfflineImportPath,\"rb\")" in importer and
        "n!=h168::Size" in importer and
        "sdmc:/3ds/PKSM-Rogue/imports/00000001.sav" in importer,
        "Phase19 fixed copy reader opens only rb and validates size")
require("FS_OPEN_WRITE" not in native+importer and
        "FSPXI_WriteFile" not in native+importer and
        "rogue18::propose(" not in native+importer and
        "exportCopied" not in native+importer,
        "Phase19 native viewer has no identified write entrypoint")
require("PHASE19_CANARY_IMPORT" not in importer,
        "No test-only write bypass in imported save reader")
for phase in ("phase17","phase18","phase19"):
    listed=subprocess.check_output(
        ["git","-C",str(root/phase),"ls-files"],text=True).splitlines()
    bad=[x for x in listed if Path(x).suffix.lower() in
         (".sav",".gba",".rom",".cia",".3ds",".3dsx")]
    require(not bad,phase+" checkout has no tracked real saves/ROMs/CIA binaries")

matches=re.findall(r"ref:\s*([0-9a-f]{40})",workflow)
if matches:
    current=subprocess.check_output(
        ["git","-C",str(root/"phase18"),"rev-parse","HEAD"],text=True).strip()
    if any(old!=current for old in matches):
        print("::warning::Phase19 workflow pins Phase18 at",matches[0],
              "but QA tests Phase18 at",current,
              "- re-pin and re-test before integrating")
print("PASS: static source policy checks (not a proof of runtime write isolation)")
