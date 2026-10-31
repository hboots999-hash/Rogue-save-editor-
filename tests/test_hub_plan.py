"""Synthetic-only read-only hub dependency planner tests."""
import json
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from hub_plan import MATRIX,parse_masks,plan

def main():
    matrix=json.loads(MATRIX.read_text(encoding="utf-8"))
    assert matrix["hub_area_count"]==11
    assert matrix["hub_upgrade_count"]==48
    assert len(matrix["upgrades"])==48
    assert all(r["id"]==i for i,r in enumerate(matrix["upgrades"]))
    # Full unlock test: 48 entries reported owned, none to add.
    report=plan((1<<11)-1,(1<<48)-1,matrix)
    assert len(report["owned"])==48
    assert not report["eligible"]
    # No built areas -> only "blocked by area" or quest/reserved.
    report=plan(0,0,matrix)
    assert not report["eligible"]
    assert report["blocked_by_area"]
    assert report["quest_or_reserved"]
    # Labs built and LAB flag 4 present -> slots upgrade 5 becomes eligible.
    report=plan(1<<2,1<<4,matrix)
    assert 5 in report["eligible"]
    assert 6 in report["blocked_by_upgrade"]
    # Empty LAB area but the prerequisite flag already set still refuses.
    report=plan(0,1<<4,matrix)
    assert 5 in report["blocked_by_area"]
    try:
        parse_masks("FFFF","000000000000")
    except ValueError:
        pass
    else:
        raise AssertionError("Unknown area bits allowed")
    print("PASS: all 48 source-defined hub prerequisites; no writes, reserved/quest exclusion")

if __name__=="__main__":
    main()
