#!/usr/bin/env bash
# Phase17 independent STATIC scope audit of exact pinned Phase20 source tree.
# Does not execute installed-game code and is NOT a runtime security proof.
set -euo pipefail
p20="${1:?phase20 repo checkout required}"
p18="${2:?phase18 repo checkout required}"
moves="${3:?phase18 move-research repo checkout required}"
test -f "$p20/phase20/main.cpp"
test -f "$p20/phase20/offline_edit_v1.hpp"
test -f "$p20/phase20/read_only_v1.hpp"
test -f "$p20/phase20/move_catalog_bridge_v1.hpp"
test -f "$moves/phase18/move_replacement_v2.hpp"
test -f "$p18/phase18/raw_v1_sd.hpp"
test -f "$p18/phase18/offline_export.hpp"
test -f "$p18/phase18/multi_export_v3.hpp"
native=("$p20/phase20/main.cpp" "$p20/phase20/read_only_v1.hpp" \
        "$p20/phase20/offline_edit_v1.hpp" \
        "$p20/phase20/move_catalog_bridge_v1.hpp")
# No Phase20 UI TU may open an installed title or call a restore backend.
if grep -En 'FSPXI_|FSUSER_OpenArchive|FSFILE_Write|FS_OPEN_WRITE|AGBSAVE|ARCHIVE_USER_SAVEDATA|ARCHIVE_SAVEDATA|phase14::execute|phase14::recover|NativeBackend' "${native[@]}"; then
    echo 'BLOCKED: Phase20 native UI acquired a forbidden installed-game IO/restore interface' >&2
    exit 1
fi
# Phase18's only native create/write handle is explicitly scoped to SDMC,
# the app-owned raw copied import, backup and draft paths.
grep -Fq 'FSUSER_OpenArchive(&archive,ARCHIVE_SDMC' "$p18/phase18/raw_v1_sd.hpp"
grep -Fq 'const std::string prefix="sdmc:";' "$p18/phase18/raw_v1_sd.hpp"
grep -Fq 'if(dir!="sdmc:/3ds/PKSM-Rogue")return false;' "$p18/phase18/offline_export.hpp"
grep -Fq 'std::fopen(phase19::OfflineImportPath,"rb")' "$p20/phase20/read_only_v1.hpp"
grep -Fq 'phase19::OfflineImportPath,"sdmc:/3ds/PKSM-Rogue"' "$p20/phase20/offline_edit_v1.hpp"
grep -Fq 'phase20::batchShortcut(state,observation,batch)' "$p20/phase20/main.cpp"
# Current Phase20 includes a separate READ-ONLY, fixed-size, SHA-matched
# move catalogue file. It does NOT confer installed ROM identity.
grep -Fq 'std::fopen(path,"rb")' "$p20/phase20/move_catalog_bridge_v1.hpp"
grep -Fq 'sdmc:/3ds/PKSM-Rogue/catalogs/exact_rom_move_metadata.bin' "$p20/phase20/move_catalog_bridge_v1.hpp"
grep -Fq 'if(!rogue18::move_v2::authenticate(db))return false;' "$p20/phase20/move_catalog_bridge_v1.hpp"
grep -Fq 'static_assert(!rogue18::move_v2::ApplyApproved' "$p20/phase20/move_catalog_bridge_v1.hpp"
grep -Fq 'constexpr bool ApplyApproved=false;' "$moves/phase18/move_replacement_v2.hpp"
grep -Fq 'else if(localMoves.read())' "$p20/phase20/main.cpp"
grep -Fq '000400000f73a700' "$p20/.github/workflows/phase20-readonly.yml"
grep -Fq '0004000007d7ba00' "$p20/.github/workflows/phase20-readonly.yml"
# Prevent a newly introduced alternate native title archive in the direct
# SD write helper. FSUSER_CreateFile is allowed only on ARCHIVE_SDMC.
if grep -En 'ARCHIVE_USER_SAVEDATA|ARCHIVE_SAVEDATA|AGBSAVE|FSPXI_' \
  "$p18/phase18/raw_v1_sd.hpp" "$p18/phase18/offline_export.hpp" \
  "$p18/phase18/multi_export_v3.hpp" \
  "$moves/phase18/move_replacement_v2.hpp"; then
    echo 'BLOCKED: unreviewed save/archive access in native SD helper' >&2
    exit 1
fi
echo 'PASS STATIC PINNED SOURCE: Phase20 UI + SHA-gated move browser lack installed-game restore calls; Phase18 exporter uses SDMC.'
echo 'WARNING: this is NOT an ARM11 runtime proof or safe installed-game restore authority.'
