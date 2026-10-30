"""Fail-closed regression tests for the experimental PKSM startup restriction."""
from pathlib import Path
from tempfile import TemporaryDirectory
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from restrict_startup import restrict

APP = '''#include "app.hpp"
Result App::init(const std::string& execPath)
{
if (assetStore.verdict() != AssetStore::Verdict::Ok)
    {
        if (!update(execPath)) { return -1; }
    }
if (Configuration::getInstance().autoUpdate() && update(execPath)) { return -1; }
    const bool contentUp =
        subsystems.acquire("banks", Banks::init) &&
        subsystems.acquire(
            "cart scan", [] { Threads::background(cartScan); },
            stopCartScan);
    ScreenStack::push(std::make_unique<TitleLoadScreen>());
    return 0;
}
'''
def setup(root, app_text=APP):
    app=root/"3ds/source/app.cpp"
    app.parent.mkdir(parents=True,exist_ok=True)
    app.write_text(app_text,encoding="utf-8")
    inspector=root/"3ds/source/gui/screen/RogueInspectScreen.cpp"
    inspector.parent.mkdir(parents=True,exist_ok=True)
    inspector.write_text("// test overlay\n",encoding="utf-8")
    return app

def main():
    with TemporaryDirectory() as d:
        root=Path(d); p=setup(root); restrict(root)
        s=p.read_text()
        assert "constexpr bool rogueInspectOnly = true;" in s
        assert "const bool contentUp = rogueInspectOnly || (" in s
        assert "stopCartScan));" in s
        assert "if (!rogueInspectOnly && Configuration::getInstance().autoUpdate()" in s
        assert "ScreenStack::push(std::make_unique<RogueInspectScreen>());" in s
        before=p.read_bytes()
        try: restrict(root)
        except ValueError: pass
        else: raise AssertionError("Double restriction accepted")
        assert before == p.read_bytes()
    with TemporaryDirectory() as d:
        root=Path(d); p=setup(root,APP.replace('stopCartScan);','stopNonexistent);'))
        before=p.read_bytes()
        try: restrict(root)
        except ValueError: pass
        else: raise AssertionError("Missing anchor accepted")
        assert before == p.read_bytes()
    print("PASS: Rogue-only startup, updater scan guards, and refuse-without-writes")

if __name__ == "__main__":
    main()
