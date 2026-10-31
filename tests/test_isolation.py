"""Regression tests for the fail-closed PKSM path isolation step."""
from pathlib import Path
from tempfile import TemporaryDirectory
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from isolate_pksm import isolate_tree

DIRS = (
    "3ds/source", "3ds/include",
    "common/source", "common/include",
    "core/source", "core/include",
)
def fixture(root: Path, count: int = 25):
    for name in DIRS:
        (root / name).mkdir(parents=True, exist_ok=True)
    (root / "3ds/Makefile").write_text("app PKSM\n", encoding="utf-8")
    (root / "3ds/source/io").mkdir(parents=True, exist_ok=True)
    src = root / "3ds/source/io/Archive.cpp"
    src.write_text(
        'const char *sd = "sdmc:/3ds/PKSM/logs/a.log";\n'
        + ('const char *p = "/3ds/PKSM/backups";\n' * (count - 1))
        + 'const char *unrelated = "/3ds/PKSMExtra/path";\n'
        + 'const char *web = "https://github.com/FlagBrew/PKSM";\n',
        encoding="utf-8"
    )
    return src

def main():
    with TemporaryDirectory() as d:
        root = Path(d)
        path = fixture(root)
        original = path.read_text()
        stats = isolate_tree(root)
        assert stats == {"files": 1, "paths": 25}, stats
        text = path.read_text()
        assert "sdmc:/3ds/PKSM-Rogue/logs" in text
        assert text.count("/3ds/PKSM-Rogue/backups") == 24
        assert "/3ds/PKSMExtra/path" in text
        assert "https://github.com/FlagBrew/PKSM" in text
        assert text != original
        try: isolate_tree(root)
        except ValueError: pass
        else: raise AssertionError("Repeated isolation unexpectedly succeeded")
    with TemporaryDirectory() as d:
        root = Path(d)
        path = fixture(root, count=19)
        original = path.read_bytes()
        try: isolate_tree(root)
        except ValueError: pass
        else: raise AssertionError("Insufficient path evidence unexpectedly accepted")
        assert path.read_bytes() == original
    with TemporaryDirectory() as d:
        root = Path(d)
        path = fixture(root)
        (root / "3ds/source/gui/screen").mkdir(parents=True)
        (root / "3ds/source/gui/screen/RogueInspectScreen.cpp").write_text("guard")
        original = path.read_bytes()
        try: isolate_tree(root)
        except ValueError: pass
        else: raise AssertionError("Overlay-first unexpectedly accepted")
        assert path.read_bytes() == original
    print("PASS: path isolation, nonmatching strings, refusal, and no-edit failure cases")

if __name__ == "__main__":
    main()
