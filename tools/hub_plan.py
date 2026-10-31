#!/usr/bin/env python3
"""Read-only Hub upgrade eligibility planner.

Uses source-derived 48-upgrade dependency matrix for EX 2.1.2 candidate
layout, not a ROM identity or saved-game writer. Accepts *hex bitfields*,
not raw SAV bytes. Never modifies files or unlock flags.
"""
import argparse
import json
from pathlib import Path

MATRIX = Path(__file__).resolve().parent.parent/"docs/hub_upgrade_matrix.json"

def parse_masks(area_hex: str, upgrade_hex: str) -> tuple[int,int]:
    try:
        areas=int(area_hex.replace(" ",""),16)
        upgrades=int(upgrade_hex.replace(" ",""),16)
    except ValueError as exc:
        raise ValueError("Both arguments must be hexadecimal bitmasks") from exc
    if areas<0 or areas>=2**16 or areas&~((1<<11)-1):
        raise ValueError("Area mask must contain only eleven defined bits")
    if upgrades<0 or upgrades>=2**48:
        raise ValueError("Upgrade mask must contain at most 48 defined bits")
    return areas,upgrades

def plan(area_mask: int, upgrade_mask: int, matrix: dict) -> dict:
    rows=matrix["upgrades"]
    if len(rows)!=48 or any(r["id"]!=i for i,r in enumerate(rows)):
        raise ValueError("Upgrade matrix incomplete or out of order")
    if any(not 0<=r["area_id"]<11 or any(not 0<=j<48 for j in r["requires"]) for r in rows):
        raise ValueError("Upgrade matrix contains impossible IDs")
    result={"owned":[],"eligible":[],"blocked_by_area":[],"blocked_by_upgrade":[],
            "quest_or_reserved":[]}
    for row in rows:
        i=row["id"]
        if upgrade_mask & (1<<i):
            result["owned"].append(i)
        elif row["special_quest_or_hidden"] or row["reserved_self_requirement"]:
            result["quest_or_reserved"].append(i)
        elif not area_mask & (1<<row["area_id"]):
            result["blocked_by_area"].append(i)
        elif any(not (upgrade_mask&(1<<dep)) for dep in row["requires"]):
            result["blocked_by_upgrade"].append(i)
        else:
            result["eligible"].append(i)
    assert sum(map(len,result.values()))==48
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--area-mask",required=True,help="two serialized area-flag bytes as little-endian HEX, e.g. 0300")
    p.add_argument("--upgrade-mask",required=True,help="six serialized upgrade-flag bytes as little-endian HEX")
    args=p.parse_args()
    matrix=json.loads(MATRIX.read_text(encoding="utf-8"))
    area_bytes=bytes.fromhex(args.area_mask)
    upgrade_bytes=bytes.fromhex(args.upgrade_mask)
    if len(area_bytes)!=2 or len(upgrade_bytes)!=6:
        p.error("Exactly 2 area bytes and 6 upgrade bytes are required")
    areas,upgrades=parse_masks(area_bytes[::-1].hex(),upgrade_bytes[::-1].hex())
    report=plan(areas,upgrades,matrix)
    print("READ-ONLY SOURCE-DERIVED ELIGIBILITY (NOT A SAVE EDIT)")
    print("Source:",matrix["source_repository"],matrix["source_commit"])
    for category,ids in report.items():
        print(f"{category} ({len(ids)}):", ", ".join(str(i)+":"+matrix["upgrades"][i]["constant"].replace("HUB_UPGRADE_","") for i in ids))
    print("WARNING: Unknown actual ROM/coordinate ABI. This is not a safe hub unlock recommendation.")

if __name__=="__main__":
    main()
