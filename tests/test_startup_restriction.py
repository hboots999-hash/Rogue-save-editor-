"""Fail-closed tests for PKSM Rogue-only startup and storage-erase blocking."""
from pathlib import Path
from tempfile import TemporaryDirectory
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from restrict_startup import restrict

APP = '''#include "app.hpp"
    bool confirmExtdataReset()
    {
        return true;
    }

    Result HBLDR_SetTarget(const char* path)
    {
        return -1;
    }
Result App::init(const std::string& execPath)
{
if (assetStore.verdict() != AssetStore::Verdict::Ok)
    {
        if (!update(execPath)) { return -1; }
    }
if (Configuration::getInstance().autoUpdate() && update(execPath)) { return -1; }
    Archive::init(execPath, confirmExtdataReset);
    const bool contentUp =
        subsystems.acquire("banks", Banks::init) &&
        subsystems.acquire(
            "cart scan", [] { Threads::background(cartScan); },
            stopCartScan);
    ScreenStack::push(std::make_unique<TitleLoadScreen>());
    return 0;
}
'''

ARCHIVE = '''Result Archive::init(const std::string& execPath, bool (*confirmExtdataReset)())
{
    Result res = 0;
    const auto confirmReset = [confirmExtdataReset]
    { return confirmExtdataReset && confirmExtdataReset(); };

    if (R_FAILED(data().result()))
    {
        if (R_SUMMARY(openResult) == RS_INVALIDSTATE)
        {
            if (!confirmReset())
            {
                return openResult;
            }

            if (R_FAILED(res = resetPKSMExtdataArchive(execPath)))
            {
                return res;
            }
        }
    }
    else if (!extdataReadable())
    {
        Logging::startupLog("archive", "ext data root unreadable");
        if (!confirmReset())
        {
            return -1;
        }
        if (R_FAILED(res = resetPKSMExtdataArchive(execPath)))
        {
            return res;
        }
    }
    else
    {
        auto checkFile = data().file(fsMakePath(PATH_UTF16, u"/sizeCheck"), FS_OPEN_READ);
        if (!checkFile)
        {
            FSUSER_DeleteExtSaveData(PKSM_ARCHIVE_DATA);
            createPKSMExtdataArchive(execPath);
        }
        else
        {
            checkFile->close();
        }
    }
    return 0;
}

void Archive::exit(void)
{
}
'''


def setup(root, app_text=APP, archive_text=ARCHIVE):
    app=root/"3ds/source/app.cpp"
    app.parent.mkdir(parents=True, exist_ok=True)
    app.write_text(app_text, encoding="utf-8")
    archive=root/"3ds/source/io/Archive.cpp"
    archive.parent.mkdir(parents=True, exist_ok=True)
    archive.write_text(archive_text, encoding="utf-8")
    inspector=root/"3ds/source/gui/screen/RogueInspectScreen.cpp"
    inspector.parent.mkdir(parents=True, exist_ok=True)
    inspector.write_text("// test overlay\n", encoding="utf-8")
    return app, archive


def main():
    with TemporaryDirectory() as d:
        root=Path(d)
        app,archive=setup(root)
        restrict(root)
        s=app.read_text()
        a=archive.read_text()
        assert "constexpr bool rogueInspectOnly = true;" in s
        assert "const bool contentUp = rogueInspectOnly || (" in s
        assert "stopCartScan));" in s
        assert "if (!rogueInspectOnly && Configuration::getInstance().autoUpdate()" in s
        assert "ScreenStack::push(std::make_unique<RogueInspectScreen>());" in s
        assert "Archive::init(execPath, nullptr);" in s
        assert "confirmExtdataReset()" not in s
        assert "mountNewPKSMExtdataArchive(execPath)" in a
        assert "resetPKSMExtdataArchive(" not in a
        assert "FSUSER_DeleteExtSaveData" not in a
        assert "confirmReset()" not in a
        assert "Rogue inspect: extdata missing sizeCheck" in a
        before=(app.read_bytes(),archive.read_bytes())
        try: restrict(root)
        except ValueError: pass
        else: raise AssertionError("Second restriction unexpectedly accepted")
        assert before == (app.read_bytes(),archive.read_bytes())

    with TemporaryDirectory() as d:
        root=Path(d)
        app,archive=setup(root,APP.replace("stopCartScan);","stopNonexistent);"))
        before=(app.read_bytes(),archive.read_bytes())
        try: restrict(root)
        except ValueError: pass
        else: raise AssertionError("Missing app anchor unexpectedly accepted")
        assert before == (app.read_bytes(),archive.read_bytes())

    with TemporaryDirectory() as d:
        root=Path(d)
        app,archive=setup(root,APP,ARCHIVE.replace("FSUSER_DeleteExtSaveData","unknownFunction"))
        before=(app.read_bytes(),archive.read_bytes())
        try: restrict(root)
        except ValueError: pass
        else: raise AssertionError("Changed migration unexpectedly accepted")
        assert before == (app.read_bytes(),archive.read_bytes())

    print("PASS: Rogue-only startup; no reset prompts or extdata deletion; fail closed")


if __name__ == "__main__":
    main()
