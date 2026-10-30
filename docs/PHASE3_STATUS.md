# PKSM Rogue Edition - Phase 3 experimental inspection

**Not an installable release. Do not install a PKSM-based test CIA on your 3DS yet.**

This development branch holds the read-only candidate parser, a PKSM screen (SELECT at the title loader), and a guarded script for applying the overlay to a clean FlagBrew/PKSM checkout.

## Developer workflow

1. Clone FlagBrew/PKSM including submodules into a disposable branch or copy.
2. Run: python3 tools/apply_to_pksm.py /path/to/PKSM
3. Review the resulting PKSM diff and application identity.
4. Compile PKSM using supported devkitPro tools. A successful host build does not prove hardware compatibility.

The screen reads a COPIED 128 KiB raw save only from sdmc:/3ds/PKSM-Rogue/imports/00000001.sav, using read-only binary I/O. It displays numeric candidate data, not verified names. Its path does not invoke PKSM's standard Gen 3 save loader.

The proposed app ID is derived from UNIQUE_ID=0xF73A2. It has not been checked against the user's installed titles. Upstream PKSM's other SD paths/settings could overlap with installed PKSM, so a separate title ID is not sufficient isolation.

## Validation
GitHub Actions compiles the bridge and UI source against mock host headers, runs invalid-save/preservation smoke checks and tests synthetic patch guards. No full PKSM compilation, CIA launch, installed-save read, restore or real-ROM compatibility has been demonstrated.

## Licensing
Candidate parser from RogueSM (MIT; roguesm_core/LICENSE). PKSM is GPL-3.0-or-later with additional attribution requirements. Preserve upstream copyrights, licensing and notices in any derived distribution.
